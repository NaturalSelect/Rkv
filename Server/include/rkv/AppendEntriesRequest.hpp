#pragma once
#ifndef _RKV_APPENDENTIRESREQUEST_HPP
#define _RKV_APPENDENTIRESREQUEST_HPP

#include <sharpen/IpEndPoint.hpp>

#include "RaftLog.hpp"

namespace rkv
{
    class AppendEntriesRequest:public sharpen::BinarySerializable<rkv::AppendEntriesRequest>
    {
    private:
        using Self = rkv::AppendEntriesRequest;
    

        std::vector<rkv::RaftLog> logs_;
        sharpen::IpEndPoint leaderId_;
        sharpen::Uint64 leaderTerm_;
        sharpen::Uint64 prevLogIndex_;
        sharpen::Uint64 prevLogTerm_;
        sharpen::Uint64 commitIndex_;
        sharpen::Optional<std::uint64_t> group_;
    public:
    
        AppendEntriesRequest() = default;
    
        AppendEntriesRequest(const Self &other) = default;
    
        AppendEntriesRequest(Self &&other) noexcept = default;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->logs_ = std::move(other.logs_);
                this->leaderId_ = std::move(other.leaderId_);
                this->leaderTerm_ = other.leaderTerm_;
                this->prevLogIndex_ = other.prevLogIndex_;
                this->prevLogTerm_ = other.prevLogTerm_;
                this->commitIndex_ = other.commitIndex_;
                this->group_ = std::move(other.group_);
            }
            return *this;
        }
    
        ~AppendEntriesRequest() noexcept = default;

        inline std::vector<rkv::RaftLog> &Logs() noexcept
        {
            return this->logs_;
        }

        inline const std::vector<rkv::RaftLog> &Logs() const noexcept
        {
            return this->logs_;
        }

        inline sharpen::IpEndPoint &LeaderId() noexcept
        {
            return this->leaderId_;
        }

        inline const sharpen::IpEndPoint &LeaderId() const noexcept
        {
            return this->leaderId_;
        }

        inline std::uint64_t GetLeaderTerm() const noexcept
        {
            return this->leaderTerm_;
        }

        inline void SetLeaderTerm(std::uint64_t term) noexcept
        {
            this->leaderTerm_ = term;
        }

        inline std::uint64_t GetPrevLogIndex() const noexcept
        {
            return this->prevLogIndex_;
        }

        inline void SetPrevLogIndex(std::uint64_t index) noexcept
        {
            this->prevLogIndex_ = index;
        }

        inline std::uint64_t GetPrevLogTerm() const noexcept
        {
            return this->prevLogTerm_;
        }

        inline void SetPrevLogTerm(std::uint64_t term) noexcept
        {
            this->prevLogTerm_ = term;
        }

        inline std::uint64_t GetCommitIndex() const noexcept
        {
            return this->commitIndex_;
        }

        inline void SetCommitIndex(std::uint64_t index) noexcept
        {
            this->commitIndex_ = index;
        }

        inline sharpen::Optional<std::uint64_t> &Group() noexcept
        {
            return this->group_;
        }

        inline const sharpen::Optional<std::uint64_t> &Group() const noexcept
        {
            return this->group_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif