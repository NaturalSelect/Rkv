#pragma once
#ifndef _RKV_GETCOMPLETEDMIGRATIONSRESPONSE_HPP
#define _RKV_GETCOMPLETEDMIGRATIONSRESPONSE_HPP

#include <iterator>

#include <sharpen/BinarySerializable.hpp>

#include "CompletedMigration.hpp"

namespace rkv
{
    class GetCompletedMigrationsResponse:public sharpen::BinarySerializable<rkv::GetCompletedMigrationsResponse>
    {
    private:
        using Self = rkv::GetCompletedMigrationsResponse;
        using Migrations = std::vector<rkv::CompletedMigration>;
        using Iterator = typename Migrations::iterator;
        using ConstIterator = typename Migrations::const_iterator;
        
        Migrations migrations_;
    public:
    
        GetCompletedMigrationsResponse();
    
        GetCompletedMigrationsResponse(const Self &other);
    
        GetCompletedMigrationsResponse(Self &&other) noexcept;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~GetCompletedMigrationsResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline Iterator MigrationsBegin() noexcept
        {
            return this->migrations_.begin();
        }
        
        inline ConstIterator MigrationsBegin() const noexcept
        {
            return this->migrations_.begin();
        }
        
        inline Iterator MigrationsEnd() noexcept
        {
            return this->migrations_.end();
        }
        
        inline ConstIterator MigrationsEnd() const noexcept
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