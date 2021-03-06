#include <rkv/RaftGroup.hpp>

#include <sharpen/Quorum.hpp>

sharpen::TimerLoop::LoopStatus rkv::RaftGroup::FollowerLoop() noexcept
{
    if(this->raft_.GetRole() == sharpen::RaftRole::Leader)
    {
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    rkv::VoteProposal proposal;
    proposal.Group() = this->group_;
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
    sharpen::Quorum::TimeLimitedProposeAsync(this->proposeTimer_,std::chrono::milliseconds{static_cast<std::int64_t>(Self::electionMaxWaitMs_)},this->raft_.Members().begin(),this->raft_.Members().end(),proposal,continuation,finish);
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
        std::printf("[Info]Become leader of term %llu (%llu)\n",this->raft_.GetCurrentTerm(),this->group_.Exist() ? this->group_.Get():0);
        this->leaderLoop_.Start();
        finish.WaitAsync();
        return sharpen::TimerLoop::LoopStatus::Terminate;
    }
    finish.WaitAsync();
    return sharpen::TimerLoop::LoopStatus::Continue;
}

bool rkv::RaftGroup::ProposeAppendEntries()
{
    rkv::LogProposal proposal{this->raft_.PersistenceStorage()};
    proposal.Group() = this->group_;
    proposal.SetCommitIndex(this->raft_.GetCommitIndex());
    proposal.SetTerm(this->raft_.GetCurrentTerm());
    proposal.Id() = this->raft_.GetSelfId();
    sharpen::AwaitableFuture<bool> continuation;
    sharpen::AwaitableFuture<void> finish;
    sharpen::Quorum::TimeLimitedProposeAsync(this->proposeTimer_,std::chrono::milliseconds{static_cast<std::int64_t>(Self::appendEntriesMaxWaitMs_)},this->raft_.Members().begin(),this->raft_.Members().end(),proposal,continuation,finish);
    bool result{continuation.Await()};
    if(!result)
    {
        std::fputs("[Error]Append entires to other members failure\n",stderr);
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
        result = this->ProposeAppendEntries();
        for (auto begin = this->raft_.Members().begin(),end = this->raft_.Members().end(); begin != end; ++begin)
        {
            if(begin->second.GetCurrentIndex() >= index)
            {
                ++commitSize;
            }
        }
        if(result && commitSize >= this->raft_.MemberMajority())
        {
            std::uint64_t commitIndex{this->raft_.GetCommitIndex()};
            this->raft_.SetCommitIndex(index);
            this->raft_.ApplyLogs(RaftType::LostPolicy::Ignore);
            if (commitIndex != index && this->appendEntriesCb_)
            {
                this->appendEntriesCb_();
            }
        }
    }
    return sharpen::TimerLoop::LoopStatus::Continue;
}