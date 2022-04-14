#pragma once
#ifndef _RKV_GETSHARDBYKEYRESPONSE_HPP
#define _RKV_GETSHARDBYKEYRESPONSE_HPP

#include "Shard.hpp"

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetShardByKeyResponse:public sharpen::BinarySerializable<rkv::GetShardByKeyResponse>
    {
    private:
        using Self = rkv::GetShardByKeyResponse;
    
        sharpen::Optional<rkv::Shard> shard_;
    public:
    
        GetShardByKeyResponse() = default;
    
        GetShardByKeyResponse(const Self &other) = default;
    
        GetShardByKeyResponse(Self &&other) noexcept = default;
    
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
                this->shard_ = std::move(other.shard_);   
            }
            return *this;
        }
    
        ~GetShardByKeyResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline sharpen::Optional<rkv::Shard> &Shard() noexcept
        {
            return this->shard_;
        }

        inline const sharpen::Optional<rkv::Shard> &Shard() const noexcept
        {
            return this->shard_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif