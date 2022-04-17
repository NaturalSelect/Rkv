#pragma once
#ifndef _RKV_GETSHARDBYIDREQUEST_HPP
#define _RKV_GETSHARDBYIDREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetShardByIdRequest:public sharpen::BinarySerializable<rkv::GetShardByIdRequest>
    {
    private:
        using Self = rkv::GetShardByIdRequest;
    
        std::uint64_t id_;
    public:
    
        GetShardByIdRequest() = default;
    
        GetShardByIdRequest(const Self &other) = default;
    
        GetShardByIdRequest(Self &&other) noexcept = default;
    
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
                this->id_ = other.id_;
            }
            return *this;
        }
    
        ~GetShardByIdRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::uint64_t GetId() const noexcept
        {
            return this->id_;
        }

        inline void SetId(std::uint64_t id) noexcept
        {
            this->id_ = id;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif