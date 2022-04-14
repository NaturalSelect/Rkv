#pragma once
#ifndef _RKV_MIGRATION_HPP
#define _RKV_MIGRATION_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/ByteBuffer.hpp>

namespace rkv
{
    class Migration
    {
    private:
        using Self = rkv::Migration;
    
        sharpen::Uint64 source_;
        sharpen::IpEndPoint destination_;
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
    public:
    
        Migration();
    
        Migration(const Self &other);
    
        Migration(Self &&other) noexcept;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~Migration() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }
    };
}

#endif