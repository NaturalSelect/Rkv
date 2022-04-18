#pragma once
#ifndef _RKV_MIGRATEREQUEST_HPP
#define _RKV_MIGRATEREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>
#include <sharpen/ByteBuffer.hpp>

namespace rkv
{
    class MigrateRequest:public sharpen::BinarySerializable<rkv::MigrateRequest>
    {
    private:
        using Self = rkv::MigrateRequest;
    
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
    public:
    
        MigrateRequest() = default;
    
        MigrateRequest(const Self &other) = default;
    
        MigrateRequest(Self &&other) noexcept = default;
    
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
                this->beginKey_ = std::move(other.beginKey_);
                this->endKey_ = std::move(other.endKey_);
            }
            return *this;
        }
    
        ~MigrateRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
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