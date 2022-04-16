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
#include <rkv/GetShardByIdRequest.hpp>
#include <rkv/GetShardByIdResponse.hpp>
#include <rkv/DeriveShardRequest.hpp>
#include <rkv/DeriveShardResponse.hpp>

rkv::MasterServer::MasterServer(sharpen::EventEngine &engine,const rkv::MasterServerOption &option)
    :sharpen::TcpServer(sharpen::AddressFamily::Ip,option.SelfId(),engine)
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
    //set default append index of members
    std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
    for (auto begin = option.MembersBegin(),end = option.MembersEnd(); begin != end; ++begin)
    {
        rkv::RaftMember member{*begin,engine};
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
}

sharpen::IpEndPoint rkv::MasterServer::GetRandomWorkerId() const noexcept
{
    std::size_t index{this->distribution_(this->random_) - 1};
    return this->workers_[index];
}

void rkv::MasterServer::OnLeaderRedirect(sharpen::INetStreamChannel &channel) const
{
    rkv::LeaderRedirectResponse response;
    response.SetKnowLeader(this->group_->Raft().KnowLeader());
    if(response.KnowLeader())
    {
        response.Endpoint() = this->group_->Raft().GetLeaderId();
    }
    char buf[sizeof(bool) + sizeof(sharpen::IpEndPoint)];
    std::size_t sz{response.StoreTo(buf,sizeof(buf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(buf,sz);
}

void rkv::MasterServer::OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    this->group_->DelayCycle();
    rkv::AppendEntriesRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{false};
    std::uint64_t currentTerm{0};
    std::uint64_t lastAppiled{0};
    std::printf("[Info]Channel want to append entries to host term is %llu prev log index is %llu prev log term is %llu commit index is %llu\n",request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
    {
        result = this->group_->Raft().AppendEntries(request.Logs().begin(),request.Logs().end(),request.LeaderId(),request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
        currentTerm = this->group_->Raft().GetCurrentTerm();
        lastAppiled = this->group_->Raft().GetLastApplied();
    }
    if(result)
    {
        std::printf("[Info]Leader append %zu entires to host\n",request.Logs().size());
        std::puts("[Info]Flush shards");
        {
            this->statusLock_.LockWrite();
            std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
            this->shards_->Flush();
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
        const rkv::Shard *shard{this->shards_->GetShardPtr(request.Key())};
        if(shard)
        {
            response.Shard().Construct(*shard);
        }
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByKeyResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnGetShardById(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::GetShardByIdRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetShardByIdResponse response;
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
        this->shards_->GetShards(response.ShardsInserter(),request.Id());
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByIdResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::MasterServer::OnDerviveShard(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    if(this->group_->Raft().GetRole() != sharpen::RaftRole::Leader)
    {
        rkv::DeriveShardResponse response;
        response.SetResult(rkv::DeriveResult::NotCommit);
        char resBuf[sizeof(response)];
        std::size_t size{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
        rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeriveShardReponse,size)};
        channel.WriteObjectAsync(header);
        channel.WriteAsync(resBuf,size);
        return;
    }
    rkv::DeriveShardRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::DeriveShardResponse response;
    response.SetResult(rkv::DeriveResult::NotCommit);
    {
        this->statusLock_.LockRead();
        std::unique_lock<sharpen::AsyncReadWriteLock> statusLock{this->statusLock_,std::adopt_lock};
        const rkv::Shard *shard{this->shards_->FindShardPtr(request.BeginKey())};
        if(shard)
        {
            statusLock.unlock();
            response.SetResult(DeriveResult::Appiled);
        }
        else
        {
            if(this->migrations_->Contain(request.BeginKey()))
            {
                statusLock.unlock();
                response.SetResult(DeriveResult::Appiled);
            }
            else
            {
                this->statusLock_.UpgradeFromRead();
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
                    std::uint64_t index{this->group_->Raft().GetLastIndex() + 1};
                    std::uint64_t term{this->group_->Raft().GetCurrentTerm()};
                    index = this->migrations_->GenrateEmplaceLogs(std::back_inserter(logs),migrations.begin(),migrations.end(),index,term);
                    rkv::AppendEntriesResult result{this->AppendEntries(std::make_move_iterator(logs.begin()),std::make_move_iterator(logs.end()),index)};
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
    char resBuf[sizeof(response)];
    std::size_t size{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeriveShardReponse,size)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,size);
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
            if(type == rkv::MessageType::LeaderRedirectRequest)
            {
                std::printf("[Info]Channel %s:%hu ask who is leader\n",ip,ep.GetPort());
                this->OnLeaderRedirect(*channel);
                continue;
            }
            sharpen::ByteBuffer buf{sharpen::IntCast<std::size_t>(header.size_)};
            sz = channel->ReadFixedAsync(buf);
            if(sz != buf.GetSize())
            {
                break;
            }
            switch (type)
            {
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