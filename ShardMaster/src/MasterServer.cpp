#include <rkv/MasterServer.hpp>

#include <sharpen/Quorum.hpp>
#include <sharpen/FileOps.hpp>

#include <rkv/MessageHeader.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/AppendEntriesRequest.hpp>
#include <rkv/AppendEntriesResponse.hpp>
#include <rkv/VoteRequest.hpp>
#include <rkv/VoteResponse.hpp>
#include <rkv/GetShardByKeyRequest.hpp>
#include <rkv/GetShardByKeyResponse.hpp>
#include <rkv/GetShardByWorkerIdRequest.hpp>
#include <rkv/GetShardByWorkerIdResponse.hpp>
#include <rkv/DeriveShardRequest.hpp>
#include <rkv/DeriveShardResponse.hpp>
#include <rkv/GetCompletedMigrationsRequest.hpp>
#include <rkv/GetCompletedMigrationsResponse.hpp>
#include <rkv/GetMigrationsRequest.hpp>
#include <rkv/GetMigrationsResponse.hpp>
#include <rkv/CompleteMigrationRequest.hpp>
#include <rkv/CompleteMigrationResponse.hpp>
#include <rkv/GetShardByIdRequest.hpp>
#include <rkv/GetShardByIdResponse.hpp>
#include <rkv/ClearShardRequest.hpp>
#include <rkv/ClearShardResponse.hpp>
#include <rkv/StartMigrationRequest.hpp>
#include <rkv/StartMigrationResponse.hpp>

sharpen::ByteBuffer rkv::MasterServer::zeroKey_;

rkv::MasterServer::MasterServer(sharpen::EventEngine &engine,const rkv::MasterServerOption &option)
    :sharpen::TcpServer(sharpen::AddressFamily::Ip,option.BindEndpoint(),engine)
    ,app_(nullptr)
    ,group_(nullptr)
    ,random_(std::random_device{}())
    ,distribution_(1,option.GetWorkersSize())
    ,workers_()
    ,shards_(nullptr)
    ,migrations_(nullptr)
    ,completedMigrations_(nullptr)
{
    //make directories
    sharpen::MakeDirectory("./Storage");
    this->app_ = std::make_shared<rkv::KeyValueService>(engine,"./Storage/Masterdb");
    //create master group
    this->group_.reset(new rkv::RaftGroup{engine,option.SelfId(),rkv::RaftStorage{engine,"./Storage/MasterRaft"},this->app_});
    //create shard manager
    this->shards_.reset(new rkv::ShardManger{*this->app_});
    //create migration manager
    this->migrations_.reset(new rkv::MigrationManger{*this->app_});
    //create completed migration manager
    this->completedMigrations_.reset(new rkv::CompletedMigrationManager{*this->app_});
    if(!this->group_ || !this->shards_ || !this->migrations_ || !this->completedMigrations_)
    {
        throw std::bad_alloc();
    }
    //load members
    std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
    for (auto begin = option.MembersBegin(),end = option.MembersEnd(); begin != end; ++begin)
    {
        rkv::RaftMember member{*begin,*this->engine_};
        member.SetCurrentIndex(lastAppiled);
        this->group_->Raft().Members().emplace(*begin,std::move(member));
    }
    //load workers
    this->workers_.reserve(option.GetWorkersSize());
    for (auto begin = option.WorkersBegin(),end = option.WorkersEnd(); begin != end; ++begin)
    {
        this->workers_.emplace_back(*begin);   
    }
    assert(!this->workers_.empty());
    //set callback
    this->group_->SetAppendEntriesCallback(std::bind(&Self::FlushStatusWithLock,this));
}

sharpen::IpEndPoint rkv::MasterServer::GetRandomWorkerId() const noexcept
{
    std::size_t index{this->distribution_(this->random_) - 1};
    return this->workers_[index];
}

void rkv::MasterServer::FlushStatus()
{
    this->shards_->Flush();
    this->migrations_->Flush();
    this->completedMigrations_->Flush();
}

