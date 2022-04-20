#pragma once
#ifndef _RKV_PUTREQUEST_HPP
#define _RKV_PUTREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>
#include <sharpen/ByteBuffer.hpp>

namespace rkv
{
    class PutRequest:public sharpen::BinarySerializable<rkv::PutRequest>
    {
    private:
        using Self = rkv::PutRequest;

        std::uint64_t version_;
        sharpen::ByteBuffer key_;
        sharpen::ByteBuffer value_;
    public:
        PutRequest() = default;

        PutRequest(sharpen::ByteBuffer key,sharpen::ByteBuffer value)
            :version_(0)
            ,key_(std::move(key))
            ,value_(std::move(value))
        {}
    
        PutRequest(const Self &other) = default;
    
        PutRequest(Self &&other) noexcept = default;
    
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
                this->version_ = other.version_;
                this->key_ = std::move(other.key_);
                this->value_ = std::move(other.value_);
            }
            return *this;
        }
    
        ~PutRequest() noexcept = default;

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;

        inline sharpen::ByteBuffer &Key() noexcept
        {
            return this->key_;
        }

        inline const sharpen::ByteBuffer &Key() const noexcept
        {
            return this->key_;
        }

        inline sharpen::ByteBuffer &Value() noexcept
        {
            return this->value_;
        }

        inline const sharpen::ByteBuffer &Value() const noexcept
        {
            return this->value_;
        }

        inline std::uint64_t GetVersion() const noexcept
        {
            return this->version_;
        }

        inline void SetVersion(std::uint64_t version) noexcept
        {
            this->version_ = version;
        }
    };
}

#endif