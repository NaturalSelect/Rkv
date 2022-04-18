#pragma once
#ifndef _RKV_CLEARSHARDREQUEST_HPP
#define _RKV_CLEARSHARDREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>

#include "CompletedMigration.hpp"

namespace rkv
{
    class ClearShardRequest:public sharpen::BinarySerializable<rkv::ClearShardRequest>
    {
    private:
        using Self = rkv::ClearShardRequest;
    
        rkv::CompletedMigration migration_;
    public:
    
        ClearShardRequest() = default;
    
        ClearShardRequest(const Self &other) = default;
    
        ClearShardRequest(Self &&other) noexcept = default;
    
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
    
        ~ClearShardRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline rkv::CompletedMigration &Migration() noexcept
        {
            return this->migration_;
        }

        inline const rkv::CompletedMigration &Migration() const noexcept
        {
            return this->migration_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif