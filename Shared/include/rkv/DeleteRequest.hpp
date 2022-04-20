#pragma once
#ifndef _RKV_DELETEREQUEST_HPP
#define _RKV_DELETEREQUEST_HPP

#include <string>

#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class DeleteRequest:public sharpen::BinarySerializable<rkv::DeleteRequest>
    {
    private:
        using Self = DeleteRequest;
    
        std::uint64_t version_;
        sharpen::ByteBuffer key_;
    public:
    
        DeleteRequest() = default;

        explicit DeleteRequest(sharpen::ByteBuffer key)
            :version_(0)
            ,key_(std::move(key))
        {}

        explicit DeleteRequest(const std::string &key)
            :version_(0)
            ,key_()
        {
            if(!key.empty())
            {
                this->key_.ExtendTo(key.size());
                std::memcpy(this->key_.Data(),key.data(),key.size());
            }
        }
    
        DeleteRequest(const Self &other) = default;
    
        DeleteRequest(Self &&other) noexcept = default;
    
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
            }
            return *this;
        }
    
        ~DeleteRequest() noexcept = default;

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