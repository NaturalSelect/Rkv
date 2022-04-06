#pragma once
#ifndef _RKV_GETREQUEST_HPP
#define _RKV_GETREQUEST_HPP

#include <string>

#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetRequest:public sharpen::BinarySerializable<rkv::GetRequest>
    {
    private:
        using Self = GetRequest;
    
        sharpen::ByteBuffer key_;
    public:
    
        GetRequest() = default;

        explicit GetRequest(sharpen::ByteBuffer key)
            :key_(std::move(key))
        {}

        explicit GetRequest(const std::string &key)
            :key_()
        {
            if(!key.empty())
            {
                this->key_.ExtendTo(key.size());
                std::memcpy(this->key_.Data(),key.data(),key.size());
            }
        }
    
        GetRequest(const Self &other) = default;
    
        GetRequest(Self &&other) noexcept = default;
    
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
                this->key_ = std::move(other.key_);
            }
            return *this;
        }
    
        ~GetRequest() noexcept = default;

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
    };
}

#endif