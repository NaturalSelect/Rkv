#pragma once
#ifndef _RKV_MIGRATERESPONSE_HPP
#define _RKV_MIGRATERESPONSE_HPP

#include <map>

#include <sharpen/BinarySerializable.hpp>
#include <sharpen/ByteBuffer.hpp>

namespace rkv
{
    class MigrateResponse:public sharpen::BinarySerializable<rkv::MigrateResponse>
    {
    private:
        using Self = rkv::MigrateResponse;
    
        std::map<sharpen::ByteBuffer,sharpen::ByteBuffer> map_;
    public:
    
        MigrateResponse() = default;
    
        MigrateResponse(const Self &other) = default;
    
        MigrateResponse(Self &&other) noexcept = default;
    
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
                this->map_ = std::move(other.map_);
            }
            return *this;
        }
    
        ~MigrateResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::map<sharpen::ByteBuffer,sharpen::ByteBuffer> &Map() noexcept
        {
            return this->map_;
        }

        inline const std::map<sharpen::ByteBuffer,sharpen::ByteBuffer> &Map() const noexcept
        {
            return this->map_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif