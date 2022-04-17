#pragma once
#ifndef _RKV_GETSHARDBYIDREQUEST_HPP
#define _RKV_GETSHARDBYIDREQUEST_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetShardByWorkerIdRequest:public sharpen::BinarySerializable<rkv::GetShardByWorkerIdRequest>
    {
    private:
        using Self = rkv::GetShardByWorkerIdRequest;
    
        sharpen::IpEndPoint id_;
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
                this->id_ = std::move(other.id_);
            }
            return *this;
        }
    
        ~GetShardByWorkerIdRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline sharpen::IpEndPoint &Id() noexcept
        {
            return this->id_;
        }

        inline const sharpen::IpEndPoint &Id() const noexcept
        {
            return this->id_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif