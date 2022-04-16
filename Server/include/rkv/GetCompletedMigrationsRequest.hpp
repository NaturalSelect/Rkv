#pragma once
#ifndef _RKV_GETCOMPLETEDMIGRATIONSREQUEST_HPP
#define _RKV_GETCOMPLETEDMIGRATIONSREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetCompletedMigrationsRequest:public sharpen::BinarySerializable<rkv::GetCompletedMigrationsRequest>
    {
    private:
        using Self = rkv::GetCompletedMigrationsRequest;
    
        std::uint64_t source_;
        std::uint64_t beginId_;
    public:
    
        GetCompletedMigrationsRequest() = default;
    
        GetCompletedMigrationsRequest(const Self &other) = default;
    
        GetCompletedMigrationsRequest(Self &&other) noexcept = default;
    
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
                this->source_ = other.source_;
                this->beginId_ = other.beginId_;
            }
            return *this;
        }
    
        ~GetCompletedMigrationsRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::uint64_t GetSource() const noexcept
        {
            return this->source_;
        }

        inline void SetSource(std::uint64_t source) noexcept
        {
            this->source_ = source;
        }

        inline std::uint64_t GetBeginId() const noexcept
        {
            return this->beginId_;
        }

        inline void SetBeginId(std::uint64_t id) noexcept
        {
            this->beginId_ = id;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif