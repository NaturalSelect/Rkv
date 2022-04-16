#pragma once
#ifndef _RKV_GETSHARDBYIDRESPONSE_HPP
#define _RKV_GETSHARDBYIDRESPONSE_HPP

#include <iterator>

#include <sharpen/BinarySerializable.hpp>

#include <rkv/Shard.hpp>

namespace rkv
{
    class GetShardByIdResponse:public sharpen::BinarySerializable<rkv::GetShardByIdResponse>
    {
    private:
        using Self = rkv::GetShardByIdResponse;
        using Shards = std::vector<rkv::Shard>;
        using Iterator = typename Shards::iterator;
        using ConstIterator = typename Shards::const_iterator;
    
        Shards shards_;
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
                this->shards_ = std::move(other.shards_);
            }
            return *this;
        }
    
        ~GetShardByIdResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::back_insert_iterator<Shards> GetShardsInserter() noexcept
        {
            return std::back_inserter(this->shards_);
        }

        inline Iterator ShardsBegin() noexcept
        {
            return this->shards_.begin();
        }
        
        inline ConstIterator ShardsBegin() const noexcept
        {
            return this->shards_.begin();
        }
        
        inline Iterator ShardsEnd() noexcept
        {
            return this->shards_.end();
        }
        
        inline ConstIterator ShardsEnd() const noexcept
        {
            return this->shards_.end();
        }

        inline bool Empty() const noexcept
        {
            return this->shards_.empty();
        }

        inline std::size_t GetSize() const noexcept
        {
            return this->shards_.size();
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif