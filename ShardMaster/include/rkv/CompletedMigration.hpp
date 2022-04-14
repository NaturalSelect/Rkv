#pragma once
#ifndef _RKV_COMPLETEDMIGRATION_HPP
#define _RKV_COMPLETEDMIGRATION_HPP

#include <utility>

#include <sharpen/ByteBuffer.hpp>

namespace rkv
{
    class CompletedMigration
    {
    private:
        using Self = rkv::CompletedMigration;
    
        std::uint64_t source_;
        std::uint64_t destination_;
        sharpen::ByteBuffer beginKey_;
        sharpen::ByteBuffer endKey_;
    public:
    
        CompletedMigration();
    
        CompletedMigration(const Self &other);
    
        CompletedMigration(Self &&other) noexcept;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~CompletedMigration() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }
    };   
}

#endif