void rkv::MasterServer::OnLeaderRedirect(sharpen::INetStreamChannel &channel)
{
    rkv::LeaderRedirectResponse response;
    response.SetKnowLeader(false);
    if (this->group_->Raft().KnowLeader())
    {
        try
        {
            response.Endpoint() = this->group_->Raft().GetLeaderId();
            response.SetKnowLeader(true);
        }
        catch(const std::exception& ignore)
        {
            static_cast<void>(ignore);
        }
    }
    char buf[sizeof(bool) + sizeof(sharpen::IpEndPoint)];
    std::size_t sz{response.StoreTo(buf,sizeof(buf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(buf,sz);
}

void rkv::MasterServer::OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    this->group_->DelayFollowerCycle();
    rkv::AppendEntriesRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{false};
    std::uint64_t currentTerm{0};
    std::uint64_t lastAppiled{0};
    std::printf("[Info]Channel want to append entries to host term is %llu prev log index is %llu prev log term is %llu commit index is %llu\n",request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
        result = this->group_->Raft().AppendEntries(request.Logs().begin(),request.Logs().end(),request.LeaderId(),request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
        currentTerm = this->group_->Raft().GetCurrentTerm();
        lastAppiled = this->group_->Raft().GetLastApplied();
    }
    if(result)
    {
        std::printf("[Info]Leader append %zu entires to host\n",request.Logs().size());
        {
            this->statusLock_.LockWrite();
            std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
            this->FlushStatus();
        }
    }
    else
    {
        std::printf("[Info]Channel want to append %zu entires to host but failure\n",request.Logs().size());
    }
    rkv::AppendEntriesResponse response;
    response.SetResult(result);
    response.SetTerm(currentTerm);
    response.SetAppiledIndex(lastAppiled);
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::AppendEntriesResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::VoteRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{false};
    std::uint64_t currentTerm{0};
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
        result = this->group_->Raft().RequestVote(request.GetTerm(),request.Id(),request.GetLastIndex(),request.GetLastTerm());
        currentTerm = this->group_->Raft().GetCurrentTerm();
    }
    std::printf("[Info]Candidate want to get vote from host term is %llu last log index is %llu last log term is %llu\n",request.GetTerm(),request.GetLastIndex(),request.GetLastTerm());
    if(result)
    {
        std::puts("[Info]Candidate got vote from host");
    }
    else
    {
        std::puts("[Info]Candidate cannot get vote from host");
    }
    rkv::VoteResponse response;
    response.SetResult(result);
    response.SetTerm(currentTerm);
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::VoteResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnGetShardByKey(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetShardByKeyRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetShardByKeyResponse response;
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
        if(this->shards_->Empty())
        {
            this->statusLock_.UpgradeFromRead();
            //double check
            if(this->shards_->Empty())
            {
                std::uint64_t index{0};
                std::uint64_t term{0};
                {
                    std::unique_lock<sharpen::AsyncMutex> raftLock{this->group_->GetRaftLock()};
                    if(this->group_->Raft().GetRole() == sharpen::RaftRole::Leader)
                    {
                        std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
                        index = this->group_->Raft().GetLastIndex();
                        if(lastAppiled == index)
                        {
                            index += 1;
                            term = this->group_->Raft().GetCurrentTerm();
                        }
                        else
                        {
                            index = 0;
                        }
                    }
                }
                if(index)
                {
                    std::puts("[Info]Try to create first shard");
                    std::vector<sharpen::IpEndPoint> workers;
                    workers.reserve(Self::replicationFactor_);
                    std::size_t count{this->SelectWorkers(std::back_inserter(workers),Self::replicationFactor_)};
                    std::printf("[Info]Select %zu workers\n",workers.size());
                    if(count == Self::replicationFactor_)
                    {
                        rkv::Shard shard;
                        shard.SetId(this->shards_->GetNextIndex());
                        for (auto begin = workers.begin(),end = workers.end(); begin != end; ++begin)
                        {
                            shard.Workers().emplace_back(*begin);
                        }
                        shard.BeginKey() = Self::zeroKey_;
                        std::vector<rkv::RaftLog> logs;
                        logs.reserve(2);
                        index = this->shards_->GenrateEmplaceLogs(std::back_inserter(logs),&shard,&shard + 1,index,term);
                        if(logs.empty())
                        {
                            index -= 1;
                        }
                        rkv::AppendEntriesResult result{this->ProposeAppendEntries(std::make_move_iterator(logs.begin()),std::make_move_iterator(logs.end()),index)};
                        switch (result)
                        {
                        case rkv::AppendEntriesResult::Appiled:
                            std::puts("[Info]New shard has been appiled");
                            break;
                        case rkv::AppendEntriesResult::Commited:
                            std::puts("[Info]New shard has been commited");
                            break;
                        case rkv::AppendEntriesResult::NotCommit:
                            std::puts("[Info]New shard has been commited");
                            break;
                        }
                    }
                }
            }
        }
        const rkv::Shard *shard{this->shards_->GetShardPtr(request.Key())};
        if(shard)
        {
            //notify migration
            response.Shard().Construct(*shard);
            std::vector<rkv::Migration> migrations;
            this->migrations_->GetMigrations(std::back_inserter(migrations),shard->BeginKey());
            lock.unlock();
            for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
            {
                this->NotifyStartMigration(begin->Destination(),*begin);
            }
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByKeyResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnGetShardByWorkerId(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetShardByWorkerIdRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetShardByWorkerIdResponse response;
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
        if(this->shards_->Empty())
        {
            this->statusLock_.UpgradeFromRead();
            if(this->shards_->Empty())
            {
                std::uint64_t index{0};
                std::uint64_t term{0};
                {
                    std::unique_lock<sharpen::AsyncMutex> raftLock{this->group_->GetRaftLock()};
                    if(this->group_->Raft().GetRole() == sharpen::RaftRole::Leader)
                    {
                        std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
                        index = this->group_->Raft().GetLastIndex();
                        if(lastAppiled == index)
                        {
                            index += 1;
                            term = this->group_->Raft().GetCurrentTerm();
                        }
                        else
                        {
                            index = 0;
                        }
                    }
                }
                if(index)
                {
                    std::puts("[Info]Try to create first shard");
                    std::vector<sharpen::IpEndPoint> workers;
                    workers.reserve(Self::replicationFactor_);
                    std::size_t count{this->SelectWorkers(std::back_inserter(workers),Self::replicationFactor_)};
                    std::printf("[Info]Select %zu workers\n",workers.size());
                    if(count == Self::replicationFactor_)
                    {
                        rkv::Shard shard;
                        shard.SetId(this->shards_->GetNextIndex());
                        for (auto begin = workers.begin(),end = workers.end(); begin != end; ++begin)
                        {
                            shard.Workers().emplace_back(*begin);
                        }
                        shard.BeginKey() = Self::zeroKey_;
                        std::vector<rkv::RaftLog> logs;
                        logs.reserve(2);
                        index = this->shards_->GenrateEmplaceLogs(std::back_inserter(logs),&shard,&shard + 1,index,term);
                        rkv::AppendEntriesResult result{this->ProposeAppendEntries(std::make_move_iterator(logs.begin()),std::make_move_iterator(logs.end()),index)};
                        switch (result)
                        {
                        case rkv::AppendEntriesResult::Appiled:
                            std::puts("[Info]New shard has been appiled");
                            break;
                        case rkv::AppendEntriesResult::Commited:
                            std::puts("[Info]New shard has been commited");
                            break;
                        case rkv::AppendEntriesResult::NotCommit:
                            std::puts("[Info]New shard has been commited");
                            break;
                        }
                    }
                }
            }
        }
        this->shards_->GetShards(response.GetShardsInserter(),request.WorkerId());
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByWorkerIdResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::NotifyStartMigration(const sharpen::IpEndPoint &id,const rkv::Migration &migration)
{
    try
    {
        sharpen::TimerPtr timer = sharpen::MakeTimer(*this->engine_);
        sharpen::IpEndPoint ep{0,0};
        sharpen::NetStreamChannelPtr channel = sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip);
        channel->Bind(ep);
        channel->Register(*this->engine_);
        bool connected{channel->ConnectWithTimeout(timer,std::chrono::milliseconds{static_cast<std::int64_t>(Self::notifyTimeout_)},id)};
        if (connected)
        {
            sharpen::ByteBuffer buf;
            rkv::StartMigrationRequest request;
            request.Migration() = migration;
            request.Serialize().StoreTo(buf);
            rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::StartMigrationRequest,buf.GetSize())};
            channel->WriteObjectAsync(header);
            channel->WriteAsync(buf);
        }
    }
    catch(const std::exception &ignore)
    {
        static_cast<void>(ignore);
    }
}

void rkv::MasterServer::OnDerviveShard(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::DeriveShardRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::DeriveShardResponse response;
    response.SetResult(rkv::DeriveResult::Appiled);
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> statusLock{this->statusLock_,std::adopt_lock};
        //double check
        if(!this->shards_->Contain(request.BeginKey()))
        {
            if(!this->migrations_->Contain(request.BeginKey()))
            {
                this->statusLock_.UpgradeFromRead();
                if(!this->shards_->Contain(request.BeginKey()) && !this->migrations_->Contain(request.BeginKey()))
                {
                    std::uint64_t index{0};
                    std::uint64_t term{0};
                    {
                        std::unique_lock<sharpen::AsyncMutex> raftLock{this->group_->GetRaftLock()};
                        if(this->group_->Raft().GetRole() == sharpen::RaftRole::Leader)
                        {
                            std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
                            index = this->group_->Raft().GetLastIndex();
                            if(lastAppiled == index)
                            {
                                index += 1;
                                term = this->group_->Raft().GetCurrentTerm();
                            }
                            else
                            {
                                index = 0;
                                response.SetResult(rkv::DeriveResult::NotCommit);
                            }
                        }
                        else
                        {
                            response.SetResult(rkv::DeriveResult::NotCommit);
                        }
                    }
                    if(index)
                    {
                        std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
                        std::vector<sharpen::IpEndPoint> workers;
                        workers.reserve(Self::replicationFactor_);
                        std::size_t count{this->SelectWorkers(std::back_inserter(workers),Self::replicationFactor_)};
                        if(count == Self::replicationFactor_)
                        {
                            std::vector<rkv::Migration> migrations;
                            migrations.reserve(count);
                            this->GenrateMigrations(std::back_inserter(migrations),workers.begin(),workers.end(),this->migrations_->GetNextMirgationId(),this->migrations_->GetNextGroupId(),request.GetSource(),request.BeginKey(),request.EndKey());
                            std::vector<rkv::RaftLog> logs;
                            logs.reserve(count);
                            index = this->migrations_->GenrateEmplaceLogs(std::back_inserter(logs),migrations.begin(),migrations.end(),index,term);
                            rkv::AppendEntriesResult result{this->ProposeAppendEntries(std::make_move_iterator(logs.begin()),std::make_move_iterator(logs.end()),index)};
                            switch (result)
                            {
                            case rkv::AppendEntriesResult::Appiled:
                                response.SetResult(rkv::DeriveResult::Appiled);
                                break;
                            case rkv::AppendEntriesResult::Commited:
                                response.SetResult(rkv::DeriveResult::Commited);
                                break;
                            case rkv::AppendEntriesResult::NotCommit:
                                response.SetResult(rkv::DeriveResult::NotCommit);
                                break;
                            }
                        }
                        else
                        {
                            response.SetResult(rkv::DeriveResult::LeakOfWorker);
                        }
                    }
                }
            }
        }
        //notify
        {
            std::vector<rkv::Migration> migrations;
            this->migrations_->GetMigrations(std::back_inserter(migrations),request.BeginKey());
            statusLock.unlock();
            std::puts("[Info]Norify workers to migrate data");
            for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
            {
                this->NotifyStartMigration(begin->Destination(),*begin);
            }
        }
    }
    char resBuf[sizeof(response)];
    std::size_t size{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeriveShardReponse,size)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,size);
}

void rkv::MasterServer::OnGetCompletedMigrations(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetCompletedMigrationsRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetCompletedMigrationsResponse response;
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
        this->completedMigrations_->GetCompletedMigrations(response.GetMigrationsInserter(),request.GetSource(),request.GetBeginId());
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetCompletedMigrationsResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnGetMigrations(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetMigrationsRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetMigrationsResponse response;
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
        this->migrations_->GetMigrations(response.GetMigrationsInserter(),request.Destination());
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetMigrationsResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::NotifyMigrationCompleted(const sharpen::IpEndPoint &id,const rkv::CompletedMigration &migration) const noexcept
{
    try
    {
        sharpen::TimerPtr timer = sharpen::MakeTimer(*this->engine_);
        sharpen::IpEndPoint ep{0,0};
        sharpen::NetStreamChannelPtr channel = sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip);
        channel->Bind(ep);
        channel->Register(*this->engine_);
        bool result{channel->ConnectWithTimeout(timer,std::chrono::milliseconds{static_cast<std::int64_t>(Self::notifyTimeout_)},id)};
        if (result)
        {
            sharpen::ByteBuffer buf;
            rkv::ClearShardRequest request;
            request.Migration() = migration;
            request.Serialize().StoreTo(buf);
            rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::ClearShardRequest,buf.GetSize())};
            channel->WriteObjectAsync(header);
            channel->WriteAsync(buf);
        }
    }
    catch(const std::exception &ignore)
    {
        static_cast<void>(ignore);
    }
}

void rkv::MasterServer::OnCompleteMigration(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::CompleteMigrationRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::CompleteMigrationResponse response;
    response.SetResult(rkv::CompleteMigrationResult::Appiled);
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> statusLock{this->statusLock_,std::adopt_lock};
        std::vector<rkv::Migration> migrations;
        this->migrations_->GetMigrations(std::back_inserter(migrations),request.GetGroupId());
        //double check
        if(!migrations.empty())
        {
            this->statusLock_.UpgradeFromRead();
            migrations.clear();
            this->migrations_->GetMigrations(std::back_inserter(migrations),request.GetGroupId());
            if (!migrations.empty())
            {
                std::uint64_t index{0};
                std::uint64_t term{0};
                {
                    std::unique_lock<sharpen::AsyncMutex> raftLock{this->group_->GetRaftLock()};
                    if(this->group_->Raft().GetRole() == sharpen::RaftRole::Leader)
                    {
                        std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
                        index = this->group_->Raft().GetLastIndex();
                        if(lastAppiled == index)
                        {
                            index += 1;
                            term = this->group_->Raft().GetCurrentTerm();
                        }
                        else
                        {
                            index = 0;
                            response.SetResult(rkv::CompleteMigrationResult::NotCommit);
                        }
                    }
                    else
                    {
                        response.SetResult(rkv::CompleteMigrationResult::NotCommit);
                    }
                }
                if (index)
                {
                    std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
                    std::vector<rkv::RaftLog> logs;
                    logs.reserve(Self::reverseLogsCount_);
                    std::size_t migrationsCount{migrations.size()};
                    sharpen::Optional<rkv::CompletedMigration> notifyMigration;
                    const rkv::Shard *notifyShard{nullptr};
                    std::uint64_t indexOffset{0};
                    //install shard
                    if(migrationsCount == Self::replicationFactor_)
                    {
                        if (!this->shards_->Contain(migrations[0].BeginKey()))
                        {
                            std::puts("[Info]A new shard has been created");
                            rkv::Shard shard;
                            for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
                            {
                                shard.Workers().emplace_back(begin->Destination());
                            }
                            shard.SetId(this->shards_->GetNextIndex());
                            shard.BeginKey() = std::move(migrations[0].BeginKey());
                            index = this->shards_->GenrateEmplaceLogs(std::back_inserter(logs),&shard,&shard + 1,index,term);
                            indexOffset = 1;
                        }
                    }
                    else if(migrationsCount == 1)
                    {
                        if(migrations[0].Destination() == request.Id())
                        {
                            const rkv::Shard *shard{this->shards_->FindShardPtr(migrations[0].BeginKey())};
                            if(shard)
                            {
                                rkv::CompletedMigration completedMigration;
                                completedMigration.SetId(this->completedMigrations_->GetNextId());
                                completedMigration.SetDestination(shard->GetId());
                                completedMigration.SetSource(migrations[0].GetSource());
                                completedMigration.BeginKey() = std::move(migrations[0].BeginKey());
                                completedMigration.EndKey() = std::move(migrations[0].EndKey());
                                index = this->completedMigrations_->GenrateEmplaceLogs(std::back_inserter(logs),&completedMigration,&completedMigration + 1,index,term);
                                notifyMigration.Construct(std::move(completedMigration));
                                notifyShard = shard;
                                indexOffset = 1;
                            }
                        }
                    }
                    for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
                    {
                        if(begin->Destination() == request.Id())
                        {
                            std::uint64_t id{begin->GetId()};
                            index = this->migrations_->GenrateRemoveLogs(std::back_inserter(logs),&id,&id + 1,index + indexOffset,term);
                            break;
                        }
                    }
                    rkv::AppendEntriesResult result{this->ProposeAppendEntries(std::make_move_iterator(logs.begin()),std::make_move_iterator(logs.end()),index)};
                    switch (result)
                    {
                    case rkv::AppendEntriesResult::Appiled:
                        response.SetResult(rkv::CompleteMigrationResult::Appiled);
                        break;
                    case rkv::AppendEntriesResult::Commited:
                        response.SetResult(rkv::CompleteMigrationResult::Commited);
                        break;
                    case rkv::AppendEntriesResult::NotCommit:
                        response.SetResult(rkv::CompleteMigrationResult::NotCommit);
                        break;
                    }
                    //notify migration completed
                    if(notifyMigration.Exist() && notifyShard)
                    {
                        std::vector<sharpen::IpEndPoint> workers{notifyShard->Workers()};
                        const rkv::Shard *tmp{this->shards_->GetShardPtr(notifyMigration.Get().GetDestination())};
                        if(tmp)
                        {
                            rkv::Shard shard{*tmp};
                            statusLock.unlock();
                            for (auto begin = workers.begin(),end = workers.end(); begin != end; ++begin)
                            {
                                bool skip{false};
                                for(auto workerBegin = shard.Workers().begin(),workerEnd = shard.Workers().end(); workerBegin != workerEnd; ++workerBegin)
                                {
                                    if(*begin == *workerBegin)
                                    {
                                        skip = true;
                                        break;
                                    }
                                }
                                if(!skip)
                                {
                                    this->NotifyMigrationCompleted(*begin,notifyMigration.Get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    char resBuf[sizeof(response)];
    std::size_t size{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::CompleteMigrationResponse,size)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,size);
}

void rkv::MasterServer::OnGetShardById(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetShardByIdRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetShardByIdResponse response;
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
        const rkv::Shard *shard{this->shards_->GetShardPtr(request.GetId())};
        if(shard)
        {
            response.Shard().Construct(*shard);
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByIdResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnNewChannel(sharpen::NetStreamChannelPtr channel)
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
                this->OnLeaderRedirect(*channel);
                break;
            case rkv::MessageType::AppendEntriesRequest:
                std::printf("[Info]Channel %s:%hu want to append entires\n",ip,ep.GetPort());
                this->OnAppendEntries(*channel,buf);
                break;
            case rkv::MessageType::VoteRequest:
                std::printf("[Info]Channel %s:%hu want to request a vote\n",ip,ep.GetPort());
                this->OnRequestVote(*channel,buf);
                break;
            case rkv::MessageType::GetShardByKeyRequest:
                std::printf("[Info]Channel %s:%hu want to get a shard\n",ip,ep.GetPort());
                this->OnGetShardByKey(*channel,buf);
                break;
            case rkv::MessageType::GetShardByWorkerIdRequest:
                std::printf("[Info]Channel %s:%hu want to get a shard\n",ip,ep.GetPort());
                this->OnGetShardByWorkerId(*channel,buf);
                break;
            case rkv::MessageType::CompleteMigrationRequest:
                std::printf("[Info]Channel %s:%hu want to complete a migration\n",ip,ep.GetPort());
                this->OnCompleteMigration(*channel,buf);
                break;
            case rkv::MessageType::GetCompletedMigrationsRequest:
                std::printf("[Info]Channel %s:%hu want to get completed migrations\n",ip,ep.GetPort());
                this->OnGetCompletedMigrations(*channel,buf);
                break;
            case rkv::MessageType::GetMigrationsRequest:
                std::printf("[Info]Channel %s:%hu want to get migrations\n",ip,ep.GetPort());
                this->OnGetMigrations(*channel,buf);
                break;
            case rkv::MessageType::DeriveShardRequest:
                std::printf("[Info]Channel %s:%hu want to derive shard\n",ip,ep.GetPort());
                this->OnDerviveShard(*channel,buf);
                break;
            case rkv::MessageType::GetShardByIdRequest:
                std::printf("[Info]Channel %s:%hu want to get a shard\n",ip,ep.GetPort());
                this->OnGetShardById(*channel,buf);
                break;
            default:
                std::printf("[Info]Channel %s:%hu send a unknown request\n",ip,ep.GetPort());
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