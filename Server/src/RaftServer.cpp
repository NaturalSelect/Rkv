#include <rkv/RaftServer.hpp>

#include <sharpen/Quorum.hpp>
#include <sharpen/FileOps.hpp>

#include <rkv/MessageHeader.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/AppendEntiresRequest.hpp>
#include <rkv/AppendEntiresResponse.hpp>
#include <rkv/VoteRequest.hpp>
#include <rkv/VoteResponse.hpp>

rkv::RaftServer::RaftServer(sharpen::EventEngine &engine,const rkv::RaftServerOption &option)
    :sharpen::TcpServer(sharpen::AddressFamily::Ip,option.SelfId(),engine)
    ,random_(std::random_device{}())
    ,distribution_(Self::followerMinWaitMs,Self::followerMaxWaitMs)
    ,app_(nullptr)
    ,raft_(nullptr)
    ,voteLock_()
    ,proposeTimer_(sharpen::MakeTimer(engine))
    ,leaderLoop_(engine,sharpen::MakeTimer(engine),std::chrono::milliseconds{Self::leaderMaxWaitMs},std::bind(&Self::LeaderLoop,this))
    ,followerLoop_(engine,sharpen::MakeTimer(engine),std::bind(&Self::FollowerLoop,this),std::bind(&Self::GenerateWaitTime,this))
{
    //make directories
    sharpen::MakeDirectory("./Storage");
    sharpen::MakeDirectory("./Storage");
    this->app_ = std::make_shared<rkv::KeyValueService>(engine,"./Storage/Kvdb");
    this->raft_.reset(new Raft{option.SelfId(),rkv::RaftStorage{engine,"./Storage/Raft"},this->app_});
    std::uint64_t lastAppiled{this->raft_->GetLastApplied()};
    for (auto begin = option.MembersBegin(),end = option.MembersEnd(); begin != end; ++begin)
    {
        rkv::RaftMember member{*begin,engine};
        member.SetCurrentIndex(lastAppiled);
        this->raft_->Members().emplace(*begin,std::move(member));
    }
}

std::chrono::milliseconds rkv::RaftServer::GenerateWaitTime() const
{
    std::uint32_t waitMs{this->distribution_(this->random_)};
    std::printf("[Info]Need to wait %u ms\n",waitMs);
    return std::chrono::milliseconds{waitMs};
}

void rkv::RaftServer::RequestVoteCallback() noexcept
{
    std::unique_lock<sharpen::SpinLock> lock{this->voteLock_};
    this->raft_->ReactVote(1);
}

sharpen::TimerLoop::LoopStatus rkv::RaftServer::FollowerLoop()
{
    if(this->raft_->GetRole() == sharpen::RaftRole::Leader)
    {
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    rkv::VoteProposal proposal;
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->raftLock_};
        this->raft_->RaiseElection();
        proposal.SetTerm(this->raft_->GetCurrentTerm());
        proposal.SetLastIndex(this->raft_->GetLastIndex());
        proposal.SetLastTerm(this->raft_->GetLastTerm());
    }
    proposal.Id() = this->raft_->GetSelfId();
    proposal.Callback() = std::bind(&Self::RequestVoteCallback,this);
    sharpen::AwaitableFuture<bool> continuation;
    sharpen::AwaitableFuture<void> finish;
    std::puts("[Info]Try to request vote from other members");
    sharpen::Quorum::TimeLimitedProposeAsync(this->proposeTimer_,std::chrono::milliseconds{Self::electionMaxWaitMs},this->raft_->Members().begin(),this->raft_->Members().end(),proposal,continuation,finish);
    continuation.Await();
    bool result{false};
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->raftLock_};
        result = this->raft_->StopElection();
        if(!result)
        {
            this->raft_->ReactNewTerm(proposal.GetMaxTerm());
        }
    }
    if(result)
    {
        std::printf("[Info]Become leader of term %llu\n",this->raft_->GetCurrentTerm());
        this->leaderLoop_.Start();
        finish.WaitAsync();
        std::puts("[Info]Follower loop terminate");
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    std::puts("[Info]Election failure");
    finish.WaitAsync();
    std::puts("[Info]Follower loop continue");
    return sharpen::TimerLoop::LoopStatus::Continue;
}

bool rkv::RaftServer::ProposeAppendEntires()
{
    rkv::LogProposal proposal{const_cast<const Raft*>(this->raft_.get())->PersistenceStorage()};
    proposal.SetCommitIndex(this->raft_->GetCommitIndex());
    proposal.SetTerm(this->raft_->GetCurrentTerm());
    proposal.Id() = this->raft_->GetSelfId();
    sharpen::AwaitableFuture<bool> continuation;
    sharpen::AwaitableFuture<void> finish;
    std::puts("[Info]Try to append entries to other members");
    sharpen::Quorum::TimeLimitedProposeAsync(this->proposeTimer_,std::chrono::milliseconds{Self::appendEntriesMaxWaitMs},this->raft_->Members().begin(),this->raft_->Members().end(),proposal,continuation,finish);
    bool result{continuation.Await()};
    if(!result)
    {
        std::fputs("[Error]Append entires to other members failure\n",stderr);
    }
    else
    {
        std::puts("[Info]Append entires to other members success");
    }
    finish.WaitAsync();
    this->raft_->ReactNewTerm(proposal.GetMaxTerm());
    return result;
}

sharpen::TimerLoop::LoopStatus rkv::RaftServer::LeaderLoop()
{
    if(this->raft_->GetRole() != sharpen::RaftRole::Leader)
    {
        this->followerLoop_.Start();
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    if(!this->raftLock_.TryLock())
    {
        return sharpen::TimerLoop::LoopStatus::Continue;
    }
    bool result{false};
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->raftLock_,std::adopt_lock};
        result = this->ProposeAppendEntires();
        if(result)
        {
            std::uint64_t index{this->raft_->GetLastIndex()};
            this->raft_->SetCommitIndex(index);
            this->raft_->ApplyLogs(Raft::LostPolicy::Ignore);
        }
    }
    return sharpen::TimerLoop::LoopStatus::Continue;
}

void rkv::RaftServer::OnLeaderRedirect(sharpen::INetStreamChannel &channel) const
{
    rkv::LeaderRedirectResponse response;
    response.SetKnowLeader(this->raft_->KnowLeader());
    if(response.KnowLeader())
    {
        response.Endpoint() = this->raft_->GetLeaderId();
    }
    char buf[sizeof(bool) + sizeof(sharpen::IpEndPoint)];
    std::size_t sz{response.StoreTo(buf,sizeof(buf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(buf,sz);
}

void rkv::RaftServer::OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const
{
    rkv::GetRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetResponse response;
    sharpen::Optional<sharpen::ByteBuffer> val{this->app_->TryGet(request.Key())};
    if(!val.Exist())
    {
        std::fputs("[Info]Try to get a not-existed key ",stdout);
        for (std::size_t i = 0; i != request.Key().GetSize(); ++i)
        {
            std::putchar(request.Key()[i]);
        }
        std::putchar('\n');
    }
    else
    {
        response.Value() = std::move(val.Get());
        std::fputs("[Info]Try to get a key value pair and key is ",stdout);
        for (std::size_t i = 0; i != request.Key().GetSize(); ++i)
        {
            std::putchar(request.Key()[i]);
        }
        std::fputs(" value is ",stdout);
        for (std::size_t i = 0; i != response.Value().GetSize(); ++i)
        {
            std::putchar(response.Value()[i]);
        }
        std::putchar('\n');
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::RaftServer::OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{

}

void rkv::RaftServer::OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{

}

void rkv::RaftServer::OnAppendEntires(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    this->followerLoop_.Cancel();
    rkv::AppendEntiresRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{this->raft_->AppendEntries(request.Logs().begin(),request.Logs().end(),request.LeaderId(),request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex())};
    std::printf("[Info]Channel want to append entries to host term is %llu\n",request.GetLeaderTerm());
    if(result)
    {
        std::printf("[Info]Leader append %zu entires to host\n",request.Logs().size());
    }
    else
    {
        std::printf("[Info]Channel want to append %zu entires to host but failure\n",request.Logs().size());
    }
    rkv::AppendEntiresResponse response;
    response.SetResult(result);
    response.SetTerm(this->raft_->GetCurrentTerm());
    response.SetAppiledIndex(this->raft_->GetLastApplied());
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::AppendEntiresResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::RaftServer::OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::VoteRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{this->raft_->RequestVote(request.GetTerm(),request.Id(),request.GetLastIndex(),request.GetLastTerm())};
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
    response.SetTerm(this->raft_->GetCurrentTerm());
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::VoteResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::RaftServer::OnNewChannel(sharpen::NetStreamChannelPtr channel)
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
            case rkv::MessageType::AppendEntiresRequest:
                std::printf("[Info]Channel %s:%hu want to append entires\n",ip,ep.GetPort());
                this->OnAppendEntires(*channel,buf);
                break;
            case rkv::MessageType::VoteRequest:
                std::printf("[Info]Channel %s:%hu want to request a vote\n",ip,ep.GetPort());
                this->OnRequestVote(*channel,buf);
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