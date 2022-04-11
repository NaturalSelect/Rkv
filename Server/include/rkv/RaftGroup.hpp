#pragma once
#ifndef _RKV_RAFTGROUP_HPP
#define _RKV_RAFTGROUP_HPP

#include <random>

#include <sharpen/EventEngine.hpp>
#include <sharpen/IpEndPoint.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/AsyncMutex.hpp>
#include <sharpen/TimerLoop.hpp>

#include "RaftLog.hpp"
#include "RaftMember.hpp"
#include "KeyValueService.hpp"
#include "RaftStorage.hpp"

namespace rkv
{
    class RaftGroup:public sharpen::Noncopyable
    {
    private:
        using Self = rkv::RaftGroup;
        using RaftType = sharpen::RaftWrapper<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;
        using RaftLock = sharpen::AsyncMutex;
        using VoteLock = sharpen::SpinLock;

        inline std::chrono::milliseconds GenerateElectionWaitTime() const noexcept
        {
            std::uint32_t val{this->distribution_(this->random_)};
            return std::chrono::milliseconds{val};
        }

        sharpen::TimerLoop::LoopStatus FollowerLoop() noexcept;

        sharpen::TimerLoop::LoopStatus LeaderLoop() noexcept;

        void RequestVoteCallback() noexcept;

        static constexpr std::uint32_t followerMinWaitMs_{5*1000};
        static constexpr std::uint32_t followerMaxWaitMs_{10*1000};
        static constexpr std::uint32_t leaderMaxWaitMs_{3*1000};
        static constexpr std::uint32_t electionMaxWaitMs_{1*1000};
        static constexpr std::uint32_t appendEntriesMaxWaitMs_{1*1000};

        mutable std::minstd_rand random_;
        std::uniform_int_distribution<std::uint32_t> distribution_;
        RaftType raft_;
        std::unique_ptr<RaftLock> raftLock_;
        std::unique_ptr<VoteLock> voteLock_;
        sharpen::TimerPtr proposeTimer_;
        std::unique_ptr<sharpen::TimerLoop> leaderLoop_;
        std::unique_ptr<sharpen::TimerLoop> followerLoop_;
    public:
    
        RaftGroup(sharpen::EventEngine &engine,const sharpen::IpEndPoint &id,rkv::RaftStorage storage,std::shared_ptr<rkv::KeyValueService> app)
            :random_(std::random_device{}())
            ,distribution_(Self::followerMinWaitMs_,Self::followerMaxWaitMs_)
            ,raft_(id,std::move(storage),std::move(app))
            ,raftLock_(new RaftLock{})
            ,voteLock_{new VoteLock{}}
            ,proposeTimer_(sharpen::MakeTimer(engine))
            ,leaderLoop_(nullptr)
            ,followerLoop_(nullptr)
        {
            this->leaderLoop_.reset(new sharpen::TimerLoop{engine,sharpen::MakeTimer(engine),std::chrono::milliseconds{Self::leaderMaxWaitMs_},std::bind(&Self::LeaderLoop,this)});
            this->followerLoop_.reset(new sharpen::TimerLoop{engine,sharpen::MakeTimer(engine),std::bind(&Self::FollowerLoop,this),std::bind(&Self::GenerateElectionWaitTime,this)});
            std::is_move_constructible<sharpen::TimerLoop>::value;
            if(!this->raftLock_ || !this->voteLock_ || !this->leaderLoop_ || !this->followerLoop_)
            {
                throw std::bad_alloc();
            }
        }
    
        RaftGroup(Self &&other) noexcept = default;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->random_ = std::move(other.random_);
                this->distribution_ = std::move(other.distribution_);
                this->raft_ = std::move(other.raft_);
                this->raftLock_ = std::move(other.raftLock_);
                this->voteLock_ = std::move(other.voteLock_);
                this->proposeTimer_ = std::move(other.proposeTimer_);
                this->leaderLoop_ = std::move(other.leaderLoop_);
                this->followerLoop_ = std::move(other.followerLoop_);
            }
            return *this;
        }
    
        ~RaftGroup() noexcept
        {
            this->Stop();
        }
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline void Tick()
        {
            if(this->raft_.GetRole() == sharpen::RaftRole::Follower)
            {
                return this->followerLoop_->Cancel();
            }
            return this->leaderLoop_->Cancel();
        }

        inline void Start()
        {
            this->followerLoop_->Start();
        }

        inline void Stop() noexcept
        {
            this->followerLoop_->Terminate();
            this->leaderLoop_->Terminate();
        }

        inline RaftLock &GetRaftLock() const noexcept
        {
            return *this->raftLock_;
        }

        inline VoteLock &GetVoteLock() const noexcept
        {
            return *this->voteLock_;
        }

        inline RaftType &Raft() noexcept
        {
            return this->raft_;
        }

        inline const RaftType &Raft() const noexcept
        {
            return this->raft_;
        }

        bool ProposeAppendEntires();
    };   
}

#endif