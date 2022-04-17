#pragma once
#ifndef _RKV_LEADERREDIRECTREQUEST_HPP
#define _RKV_LEADERREDIRECTREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class LeaderRedirectRequest:public sharpen::BinarySerializable<rkv::LeaderRedirectRequest>
    {
    private:
        using Self = rkv::LeaderRedirectRequest;
    
        sharpen::Optional<std::uint64_t> group_;
    public:
    
        LeaderRedirectRequest() = default;
    
        LeaderRedirectRequest(const Self &other) = default;
    
        LeaderRedirectRequest(Self &&other) noexcept = default;
    
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
                this->group_ = std::move(other.group_);
            }
            return *this;
        }
    
        ~LeaderRedirectRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
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