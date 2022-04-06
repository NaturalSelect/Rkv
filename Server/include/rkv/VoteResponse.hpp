#pragma once
#ifndef _RKV_VOTERESPONSE_HPP
#define _RKV_VOTERESPONSE_HPP

#include <utility>

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class VoteResponse:public sharpen::BinarySerializable<rkv::VoteResponse>
    {
    private:
        using Self = rkv::VoteResponse;
    
        bool success_;
        std::uint64_t term_;
    public:
    
        VoteResponse() = default;

        explicit VoteResponse(bool success)
            :success_(success)
            ,term_(0)
        {}
    
        VoteResponse(const Self &other) = default;
    
        VoteResponse(Self &&other) noexcept = default;
    
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
                this->success_ = other.success_;
                this->term_ = other.term_;
            }
            return *this;
        }
    
        ~VoteResponse() noexcept = default;

        inline bool Success() const noexcept
        {
            return this->success_;
        }

        inline bool Fail() const noexcept
        {
            return !this->success_;
        }

        inline void SetResult(bool result) noexcept
        {
            this->success_ = result;
        }

        inline std::uint64_t GetTerm() const noexcept
        {
            return this->term_;
        }

        inline void SetTerm(std::uint64_t term) noexcept
        {
            this->term_ = term;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif