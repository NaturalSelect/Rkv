#pragma once
#ifndef _RKV_GETMIGRATIONSREQUEST_HPP
#define _RKV_GETMIGRATIONSREQUEST_HPP

#include <sharpen/BinarySerializable.hpp>
#include <sharpen/IpEndPoint.hpp>

namespace rkv
{
    class GetMigrationsRequest:public sharpen::BinarySerializable<rkv::GetMigrationsRequest>
    {
    private:
        using Self = rkv::GetMigrationsRequest;

        sharpen::IpEndPoint destination_;
    public:
    
        GetMigrationsRequest() = default;
    
        GetMigrationsRequest(const Self &other) = default;
    
        GetMigrationsRequest(Self &&other) noexcept = default;
    
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
                this->destination_ = std::move(other.destination_);
            }
            return *this;
        }
    
        ~GetMigrationsRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline sharpen::IpEndPoint &Destination() noexcept
        {
            return this->destination_;
        }

        inline const sharpen::IpEndPoint &Destination() const noexcept
        {
            return this->destination_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif