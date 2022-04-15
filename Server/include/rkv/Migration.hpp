#pragma once
#ifndef _RKV_MIGRATION_HPP
#define _RKV_MIGRATION_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class Migration:public sharpen::BinarySerializable<rkv::Migration>
    {
    private:
        using Self = rkv::Migration;
    
        std::uint64_t id_;
        sharpen::Uint64 source_;
        sharpen::IpEndPoint destination_;
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
    public:
    
        Migration() = default;
    
        Migration(const Self &other) = default;
    
        Migration(Self &&other) noexcept = default;
    
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
                this->source_ = other.source_;
                this->destination_ = std::move(other.destination_);
                this->beginKey_ = std::move(other.beginKey_);
                this->endKey_ = std::move(other.endKey_);
            }
            return *this;
        }
    
        ~Migration() noexcept = default;
    
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

        inline std::uint64_t GetSource() const noexcept
        {
            return this->source_;
        }

        inline void SetSource(std::uint64_t source) noexcept
        {
            this->source_ = source;
        }

        inline sharpen::IpEndPoint &Destination() noexcept
        {
            return this->destination_;
        }

        inline const sharpen::IpEndPoint &Destination() const noexcept
        {
            return this->destination_;
        }

        inline sharpen::ByteBuffer &BeginKey() noexcept
        {
            return this->beginKey_;
        }

        inline const sharpen::ByteBuffer &BeginKey() const noexcept
        {
            return this->beginKey_;
        }

        inline sharpen::ByteBuffer &EndKey() noexcept
        {
            return this->endKey_;
        }

        inline const sharpen::ByteBuffer &EndKey() const noexcept
        {
            return this->endKey_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif