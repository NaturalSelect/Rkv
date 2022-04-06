#pragma once
#ifndef _RKV_RAFTMEMBER_HPP
#define _RKV_RAFTMEMBER_HPP

#include <utility>
#include <cstdint>

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/EventEngine.hpp>

#include "RaftLog.hpp"
#include "LogProposal.hpp"
#include "VoteProposal.hpp"

namespace rkv
{
    class RaftMember
    {
    private:
        using Self = rkv::RaftMember;
    
        std::uint64_t currentIndex_;
        sharpen::IpEndPoint id_;
        sharpen::NetStreamChannelPtr channel_;
        sharpen::EventEngine *engine_;

        void ConnectToEndPoint();

        void DoProposeAsync(const rkv::LogProposal *proposal,sharpen::Future<bool> *result);

        void DoProposeAsync(const rkv::VoteProposal *proposal,sharpen::Future<bool> *result);
    public:
        RaftMember(sharpen::IpEndPoint id,sharpen::EventEngine *engine);
    
        RaftMember(const Self &other) = default;
    
        RaftMember(Self &&other) noexcept = default;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~RaftMember() noexcept = default;

        inline sharpen::Uint64 GetCurrentIndex() const noexcept
        {
            return this->currentIndex_;
        }

        inline void SetCurrentIndex(sharpen::Uint64 index) noexcept
        {
            this->currentIndex_ = index;
        }

        inline sharpen::IpEndPoint &Id() noexcept
        {
            return this->id_;
        }

        inline const sharpen::IpEndPoint &Id() const noexcept
        {
            return this->id_;
        }

        void Cancel();

        void ProposeAsync(const rkv::LogProposal &proposal,sharpen::Future<bool> &result);

        void ProposeAsync(const rkv::VoteProposal &proposal,sharpen::Future<bool> &result);
    };
}

#endif