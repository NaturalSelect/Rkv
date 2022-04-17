#pragma once
#ifndef _RKV_RAFTSERVEROPTION_HPP
#define _RKV_RAFTSERVEROPTION_HPP

#include <sharpen/IpEndPoint.hpp>

namespace rkv
{
    class WorkerServerOption
    {
    private:
        using Self = rkv::WorkerServerOption;
        using MemberIds = std::vector<sharpen::IpEndPoint>;
    public:
        using Iterator = typename MemberIds::iterator;
        using ConstIterator = typename MemberIds::const_iterator;
    private:
    
        sharpen::IpEndPoint bindEndpoint_;
        sharpen::IpEndPoint selfId_;
    public:
    
        WorkerServerOption(const sharpen::IpEndPoint &bindEndpoint,const sharpen::IpEndPoint &selfId)
            :bindEndpoint_(bindEndpoint)
            ,selfId_(selfId)
        {}
    
        WorkerServerOption(const Self &other) = default;
    
        WorkerServerOption(Self &&other) noexcept = default;
    
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
                this->selfId_ = std::move(other.selfId_);
                this->bindEndpoint_ = std::move(other.bindEndpoint_);
            }
            return *this;
        }
    
        ~WorkerServerOption() noexcept = default;

        inline sharpen::IpEndPoint &SelfId() noexcept
        {
            return this->selfId_;
        }

        inline const sharpen::IpEndPoint &SelfId() const noexcept
        {
            return this->selfId_;
        }

        inline sharpen::IpEndPoint &BindEndpoint() noexcept
        {
            return this->bindEndpoint_;
        }

        inline const sharpen::IpEndPoint &BindEndpoint() const noexcept
        {
            return this->bindEndpoint_;
        }
    };
}

#endif