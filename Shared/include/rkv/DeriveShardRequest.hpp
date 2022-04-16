#pragma once
#ifndef _RKV_ADJUSTSHARDREQUEST_HPP
#define _RKV_ADJUSTSHARDREQUEST_HPP

#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class DeriveShardRequest:public sharpen::BinarySerializable<rkv::DeriveShardRequest>
    {
    private:
        using Self = rkv::DeriveShardRequest;
    
        std::uint64_t source_;
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
    public:
    
        DeriveShardRequest() = default;
    
        DeriveShardRequest(const Self &other) = default;
    
        DeriveShardRequest(Self &&other) noexcept = default;
    
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
                this->source_ = other.source_;
                this->beginKey_ = std::move(other.beginKey_);
            }
            return *this;
        }
    
        ~DeriveShardRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::uint64_t GetSource() const noexcept
        {
            return this->source_;
        }

        inline void SetSource(std::uint64_t source) noexcept
        {
            this->source_ = source;
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