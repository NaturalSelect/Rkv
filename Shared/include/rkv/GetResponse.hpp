#pragma once
#ifndef _RKV_GETRESPONSE_HPP
#define _RKV_GETRESPONSE_HPP

#include <string>

#include <sharpen/ByteBuffer.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetResponse:public sharpen::BinarySerializable<rkv::GetResponse>
    {
    private:
        using Self = rkv::GetResponse;
    
        sharpen::ByteBuffer value_;
    public:
    
        GetResponse() = default;

        explicit GetResponse(sharpen::ByteBuffer key)
            :value_(std::move(key))
        {}

        explicit GetResponse(const std::string &key)
            :value_()
        {
            if(!key.empty())
            {
                this->value_.ExtendTo(key.size());
                std::memcpy(this->value_.Data(),key.data(),key.size());
            }
        }
    
        GetResponse(const Self &other) = default;
    
        GetResponse(Self &&other) noexcept = default;
    
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
                this->value_ = std::move(other.value_);
            }
            return *this;
        }
    
        ~GetResponse() noexcept = default;

        inline sharpen::ByteBuffer &Value() noexcept
        {
            return this->value_;
        }

        inline const sharpen::ByteBuffer &Value() const noexcept
        {
            return this->value_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif