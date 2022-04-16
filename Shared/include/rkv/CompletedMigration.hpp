#pragma once
#ifndef _RKV_COMPLETEDMIGRATION_HPP
#define _RKV_COMPLETEDMIGRATION_HPP

#include <utility>

#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class CompletedMigration:public sharpen::BinarySerializable<rkv::CompletedMigration>
    {
    private:
        using Self = rkv::CompletedMigration;
    
        std::uint64_t id_;
        std::uint64_t source_;
        std::uint64_t destination_;
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
    public:
    
        CompletedMigration() = default;
    
        CompletedMigration(const Self &other) = default;
    
        CompletedMigration(Self &&other) noexcept = default;
    
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
                this->destination_ = other.destination_;
                this->beginKey_ = std::move(this->beginKey_);
                this->endKey_ = std::move(this->endKey_);
            }
            return *this;
        }
    
        ~CompletedMigration() noexcept = default;
    
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

        inline std::uint64_t GetDestination() const noexcept
        {
            return this->destination_;
        }

        inline void SetDestination(std::uint64_t destination) noexcept
        {
            this->destination_ = destination;
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