#pragma once
#ifndef _RKV_GETSHARDBYIDRESPONSE_HPP
#define _RKV_GETSHARDBYIDRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

#include "Shard.hpp"

namespace rkv
{
    class GetShardByIdResponse:public sharpen::BinarySerializable<rkv::GetShardByIdResponse>
    {
    private:
        using Self = rkv::GetShardByIdResponse;
    
        sharpen::Optional<rkv::Shard> shard_;
    public:
    
        GetShardByIdResponse() = default;
    
        GetShardByIdResponse(const Self &other) = default;
    
        GetShardByIdResponse(Self &&other) noexcept = default;
    
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
    
        ~GetShardByIdResponse() noexcept = default;
    
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