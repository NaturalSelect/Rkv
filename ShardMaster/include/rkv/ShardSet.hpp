#pragma once
#ifndef _RKV_SHARDSET_HPP
#define _RKV_SHARDSET_HPP

#include "Shard.hpp"

namespace rkv
{
    class ShardSet:public sharpen::BinarySerializable<rkv::ShardSet>
    {
    private:
        using Self = rkv::ShardSet;
        using Shards = std::vector<rkv::Shard>;
        using Iterator = typename Shards::iterator;
        using ConstIterator = typename Shards::const_iterator;
        
        static bool Compare(const rkv::Shard &shard,const sharpen::ByteBuffer &key) noexcept;

        Shards shards_;
    public:
    
        ShardSet() = default;
    
        ShardSet(const Self &other) = default;
    
        ShardSet(Self &&other) noexcept = default;
    
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
    
        ~ShardSet() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline Iterator Begin() noexcept
        {
            return this->shards_.begin();
        }
        
        inline ConstIterator Begin() const noexcept
        {
            return this->shards_.begin();
        }
        
        inline Iterator End() noexcept
        {
            return this->shards_.end();
        }
        
        inline ConstIterator End() const noexcept
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

        Iterator Find(const sharpen::ByteBuffer &key) noexcept;

        ConstIterator Find(const sharpen::ByteBuffer &key) const noexcept;

        Iterator Put(rkv::Shard shard);

        Iterator Delete(Iterator ite);

        Iterator Delete(const sharpen::ByteBuffer &key);

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif