#pragma once
#ifndef _RKV_RAFTGROUP_HPP
#define _RKV_RAFTGROUP_HPP

#include <sharpen/RaftGroup.hpp>

#include "RaftLog.hpp"
#include "RaftMember.hpp"
#include "KeyValueService.hpp"
#include "RaftStorage.hpp"

namespace rkv
{
    using RaftGroupBase = sharpen::RaftGroup<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;

    class RaftGroup:public RaftGroupBase
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

        static constexpr std::uint32_t minElectionCycle_{5*1000};
        static constexpr std::uint32_t maxElectionCycle_{10*1000};
        static constexpr std::uint32_t appendEntriesCycle_{3*1000};
        static constexpr std::uint32_t electionMaxWaitMs_{1*1000};
        static constexpr std::uint32_t appendEntriesMaxWaitMs_{1*1000};

        static inline sharpen::RaftGroupOption MakeOption() noexcept
        {
            sharpen::RaftGroupOption opt;
            opt.SetAppendEntriesCycle(std::chrono::milliseconds{Self::appendEntriesCycle_});
            opt.SetMinElectionCycle(std::chrono::milliseconds{Self::minElectionCycle_});
            opt.SetMaxElectionCycle(std::chrono::milliseconds{Self::maxElectionCycle_});
            return opt;
        }

        std::function<void()> appendEntriesCb_;
    public:
    
        RaftGroup(sharpen::EventEngine &engine,const sharpen::IpEndPoint &id,rkv::RaftStorage storage,std::shared_ptr<rkv::KeyValueService> app)
            :RaftGroupBase(engine,id,std::move(storage),std::move(app),Self::MakeOption())
            ,appendEntriesCb_()
        {}
    
        RaftGroup(Self &&other) noexcept = default;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                RaftGroupBase::operator=(std::move(other));
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

        bool ProposeAppendEntries();

        inline void SetAppendEntriesCallback(std::function<void()> cb) noexcept
        {
            this->appendEntriesCb_ = std::move(cb);
        }
    };   
}

#endif