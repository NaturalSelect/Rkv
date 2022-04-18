#include <rkv/WorkerServer.hpp>

#include <sharpen/Quorum.hpp>
#include <sharpen/FileOps.hpp>

#include <rkv/AppendEntriesRequest.hpp>
#include <rkv/AppendEntriesResponse.hpp>
#include <rkv/VoteRequest.hpp>
#include <rkv/VoteResponse.hpp>

rkv::WorkerServer::WorkerServer(sharpen::EventEngine &engine,const rkv::WorkerServerOption &option)
    :TcpServer(sharpen::AddressFamily::Ip,option.BindEndpoint(),engine)
    ,selfId_(option.SelfId())
    ,app_(nullptr)
    ,clientLock_()
    ,client_(nullptr)
    ,groups_()
    ,keyCounter_()
    ,counterFile_(nullptr)
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
    this->client_->GetMigrations(std::back_inserter(migrations),this->selfId_);
    for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
    {
        sharpen::Optional<rkv::Shard> shard{this->client_->GetShard(begin->GetSource())};
        if(shard.Exist())
        {
            bool skip{false};
            for(auto workerBegin = shard.Get().Workers().begin(),workerEnd = shard.Get().Workers().end(); workerBegin != workerEnd; ++workerBegin)
            {
                if(*workerBegin == this->selfId_)
                {
                    skip = true;
                    break;
                }   
            }
            if(!skip)
            {
                this->ExecuteMigrationAndNotify(*begin);
            }
        }
    }
    //TODO learn item
    //get shards
    std::vector<rkv::Shard> shards;
    this->client_->GetShard(std::back_inserter(shards),this->selfId_);
    for (auto begin = shards.begin(),end = shards.end(); begin != end; ++begin)
    {
        std::unique_ptr<rkv::RaftGroup> group{new rkv::RaftGroup{engine,this->selfId_,rkv::RaftStorage{engine,Self::FormatStorageName(begin->GetId())},this->app_}};
        if(!group)
        {
            throw std::bad_alloc();
        }
        std::uint64_t lastAppiled{group->Raft().GetLastApplied()};
        for(auto workerBegin = begin->Workers().begin(),workerEnd = begin->Workers().end(); workerBegin != workerEnd; ++workerBegin)
        {
            if(*workerBegin != this->selfId_)
            {
                rkv::RaftMember member{*workerBegin,engine};
                member.SetCurrentIndex(lastAppiled);
                group->Raft().Members().emplace(*workerBegin,std::move(member));
            }
        }
        this->groups_.emplace(begin->GetId(),std::move(group));
    }
    //get completed migrations
    this->counterFile_ = sharpen::MakeFileChannel("./Storage/CompletedCounter.bin",sharpen::FileAccessModel::All,sharpen::FileOpenModel::CreateOrOpen);
    this->counterFile_->Register(engine);
    std::uint64_t size{this->counterFile_->GetFileSize()};
    if(!size)
    {
        this->counterFile_->ZeroMemoryAsync(sizeof(size));
    }
    sharpen::FileMemory counterMemory{this->counterFile_->MapMemory(sizeof(size),0)};
    std::uint64_t *counter{reinterpret_cast<std::uint64_t*>(counterMemory.Get())};
    std::vector<rkv::CompletedMigration> completedMigations;
    std::uint64_t maxId{0};
    for (auto begin = completedMigations.begin(),end = completedMigations.end(); begin != end; ++begin)
    {
        auto ite = this->groups_.find(begin->GetDestination());
        if(ite == this->groups_.end())
        {
            this->CleaupCompletedMigration(*begin);
        }
    }
    *counter = maxId;
    counterMemory.FlushAndWait();
}

std::string rkv::WorkerServer::FormatStorageName(std::uint64_t id)
{
    std::string str{"./Storage/Raft/"};
    str += std::to_string(id);
    return str;
}

void rkv::WorkerServer::CleaupCompletedMigration(const rkv::CompletedMigration &migration)
{
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
    for(auto begin = keys.begin(),end = keys.end(); begin != end; ++begin)
    {
        this->app_->Delete(*begin);
    }   
}

void rkv::WorkerServer::ExecuteMigration(const rkv::Migration &migration)
{
    sharpen::Optional<rkv::Shard> shard;
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->clientLock_};
        shard = this->client_->GetShard(migration.GetSource());
    }
    if(shard.Exist())
    {
        if(!shard.Get().Workers().empty())
        {
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
                        tmp->ConnectAsync(id);
                        channel = std::move(tmp);
                    }
                    catch(const std::exception&)
                    {
                        set.emplace(id);
                    }
                }
                if (channel)
                {
                    //TODO migrate data
                }
            }
        }
    }
}

void rkv::WorkerServer::ExecuteMigrationAndNotify(const rkv::Migration &migration)
{
    this->ExecuteMigration(migration);
    rkv::CompleteMigrationResult result{rkv::CompleteMigrationResult::NotCommit};
    while (result != rkv::CompleteMigrationResult::Appiled)
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->clientLock_};
        result = this->client_->CompleteMigration(migration.GetGroupId(),this->selfId_);
    }
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
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::VoteRequest,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::WorkerServer::OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    //TODO
}

void rkv::WorkerServer::OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    //TODO
}

void rkv::WorkerServer::OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    //TODO
}

void rkv::WorkerServer::OnMigrate(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    //TOOD
}

void rkv::WorkerServer::OnClearShard(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    //TODO
}

void rkv::WorkerServer::OnStartMigration(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    //TODO
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