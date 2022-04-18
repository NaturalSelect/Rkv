#pragma once
#ifndef _RKV_STARTMIGRATIONREQUEST_HPP
#define _RKV_STARTMIGRATIONREQUEST_HPP

#include "Migration.hpp"

namespace rkv
{
    class StartMigrationRequest:public sharpen::BinarySerializable<rkv::StartMigrationRequest>
    {
    private:
        using Self = rkv::StartMigrationRequest;
    
        rkv::Migration migration_;
    public:
    
        StartMigrationRequest() = default;
    
        StartMigrationRequest(const Self &other) = default;
    
        StartMigrationRequest(Self &&other) noexcept = default;
    
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
                this->migration_ = std::move(other.migration_);
            }
            return *this;
        }
    
        ~StartMigrationRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline rkv::Migration &Migration() noexcept
        {
            return this->migration_;
        }

        inline const rkv::Migration &Migration() const noexcept
        {
            return this->migration_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif