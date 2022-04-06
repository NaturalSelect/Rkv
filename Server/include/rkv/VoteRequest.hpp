#pragma once
#ifndef _RKV_VOTEREQUEST_HPP
#define _RKV_VOTEREQUEST_HPP

#include <utility>

#include <sharpen/BinarySerializable.hpp>
#include <sharpen/IpEndPoint.hpp>

namespace rkv
{
    class VoteRequest:public sharpen::BinarySerializable<rkv::VoteRequest>
    {
    private:
        using Self = rkv::VoteRequest;

        sharpen::IpEndPoint id_;
        std::uint64_t term_;
        std::uint64_t lastIndex_;
        std::uint64_t lastTerm_;
    public:
    
        VoteRequest() = default;
    
        VoteRequest(const Self &other) = default;
    
        VoteRequest(Self &&other) noexcept = default;
    
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
                this->lastIndex_ = other.lastIndex_;
                this->lastTerm_ = other.lastTerm_;
            }
            return *this;
        }
    
        ~VoteRequest() noexcept = default;

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
        }

        inline std::uint64_t GetLastIndex() const noexcept
        {
            return this->lastIndex_;
        }

        inline void SetLastIndex(std::uint64_t index) noexcept
        {
            this->lastIndex_ = index;
        }

        inline std::uint64_t GetLastTerm() const noexcept
        {
            return this->lastTerm_;
        }

        inline void SetLastTerm(std::uint64_t term) noexcept
        {
            this->lastTerm_ = term;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,sharpen::Size size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif