#pragma once
#ifndef _RKV_VOTEPROPOSAL_HPP
#define _RKV_VOTEPROPOSAL_HPP

#include <utility>

#include <sharpen/IpEndPoint.hpp>

namespace rkv
{
    class VoteProposal
    {
    private:
        using Self = VoteProposal;
    
        sharpen::IpEndPoint id_;
        std::uint64_t term_;
        std::uint64_t lastLogIndex_;
        std::uint64_t lastLogTerm_;
        std::function<void()> callback_;
        std::uint64_t maxTerm_;
        sharpen::Optional<std::uint64_t> group_;
    public:
    
        VoteProposal() = default;
    
        VoteProposal(const Self &other) = default;
    
        VoteProposal(Self &&other) noexcept = default;
    
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
                this->id_ = std::move(other.id_);
                this->term_ = other.term_;
                this->lastLogIndex_ = other.lastLogIndex_;
                this->lastLogTerm_ = other.lastLogTerm_;
                this->callback_ = std::move(other.callback_);
                this->maxTerm_ = other.maxTerm_;
                this->group_ = std::move(other.group_);
            }
            return *this;
        }
    
        ~VoteProposal() noexcept = default;

        sharpen::IpEndPoint &Id() noexcept
        {
            return this->id_;
        }

        const sharpen::IpEndPoint &Id() const noexcept
        {
            return this->id_;
        }

        inline std::uint64_t GetTerm() const noexcept
        {
            return this->term_;
        }

        inline void SetTerm(std::uint64_t term) noexcept
        {
            this->term_ = term;
            this->maxTerm_ = term;
        }

        inline std::uint64_t GetLastIndex() const noexcept
        {
            return this->lastLogIndex_;
        }

        inline void SetLastIndex(std::uint64_t index) noexcept
        {
            this->lastLogIndex_ = index;
        }

        inline std::uint64_t GetLastTerm() const noexcept
        {
            return this->lastLogTerm_;
        }

        inline void SetLastTerm(std::uint64_t term) noexcept
        {
            this->lastLogTerm_ = term;
        }

        inline std::function<void()> &Callback() noexcept
        {
            return this->callback_;
        }

        inline const std::function<void()> &Callback() const noexcept
        {
            return this->callback_;
        }

        inline void SetMaxTerm(std::uint64_t term) noexcept
        {
            if(term > this->maxTerm_)
            {
                this->maxTerm_ = term;
            }
        }

        inline std::uint64_t GetMaxTerm() const noexcept
        {
            return this->maxTerm_;
        }

        inline sharpen::Optional<std::uint64_t> &Group() noexcept
        {
            return this->group_;
        }

        inline const sharpen::Optional<std::uint64_t> &Group() const noexcept
        {
            return this->group_;
        }
    };   
}

#endif