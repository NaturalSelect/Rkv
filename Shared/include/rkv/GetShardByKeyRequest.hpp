#pragma once
#ifndef _RKV_GETSHARDBYKEYREQUEST_HPP
#define _RKV_GETSHARDBYKEYREQUEST_HPP

#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetShardByKeyRequest:public sharpen::BinarySerializable<rkv::GetShardByKeyRequest>
    {
    private:
        using Self = rkv::GetShardByKeyRequest;

        sharpen::ByteBuffer key_;
    public:
    
        GetShardByKeyRequest() = default;
    
        GetShardByKeyRequest(const Self &other) = default;
    
        GetShardByKeyRequest(Self &&other) noexcept = default;
    
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
                this->key_ = std::move(other.key_);
            }
            return *this;
        }
    
        ~GetShardByKeyRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline sharpen::ByteBuffer &Key() noexcept
        {
            return this->key_;
        }

        inline const sharpen::ByteBuffer &Key() const noexcept
        {
            return this->key_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif