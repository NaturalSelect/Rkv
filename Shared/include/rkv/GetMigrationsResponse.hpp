#pragma once
#ifndef _RKV_GETMIGRATIONSRESPONSE_HPP
#define _RKV_GETMIGRATIONSRESPONSE_HPP

#include <iterator>

#include <sharpen/BinarySerializable.hpp>

#include "Migration.hpp"

namespace rkv
{
    class GetMigrationsResponse:public sharpen::BinarySerializable<rkv::GetMigrationsResponse>
    {
    private:
        using Self = rkv::GetMigrationsResponse;
        using Migrations = std::vector<rkv::Migration>;
        using Iterator = typename Migrations::iterator;
        using ConstIterator = typename Migrations::const_iterator;

        Migrations migrations_;
    public:
    
        GetMigrationsResponse() = default;
    
        GetMigrationsResponse(const Self &other) = default;
    
        GetMigrationsResponse(Self &&other) noexcept = default;
    
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
                this->migrations_ = std::move(other.migrations_);
            }
            return *this;
        }
    
        ~GetMigrationsResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline Iterator MigrationBegin() noexcept
        {
            return this->migrations_.begin();
        }
        
        inline ConstIterator MigrationBegin() const noexcept
        {
            return this->migrations_.begin();
        }
        
        inline Iterator MigrationEnd() noexcept
        {
            return this->migrations_.end();
        }
        
        inline ConstIterator MigrationEnd() const noexcept
        {
            return this->migrations_.end();
        }

        inline std::back_insert_iterator<Migrations> GetMigrationsInserter() noexcept
        {
            return std::back_inserter(this->migrations_);
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif