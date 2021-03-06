#pragma once
#ifndef _RKV_GETSHARDBYWORKERIDREQUEST_HPP
#define _RKV_GETSHARDBYWORKERIDREQUEST_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetShardByWorkerIdRequest:public sharpen::BinarySerializable<rkv::GetShardByWorkerIdRequest>
    {
    private:
        using Self = rkv::GetShardByWorkerIdRequest;
    
        sharpen::IpEndPoint workerId_;
    public:
    
        GetShardByWorkerIdRequest() = default;
    
        GetShardByWorkerIdRequest(const Self &other) = default;
    
        GetShardByWorkerIdRequest(Self &&other) noexcept = default;
    
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
                this->workerId_ = std::move(other.workerId_);
            }
            return *this;
        }
    
        ~GetShardByWorkerIdRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline sharpen::IpEndPoint &WorkerId() noexcept
        {
            return this->workerId_;
        }

        inline const sharpen::IpEndPoint &WorkerId() const noexcept
        {
            return this->workerId_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif