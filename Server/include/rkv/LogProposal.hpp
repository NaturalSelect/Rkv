#pragma once
#ifndef _RKV_RAFTPROPOSAL_HPP
#define _RKV_RAFTPROPOSAL_HPP

#include <sharpen/IpEndPoint.hpp>

#include "RaftStorage.hpp"

namespace rkv
{
    class LogProposal
    {
    private:
        using Self = rkv::LogProposal;
    
        const rkv::RaftStorage *storage_;
        sharpen::IpEndPoint id_;
        std::uint64_t term_;
        std::uint64_t commintIndex_;
        std::uint64_t maxTerm_;
    public:

        explicit LogProposal(const rkv::RaftStorage &storage)
            :storage_(&storage)
            ,id_()
            ,term_(0)
            ,commintIndex_(0)
            ,maxTerm_(0)
        {}
    
        LogProposal(const Self &other) = default;
    
        LogProposal(Self &&other) noexcept = default;
    
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
                this->storage_ = other.storage_;
                this->id_ = std::move(other.id_);
                this->term_ = other.term_;
                this->commintIndex_ = other.commintIndex_;
                this->maxTerm_ = other.maxTerm_;
            }
            return *this;
        }
    
        ~LogProposal() noexcept = default;

        inline const rkv::RaftStorage &GetStorage() const noexcept
        {
            return *this->storage_;
        }

        inline sharpen::IpEndPoint &Id() noexcept
        {
            return this->id_;
        }

        inline const sharpen::IpEndPoint &Id() const noexcept
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

        inline std::uint64_t GetCommitIndex() const noexcept
        {
            return this->commintIndex_;
        }

        inline void SetCommitIndex(std::uint64_t index) noexcept
        {
            this->commintIndex_ = index;
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
    };
}

#endif