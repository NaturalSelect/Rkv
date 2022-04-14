#pragma once
#ifndef _RKV_SHARD_HPP
#define _RKV_SHARD_HPP

#include <utility>

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class Shard:public sharpen::BinarySerializable<rkv::Shard>
    {
    private:
        using Self = rkv::Shard;
        using WorkersType = std::vector<sharpen::IpEndPoint>;
    
        std::uint64_t id_;
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
        WorkersType workers_;
    public:
    
        Shard() = default;
    
        Shard(const Self &other) = default;
    
        Shard(Self &&other) noexcept = default;
    
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
                //this->key_ = std::move(other.key_);
                this->workers_ = std::move(other.workers_);
            }
            return *this;
        }
    
        ~Shard() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;

        // inline sharpen::ByteBuffer &Key() noexcept
        // {
        //     return this->key_;
        // }

        // inline const sharpen::ByteBuffer &Key() const noexcept
        // {
        //     return this->key_;
        // }

        inline WorkersType &Workers() noexcept
        {
            return this->workers_;
        }

        inline const WorkersType &Workers() const noexcept
        {
            return this->workers_;
        }

        inline std::uint64_t GetId() const noexcept
        {
            return this->id_;
        }

        inline void SetId(std::uint64_t id) noexcept
        {
            this->id_ = id;
        }
    };
}

#endif