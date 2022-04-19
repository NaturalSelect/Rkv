#include <rkv/WorkerServer.hpp>

#include <sharpen/Quorum.hpp>
#include <sharpen/FileOps.hpp>

#include <rkv/AppendEntriesRequest.hpp>
#include <rkv/AppendEntriesResponse.hpp>
#include <rkv/VoteRequest.hpp>
#include <rkv/VoteResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/MigrateRequest.hpp>
#include <rkv/MigrateResponse.hpp>
#include <rkv/ClearShardRequest.hpp>
#include <rkv/ClearShardResponse.hpp>
#include <rkv/StartMigrationRequest.hpp>
#include <rkv/StartMigrationResponse.hpp>

rkv::WorkerServer::WorkerServer(sharpen::EventEngine &engine,const rkv::WorkerServerOption &option)
    :TcpServer(sharpen::AddressFamily::Ip,option.BindEndpoint(),engine)
    ,selfId_(option.SelfId())
    ,app_(nullptr)
    ,clientLock_()
    ,client_(nullptr)
    ,groups_()
    ,keyCounter_()
    ,counterFile_(nullptr)
    ,migrationLock_()
{
    //make directories
    sharpen::MakeDirectory("./Storage");
    this->app_ = std::make_shared<rkv::KeyValueService>(engine,"./Storage/WorkerDb");
    //create master client
    assert(!option.MasterEmpty());
    this->client_.reset(new rkv::MasterClient{engine,option.MasterBegin(),option.MasterEnd(),std::chrono::milliseconds{1000},10});
    if(!this->client_)
    {
        throw std::bad_alloc();
    }
    //get migrations
    std::vector<rkv::Migration> migrations;
    std::set<sharpen::ByteBuffer> badShards;
    sharpen::TimerPtr timer = sharpen::MakeTimer(sharpen::EventEngine::GetEngine());
    sharpen::AwaitableFuture<bool> future;
    using TimeoutPtr = void(*)(sharpen::Future<bool>&,rkv::MasterClient*);
    future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
    timer->WaitAsync(future,std::chrono::milliseconds{Self::masterTimeout_});
    try
    {
        this->client_->GetMigrations(std::back_inserter(migrations),this->selfId_);
        if(future.IsPending())
        {
            timer->Cancel();
            future.WaitAsync();
        }
    }
    catch(const std::exception&)
    {
        if(future.IsPending())
        {
            timer->Cancel();
            future.WaitAsync();
        }
        throw;   
    }
    std::puts("[Info]Reading migrations");
    for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
    {
        if(!this->ExecuteMigrationAndNotify(*begin))
        {
            badShards.emplace(begin->BeginKey());
        }
    }
    std::printf("[Info]Complete %zu migrations\n",migrations.size());
    //get shards
    std::vector<rkv::Shard> shards{this->FlushShard(&badShards)};
    //get completed migrations
    this->counterFile_ = sharpen::MakeFileChannel("./Storage/CompletedCounter.bin",sharpen::FileAccessModel::All,sharpen::FileOpenModel::CreateOrOpen);
    this->counterFile_->Register(*this->engine_);
    std::uint64_t size{this->counterFile_->GetFileSize()};
    if(!size)
    {
        this->counterFile_->ZeroMemoryAsync(sizeof(size) + sizeof(bool));
    }
    
    std::uint64_t count{0};
    {
        sharpen::FileMemory counterMemory{this->counterFile_->MapMemory(sizeof(std::uint64_t) + sizeof(bool),0)};
        char *tmp = reinterpret_cast<char*>(counterMemory.Get());
        count = *reinterpret_cast<std::uint64_t*>(tmp + sizeof(bool));
    }
    std::vector<rkv::CompletedMigration> completedMigations;
    for (auto begin = shards.begin(),end = shards.end(); begin != end; ++begin)
    {
        future.Reset();
        future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
        timer->WaitAsync(future,std::chrono::milliseconds{Self::masterTimeout_});
        try
        {
            this->client_->GetCompletedMigrations(std::back_inserter(completedMigations),count,begin->GetId());
            if(future.IsPending())
            {
                timer->Cancel();
                future.WaitAsync();
            }
        }
        catch(const std::exception &e)
        {
            if (future.IsPending())
            {
                timer->Cancel();
                future.WaitAsync();
            }
            else
            {
                this->client_->Reset();
            }
            std::fprintf(stderr,"[Error]Cannot get completed migrations because %s\n",e.what());
            break;
        }
    }
    for (auto begin = completedMigations.begin(),end = completedMigations.end(); begin != end; ++begin)
    {
        auto ite = this->groups_.find(begin->GetDestination());
        if(ite == this->groups_.end())
        {
            this->CleaupCompletedMigration(*begin);
        }
    }
}

std::vector<rkv::Shard> rkv::WorkerServer::FlushShard(const std::set<sharpen::ByteBuffer> *excludedSet)
{
    sharpen::TimerPtr timer = sharpen::MakeTimer(sharpen::EventEngine::GetEngine());
    sharpen::AwaitableFuture<bool> future;
    using TimeoutPtr = void(*)(sharpen::Future<bool>&,rkv::MasterClient*);
    std::vector<rkv::Shard> shards;
    try
    {
        std::puts("[Info]Reading shards");
        {
            std::unique_lock<sharpen::AsyncMutex> lock{this->clientLock_};
            future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
            timer->WaitAsync(future,std::chrono::milliseconds{Self::masterTimeout_});
            this->client_->GetShard(std::back_inserter(shards),this->selfId_);
        }
        std::puts("[Info]Reading shards done");
        if(future.IsPending())
        {
            timer->Cancel();
            future.WaitAsync();
        }
    }
    catch(const std::exception &e)
    {
        std::fprintf(stderr,"[Error]Cannot reading shards because %s\n",e.what());
        if(future.IsPending())
        {
            timer->Cancel();
            future.WaitAsync();
        }
        else
        {
            this->client_->Reset();
        }
        throw;
    }
    std::puts("[Info]Loading shards");
    {
        this->groupLock_.LockWrite();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->groupLock_,std::adopt_lock};
        for (auto begin = shards.begin(),end = shards.end(); begin != end; ++begin)
        {
            if(!excludedSet || !excludedSet->count(begin->BeginKey()))
            {
                auto ite = this->groups_.find(begin->GetId());
                if(ite == this->groups_.end() || !ite->second)
                {
                    if(begin->BeginKey().Empty() || this->app_->Exist(begin->BeginKey()) == sharpen::ExistStatus::Exist)
                    {
                        std::unique_ptr<rkv::RaftGroup> group{};
                        try
                        {
                            group.reset(new rkv::RaftGroup{*this->engine_,this->selfId_,rkv::RaftStorage{*this->engine_,Self::FormatStorageName(begin->GetId())},this->app_});
                        }
                        catch(const std::exception& e)
                        {
                            std::fprintf(stderr,"[Error]Cannot load shard because %s\n",e.what());
                            throw;
                        }
                        if(!group)
                        {
                            throw std::bad_alloc();
                        }
                        group->Group().Construct(begin->GetId());
                        std::printf("[Info]Load shard %llu\n",begin->GetId());
                        std::uint64_t lastAppiled{group->Raft().GetLastApplied()};
                        for(auto workerBegin = begin->Workers().begin(),workerEnd = begin->Workers().end(); workerBegin != workerEnd; ++workerBegin)
                        {
                            if(*workerBegin != this->selfId_)
                            {
                                rkv::RaftMember member{*workerBegin,*this->engine_};
                                member.SetCurrentIndex(lastAppiled);
                                group->Raft().Members().emplace(*workerBegin,std::move(member));
                            }
                        }
                        this->groups_.emplace(begin->GetId(),std::move(group));
                        this->shardMap_.emplace(begin->BeginKey(),begin->GetId());
                    }
                }
            }
        }
    }
    std::printf("[Info]Load %zu shards\n",this->groups_.size());
    return shards;
}

std::string rkv::WorkerServer::FormatStorageName(std::uint64_t id)
{
    std::string str{"./Storage/RaftLog"};
    str += std::to_string(id);
    return str;
}

void rkv::WorkerServer::CleaupCompletedMigration(const rkv::CompletedMigration &migration)
{
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->migrationLock_};
        sharpen::FileMemory counterMemory{this->counterFile_->MapMemory(sizeof(std::uint64_t) + sizeof(bool),0)};
        bool *notFirst{reinterpret_cast<bool*>(counterMemory.Get())};
        char *mem{reinterpret_cast<char*>(counterMemory.Get())};
        std::uint64_t *counter{reinterpret_cast<std::uint64_t*>(mem + sizeof(bool))};
        if(*notFirst && migration.GetId() <= *counter)
        {
            return;
        }
        std::vector<sharpen::ByteBuffer> keys;
        {
            auto scanner{this->app_->GetScanner(migration.BeginKey(),migration.EndKey())};
            if(!scanner.IsEmpty())
            {
                keys.reserve(Self::maxKeysPerShard_);
                do
                {
                    keys.emplace_back(scanner.GetCurrentKey());
                } while (scanner.Next());
            }
        }
        try
        {
            //garbage rather than lost
            *counter = migration.GetId();
            *notFirst = true;
            counterMemory.FlushAndWait();
            for(auto begin = keys.begin(),end = keys.end(); begin != end; ++begin)
            {
                this->app_->Delete(*begin);
            }
            std::printf("[Info]%zu keys have been removed\n",keys.size());   
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot clear shard because %s\n",e.what());
        }
    }
}

bool rkv::WorkerServer::ExecuteMigration(const rkv::Migration &migration)
{
    {
        this->groupLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->groupLock_,std::adopt_lock};
        auto shard{this->GetShardId(migration.BeginKey())};
        if(shard.Exist())
        {
            return true;
        }
    }
    using TimeoutPtr = void(*)(sharpen::Future<bool>&,rkv::MasterClient*);
    sharpen::Optional<rkv::Shard> shard;
    sharpen::TimerPtr timer = sharpen::MakeTimer(sharpen::EventEngine::GetEngine());
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->clientLock_};
        sharpen::AwaitableFuture<bool> future;
        future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
        timer->WaitAsync(future,std::chrono::milliseconds{Self::masterTimeout_});
        try
        {
            shard = this->client_->GetShard(migration.GetSource());
            if(!future.IsPending())
            {
                this->client_->Reset();
                return false;
            }
            timer->Cancel();
            future.WaitAsync();
        }
        catch(const std::exception&)
        {
            if(future.IsPending())
            {
                timer->Cancel();
                future.WaitAsync();
            }
            this->client_->Reset();
            return false;
        }
    }
    if(shard.Exist())
    {
        if(!shard.Get().Workers().empty())
        {
            for (auto begin = shard.Get().Workers().begin(),end = shard.Get().Workers().end(); begin != end; ++begin)
            {
                if(*begin == this->selfId_)
                {
                    return true;
                }
            }   
            std::unordered_set<sharpen::IpEndPoint> set;
            std::minstd_rand random{std::random_device{}()};
            std::uniform_int_distribution<std::size_t> distribution{1,shard.Get().Workers().size()};
            sharpen::NetStreamChannelPtr channel;
            while (set.size() != shard.Get().Workers().size())
            {
                std::size_t index{distribution(random) - 1};
                sharpen::IpEndPoint id{shard.Get().Workers()[index]};
                if(!set.count(id))
                {
                    sharpen::NetStreamChannelPtr tmp{sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip)};
                    sharpen::IpEndPoint ep{0,0};
                    tmp->Bind(ep);
                    tmp->Register(*this->engine_);
                    try
                    {
                        if (tmp->ConnectWithTimeout(timer,std::chrono::milliseconds{Self::migrationTimeout_},id))
                        {
                            channel = std::move(tmp);
                        }
                        set.emplace(id);
                    }
                    catch(const std::exception&)
                    {
                        set.emplace(id);
                    }
                }
            }
            if (channel)
            {
                sharpen::ByteBuffer buf;
                rkv::MigrateRequest request;
                request.BeginKey() = migration.BeginKey();
                request.EndKey() = migration.EndKey();
                request.Serialize().StoreTo(buf);
                rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::MigrateRequest,buf.GetSize())};
                sharpen::AwaitableFuture<bool> future;
                future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
                timer->WaitAsync(future,std::chrono::milliseconds{Self::migrationTimeout_});
                try
                {
                    channel->WriteObjectAsync(header);
                    channel->WriteAsync(buf);
                    if(channel->ReadFixedAsync(reinterpret_cast<char*>(&header),sizeof(header)) != sizeof(header))
                    {
                        return false;    
                    }
                    buf.ExtendTo(header.size_);
                    if (channel->ReadFixedAsync(buf) != buf.GetSize())
                    {
                        return false;   
                    }
                    rkv::MigrateResponse response;
                    response.Unserialize().LoadFrom(buf);
                    for(auto begin = response.Map().begin(),end = response.Map().end(); begin != end; ++begin)
                    {
                        if(this->app_->Exist(begin->first) == sharpen::ExistStatus::Exist)
                        {
                            continue;
                        }
                        this->app_->Put(std::move(begin->first),std::move(begin->second));
                    }
                    if(future.IsPending())
                    {
                        timer->Cancel();
                        future.WaitAsync();
                    }
                    return true;   
                }
                catch(const std::exception &ignore)
                {
                    if(future.IsPending())
                    {
                        timer->Cancel();
                        future.WaitAsync();
                    }
                    static_cast<void>(ignore);
                }
            }
        }
    }
    return false;
}

bool rkv::WorkerServer::ExecuteMigrationAndNotify(const rkv::Migration &migration)
{
    
    using TimeoutPtr = void(*)(sharpen::Future<bool>&,rkv::MasterClient*);
    if (this->ExecuteMigration(migration))
    {
        rkv::CompleteMigrationResult result{rkv::CompleteMigrationResult::NotCommit};
        sharpen::TimerPtr timer = sharpen::MakeTimer(*this->engine_);
        while (result != rkv::CompleteMigrationResult::Appiled)
        {
            std::unique_lock<sharpen::AsyncMutex> lock{this->clientLock_};
            sharpen::AwaitableFuture<bool> future;
            future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
            timer->WaitAsync(future,std::chrono::milliseconds{Self::masterTimeout_});
            try
            {
                result = this->client_->CompleteMigration(migration.GetGroupId(),this->selfId_);
                if(future.IsPending())
                {
                    timer->Cancel();
                    future.WaitAsync();
                }
            }
            catch(const std::exception &e)
            {
                std::fprintf(stderr,"[Error]Cannot complete migration because %s\n",e.what());
                if(future.IsPending())
                {
                    timer->Cancel();
                    future.WaitAsync();
                }
                else
                {
                    this->client_->Reset();
                }
                return false;
            }
        }
        return true;
    }
    return false;
}

void rkv::WorkerServer::OnLeaderRedirect(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::LeaderRedirectRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::LeaderRedirectResponse response;
    response.SetKnowLeader(false);
    if(request.Group().Exist())
    {
        {
            this->groupLock_.LockRead();
            std::unique_lock<sharpen::AsyncReadWriteLock> groupLock{this->groupLock_,std::adopt_lock};
            auto ite = this->groups_.find(request.Group().Get());
            if(ite != this->groups_.end())
            {
                if(ite->second->Raft().KnowLeader())
                {
                    try
                    {
                        response.Endpoint() = ite->second->Raft().GetLeaderId();
                        response.SetKnowLeader(true);
                    }
                    catch(const std::exception& ignore)
                    {
                        static_cast<void>(ignore);
                    }
                }
            }
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::WorkerServer::OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::AppendEntriesRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::AppendEntriesResponse response;
    bool result{false};
    std::uint64_t lastAppiled{0};
    std::uint64_t currentTerm{0};
    if(request.Group().Exist())
    {
        {
            this->groupLock_.LockRead();
            std::unique_lock<sharpen::AsyncReadWriteLock> groupLock{this->groupLock_,std::adopt_lock};
            auto ite = this->groups_.find(request.Group().Get());
            if(ite != this->groups_.end())
            {
                std::printf("[Info]Channel want to append entires to host(%llu)\n",ite->first);
                ite->second->DelayCycle();
                {
                    std::unique_lock<sharpen::AsyncMutex> raftLock{ite->second->GetRaftLock()};
                    result = ite->second->Raft().AppendEntries(request.Logs().begin(),request.Logs().end(),request.LeaderId(),request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
                    lastAppiled = ite->second->Raft().GetLastApplied();
                    currentTerm = ite->second->Raft().GetCurrentTerm();
                }
            }
        }
    }
    response.SetResult(result);
    response.SetAppiledIndex(lastAppiled);
    response.SetTerm(currentTerm);
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::AppendEntriesResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::WorkerServer::OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::VoteRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::VoteResponse response;
    bool result{false};
    std::uint64_t term{0};
    if(request.Group().Exist())
    {
        {
            this->groupLock_.LockRead();
            std::unique_lock<sharpen::AsyncReadWriteLock> groupLock{this->groupLock_,std::adopt_lock};
            auto ite = this->groups_.find(request.Group().Get());
            std::printf("[Info]Channel want to request vote from host(%llu)\n",ite->first);
            if(ite != this->groups_.end())
            {
                std::unique_lock<sharpen::AsyncMutex> raftLock{ite->second->GetRaftLock()};
                result = ite->second->Raft().RequestVote(request.GetTerm(),request.Id(),request.GetLastIndex(),request.GetLastTerm());
                term = ite->second->Raft().GetCurrentTerm();
            }
        }
    }
    response.SetTerm(term);
    response.SetResult(result);
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::VoteResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

sharpen::Optional<std::pair<std::uint64_t,sharpen::ByteBuffer>> rkv::WorkerServer::GetShardId(const sharpen::ByteBuffer &key) const noexcept
{
    sharpen::Optional<std::pair<std::uint64_t,sharpen::ByteBuffer>> result;
    auto ite = this->shardMap_.lower_bound(key);
    if(ite == this->shardMap_.begin())
    {
        ite = this->shardMap_.end();
    }
    else
    {
        ite = sharpen::IteratorBackward(ite,1);
    }
    if(ite != this->shardMap_.end())
    {
        result.Construct(ite->second,ite->first);
    }
    return result;
}

void rkv::WorkerServer::OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetResponse response;
    response.Value() = this->app_->Get(request.Key());
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetRequest,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

std::uint64_t rkv::WorkerServer::ScanKeyCount(std::uint64_t id,const sharpen::ByteBuffer &beginKey) const
{
    std::uint64_t count{0};
    {
        auto scanner{this->app_->GetScanner()};
        scanner.Seek(beginKey);
        if(!scanner.IsEmpty())
        {
            do
            {
                auto tmp{this->GetShardId(scanner.GetCurrentKey())};
                if(tmp.Exist())
                {
                    if(tmp.Get().first != id)
                    {
                        break;
                    }
                    ++count;
                }
            } while (scanner.Next());
        }
    }
    return count;
}

std::uint64_t rkv::WorkerServer::GetKeyCounter(std::uint64_t id,const sharpen::ByteBuffer &beginKey)
{ 
    std::uint64_t count{0};
    auto ite = this->keyCounter_.find(id);
    if(ite == this->keyCounter_.end())
    {
        count = this->ScanKeyCount(id,beginKey);
        this->keyCounter_.emplace(id,count);
    }
    else
    {
        count = ite->second;
    }
    return count;
}

sharpen::Optional<sharpen::ByteBuffer> rkv::WorkerServer::ScanKeys(const sharpen::ByteBuffer &beginKey,std::uint64_t count) const
{
    sharpen::Optional<sharpen::ByteBuffer> key;
    {
        auto scanner{this->app_->GetScanner()};
        scanner.Seek(beginKey);
        if(!scanner.IsEmpty())
        {
            do
            {
                --count;
            } while (scanner.Next());
            if(!count)
            {
                key.Construct(scanner.GetCurrentKey());
            }
        }
    }
    return key;
}

rkv::AppendEntriesResult rkv::WorkerServer::ProposeAppendEntries(rkv::RaftGroup &group,std::uint64_t commitIndex)
{
    std::size_t commitCount{0};
    bool result{false};
    do
    {
        result = group.ProposeAppendEntries();
        if(!result)
        {
            break;
        }
        for (auto memberBegin = group.Raft().Members().begin(),memberEnd = group.Raft().Members().end(); memberBegin != memberEnd; ++memberBegin)
        {
            if (memberBegin->second.GetCurrentIndex() >= commitIndex)
            {
                commitCount += 1;   
            }
        }
    } while (commitCount < group.Raft().MemberMajority());
    if(result)
    {
        group.Raft().SetCommitIndex(commitIndex);
        group.Raft().ApplyLogs(Raft::LostPolicy::Ignore);
        return rkv::AppendEntriesResult::Appiled;
    }
    return rkv::AppendEntriesResult::Commited;
}

void rkv::WorkerServer::DeriveNewShard(std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endKey)
{
    sharpen::TimerPtr timer = sharpen::MakeTimer(sharpen::EventEngine::GetEngine());
    sharpen::AwaitableFuture<bool> future;
    using TimeoutPtr = void(*)(sharpen::Future<bool>&,rkv::MasterClient*);
    try
    {
        std::puts("[Info]Deriving shards");
        {
            std::unique_lock<sharpen::AsyncMutex> lock{this->clientLock_};
            future.SetCallback(std::bind(static_cast<TimeoutPtr>(&Self::CancelClient),std::placeholders::_1,this->client_.get()));
            timer->WaitAsync(future,std::chrono::milliseconds{Self::masterTimeout_});
            this->client_->DeriveShard(source,beginKey,endKey);
        }
        std::puts("[Info]Deriving shards done");
        if(future.IsPending())
        {
            timer->Cancel();
            future.WaitAsync();
        }
    }
    catch(const std::exception &e)
    {
        std::fprintf(stderr,"[Error]Cannot deriving shards because %s\n",e.what());
        if(future.IsPending())
        {
            timer->Cancel();
            future.WaitAsync();
        }
        throw;
    }
}

void rkv::WorkerServer::OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::PutRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::PutResponse response;
    response.SetResult(rkv::MotifyResult::NotCommit);
    {
        this->groupLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->groupLock_,std::adopt_lock};
        sharpen::Optional<std::pair<std::uint64_t,sharpen::ByteBuffer>> shard{this->GetShardId(request.Key())};
        if(shard.Exist())
        {
            auto ite = this->groups_.find(shard.Get().first);
            if(ite != this->groups_.end())
            {
                if(ite->second->Raft().GetRole() == sharpen::RaftRole::Leader)
                {
                    bool hasRoom{true};
                    std::uint64_t counter{this->GetKeyCounter(shard.Get().first,shard.Get().second)};
                    if(counter == Self::maxKeysPerShard_)
                    {
                        bool adjust{true};
                        sharpen::Optional<sharpen::ByteBuffer> midKey{this->ScanKeys(shard.Get().second,Self::maxKeysPerShard_/2)};
                        if(midKey.Exist())
                        {
                            sharpen::Optional<sharpen::ByteBuffer> lastKey{this->ScanKeys(midKey.Get(),Self::maxKeysPerShard_/2)};
                            if(lastKey.Exist())
                            {
                                std::uint64_t midId{this->GetShardId(midKey.Get()).Get().first};
                                std::uint64_t lastId{this->GetShardId(lastKey.Get()).Get().first};
                                if(midId == lastId && midId == shard.Get().first)
                                {
                                    this->DeriveNewShard(shard.Get().first,midKey.Get(),lastKey.Get());
                                    adjust = false;
                                    hasRoom = false;
                                }
                            }
                        }
                        if(adjust)
                        {
                            this->keyCounter_[shard.Get().first] = this->ScanKeyCount(shard.Get().first,shard.Get().second);
                        }
                    }
                    if(hasRoom)
                    {
                        this->keyCounter_[shard.Get().first] += 1;
                        rkv::RaftLog log;
                        log.SetOperation(rkv::RaftLog::Operation::Put);
                        std::uint64_t index{0};
                        std::uint64_t term{0};
                        {
                            ite->second->DelayCycle();
                            std::unique_lock<sharpen::AsyncMutex> raftLock{ite->second->GetRaftLock()};
                            if(ite->second->Raft().GetRole() == sharpen::RaftRole::Leader)
                            {
                                index = ite->second->Raft().GetLastIndex() + 1;
                                term = ite->second->Raft().GetCurrentTerm();
                                rkv::RaftLog log;
                                log.SetOperation(rkv::RaftLog::Operation::Put);
                                log.SetIndex(index);
                                log.SetTerm(term);
                                log.Key() = std::move(request.Key());
                                log.Value() = std::move(request.Value());
                                ite->second->Raft().AppendLog(std::move(log));
                                rkv::AppendEntriesResult result{this->ProposeAppendEntries(*ite->second,index)};
                                switch (result)
                                {
                                case rkv::AppendEntriesResult::NotCommit:
                                    response.SetResult(rkv::MotifyResult::NotCommit);
                                    break;
                                case rkv::AppendEntriesResult::Commited:
                                    response.SetResult(rkv::MotifyResult::Commited);
                                    break;
                                case rkv::AppendEntriesResult::Appiled:
                                    response.SetResult(rkv::MotifyResult::Appiled);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::PutResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::WorkerServer::OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::DeleteRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::DeleteResponse response;
    response.SetResult(rkv::MotifyResult::NotCommit);
    {
        this->groupLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->groupLock_,std::adopt_lock};
        sharpen::Optional<std::pair<std::uint64_t,sharpen::ByteBuffer>> shard{this->GetShardId(request.Key())};
        if(shard.Exist())
        {
            auto ite = this->groups_.find(shard.Get().first);
            if(ite != this->groups_.end())
            {
                ite->second->DelayCycle();
                std::unique_lock<sharpen::AsyncMutex> raftLock{ite->second->GetRaftLock()};
                std::uint64_t index{0};
                std::uint64_t term{0};
                if(ite->second->Raft().GetRole() == sharpen::RaftRole::Leader)
                {
                    rkv::RaftLog log;
                    log.SetOperation(rkv::RaftLog::Operation::Delete);
                    index = ite->second->Raft().GetLastIndex() + 1;
                    term = ite->second->Raft().GetCurrentTerm();
                    log.SetIndex(index);
                    log.SetTerm(term);
                    log.Key() = std::move(request.Key());
                    ite->second->Raft().AppendLog(std::move(log));
                    rkv::AppendEntriesResult result{this->ProposeAppendEntries(*ite->second,index)};
                    switch (result)
                    {
                    case rkv::AppendEntriesResult::NotCommit:
                        response.SetResult(rkv::MotifyResult::NotCommit);
                        break;
                    case rkv::AppendEntriesResult::Commited:
                        response.SetResult(rkv::MotifyResult::Commited);
                        break;
                    case rkv::AppendEntriesResult::Appiled:
                        response.SetResult(rkv::MotifyResult::Appiled);
                        break;
                    }
                }
            }
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeleteResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::WorkerServer::OnMigrate(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::MigrateRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::MigrateResponse response;
    {
        auto scanner{this->app_->GetScanner(request.BeginKey(),request.EndKey())};
        if(!scanner.IsEmpty())
        {
            do
            {
                response.Map().emplace(scanner.GetCurrentKey(),scanner.GetCurrentValue());
            } while (scanner.Next());
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::MigrateRequest,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::WorkerServer::OnClearShard(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::ClearShardRequest request;
    request.Unserialize().LoadFrom(buf);
    std::uint64_t source{request.Migration().GetSource()};
    std::uint64_t destination{request.Migration().GetDestination()};
    std::printf("[Info]Clearing shard %llu becuase part of %llu move to %llu\n",source,source,destination);
    this->CleaupCompletedMigration(request.Migration());
}

void rkv::WorkerServer::OnStartMigration(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::StartMigrationRequest request;
    request.Unserialize().LoadFrom(buf);
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->migrationLock_};
        try
        {
            if(this->ExecuteMigrationAndNotify(request.Migration()))
            { 
                this->FlushShard(nullptr);
            }
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot migrate data because %s\n",e.what());   
        }
    }
}

void rkv::WorkerServer::OnNewChannel(sharpen::NetStreamChannelPtr channel)
{
    try
    {
        sharpen::IpEndPoint ep;
        channel->GetRemoteEndPoint(ep);
        char ip[21] = {};
        ep.GetAddrString(ip,sizeof(ip));
        std::printf("[Info]A new channel %s:%hu connect to host\n",ip,ep.GetPort());
        while (1)
        {
            rkv::MessageHeader header;
            std::size_t sz{channel->ReadObjectAsync(header)};
            if(sz != sizeof(header))
            {
                break;
            }
            std::printf("[Info]Receive a new message from %s:%hu type is %llu size is %llu\n",ip,ep.GetPort(),header.type_,header.size_);
            rkv::MessageType type{rkv::GetMessageType(header)};
            sharpen::ByteBuffer buf{sharpen::IntCast<std::size_t>(header.size_)};
            sz = channel->ReadFixedAsync(buf);
            if(sz != buf.GetSize())
            {
                break;
            }
            switch (type)
            {
            case rkv::MessageType::LeaderRedirectRequest:
                std::printf("[Info]Channel %s:%hu ask who is leader\n",ip,ep.GetPort());
                this->OnLeaderRedirect(*channel,buf);
                break;
            case rkv::MessageType::AppendEntriesRequest:
                std::printf("[Info]Channel %s:%hu want to append entries\n",ip,ep.GetPort());
                this->OnAppendEntries(*channel,buf);
                break;
            case rkv::MessageType::VoteRequest:
                std::printf("[Info]Channel %s:%hu want to request vote\n",ip,ep.GetPort());
                this->OnRequestVote(*channel,buf);
                break;
            case rkv::MessageType::GetRequest:
                std::printf("[Info]Channel %s:%hu want to get a value\n",ip,ep.GetPort());
                this->OnGet(*channel,buf);
                break;
            case rkv::MessageType::PutRequest:
                std::printf("[Info]Channel %s:%hu want to put a key value pair\n",ip,ep.GetPort());
                this->OnPut(*channel,buf);
                break;
            case rkv::MessageType::DeleteReqeust:
                std::printf("[Info]Channel %s:%hu want to delete a key value pair\n",ip,ep.GetPort());
                this->OnDelete(*channel,buf);
                break;
            case rkv::MessageType::MigrateRequest:
                std::printf("[Info]Channel %s:%hu want to migrate data from host\n",ip,ep.GetPort());
                this->OnMigrate(*channel,buf);
                break;
            case rkv::MessageType::ClearShardRequest:
                std::printf("[Info]Channel %s:%hu want to notify host a migration completed\n",ip,ep.GetPort());
                this->OnClearShard(*channel,buf);
                break;
            case rkv::MessageType::StartMigrationRequest:
                std::printf("[Info]Channel %s:%hu want to notify host to start a migration\n",ip,ep.GetPort());
                this->OnStartMigration(*channel,buf);
                break;
            default:
                break;
            }
        }
        std::printf("[Info]A channel %s:%hu disconnect with host\n",ip,ep.GetPort());
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]An error has occurred %s\n",e.what());
    }
}