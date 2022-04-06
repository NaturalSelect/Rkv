#pragma once
#ifndef _RKV_LEADERREDIRECTRESPONSE_HPP
#define _RKV_LEADERREDIRECTRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>
#include <sharpen/IpEndPoint.hpp>

namespace rkv
{
    class LeaderRedirectResponse:public sharpen::BinarySerializable<rkv::LeaderRedirectResponse>
    {
    private:
        using Self = rkv::LeaderRedirectResponse;
    
        bool knowLeader_;
        sharpen::IpEndPoint ep_;    
    public:
    
        LeaderRedirectResponse() = default;
    
        LeaderRedirectResponse(const Self &other) = default;
    
        LeaderRedirectResponse(Self &&other) noexcept = default;
    
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
                this->ep_ = std::move(other.ep_);
            }
            return *this;
        }
    
        ~LeaderRedirectResponse() noexcept = default;

        inline bool KnowLeader() const noexcept
        {
            return this->knowLeader_;
        }

        inline void SetKnowLeader(bool know) noexcept
        {
            this->knowLeader_ = know;
        }

        sharpen::IpEndPoint &Endpoint() noexcept
        {
            return this->ep_;
        }

        const sharpen::IpEndPoint &Endpoint() const noexcept
        {
            return this->ep_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,sharpen::Size size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif