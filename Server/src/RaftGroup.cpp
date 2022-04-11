#include <rkv/RaftGroup.hpp>

#include <sharpen/Quorum.hpp>

sharpen::TimerLoop::LoopStatus rkv::RaftGroup::FollowerLoop() noexcept
{
    if(this->raft_.GetRole() == sharpen::RaftRole::Leader)
    {
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    rkv::VoteProposal proposal;
    {
        std::unique_lock<sharpen::AsyncMutex> lock{*this->raftLock_};
        this->raft_.RaiseElection();
        proposal.SetTerm(this->raft_.GetCurrentTerm());
        proposal.SetLastIndex(this->raft_.GetLastIndex());
        proposal.SetLastTerm(this->raft_.GetLastTerm());
    }
    proposal.Id() = this->raft_.GetSelfId();
    proposal.Callback() = std::bind(&Self::RequestVoteCallback,this);
    sharpen::AwaitableFuture<bool> continuation;
    sharpen::AwaitableFuture<void> finish;
    std::puts("[Info]Try to request vote from other members");
    sharpen::Quorum::TimeLimitedProposeAsync(this->proposeTimer_,std::chrono::milliseconds{Self::electionMaxWaitMs_},this->raft_.Members().begin(),this->raft_.Members().end(),proposal,continuation,finish);
    continuation.Await();
    bool result{false};
    {
        std::unique_lock<sharpen::AsyncMutex> lock{*this->raftLock_};
        result = this->raft_.StopElection();
        if(!result)
        {
            this->raft_.ReactNewTerm(proposal.GetMaxTerm());
        }
        for (auto begin = this->raft_.Members().begin(),end = this->raft_.Members().end(); begin != end; ++begin)
        {
            begin->second.SetCurrentIndex(this->raft_.GetLastApplied());
        }
    }
    if(result)
    {
        std::printf("[Info]Become leader of term %llu\n",this->raft_.GetCurrentTerm());
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

bool rkv::RaftGroup::ProposeAppendEntires()
{
    rkv::LogProposal proposal{const_cast<const RaftType*>(&this->raft_)->PersistenceStorage()};
    proposal.SetCommitIndex(this->raft_.GetCommitIndex());
    proposal.SetTerm(this->raft_.GetCurrentTerm());
    proposal.Id() = this->raft_.GetSelfId();
    sharpen::AwaitableFuture<bool> continuation;
    sharpen::AwaitableFuture<void> finish;
    std::puts("[Info]Try to append entries to other members");
    sharpen::Quorum::TimeLimitedProposeAsync(this->proposeTimer_,std::chrono::milliseconds{Self::appendEntriesMaxWaitMs_},this->raft_.Members().begin(),this->raft_.Members().end(),proposal,continuation,finish);
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
    this->raft_.ReactNewTerm(proposal.GetMaxTerm());
    return result;
}

void rkv::RaftGroup::RequestVoteCallback() noexcept
{
    std::unique_lock<sharpen::SpinLock> lock{*this->voteLock_};
    this->raft_.ReactVote(1);
}

sharpen::TimerLoop::LoopStatus rkv::RaftGroup::LeaderLoop() noexcept
{
    if(this->raft_.GetRole() != sharpen::RaftRole::Leader)
    {
        this->followerLoop_.Start();
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    if(!this->raftLock_->TryLock())
    {
        return sharpen::TimerLoop::LoopStatus::Continue;
    }
    bool result{false};
    std::size_t commitSize{0};
    {
        std::unique_lock<sharpen::AsyncMutex> lock{*this->raftLock_,std::adopt_lock};
        std::uint64_t index{this->raft_.GetLastIndex()};
        result = this->ProposeAppendEntires();
        for (auto begin = this->raft_.Members().begin(),end = this->raft_.Members().end(); begin != end; ++begin)
        {
            if(begin->second.GetCurrentIndex() >= index)
            {
                ++commitSize;
            }
        }
        if(result && commitSize >= this->raft_.MemberMajority())
        {
            this->raft_.SetCommitIndex(index);
            this->raft_.ApplyLogs(RaftType::LostPolicy::Ignore);
        }
    }
    return sharpen::TimerLoop::LoopStatus::Continue;
}