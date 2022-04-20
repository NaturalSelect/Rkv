#pragma once
#ifndef _RKV_GETVERSIONRESPONSE_HPP
#define _RKV_GETVERSIONRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class GetVersionResponse:public sharpen::BinarySerializable<rkv::GetVersionResponse>
    {
    private:
        using Self = rkv::GetVersionResponse;
    
        std::uint64_t version_;
    public:
    
        GetVersionResponse() = default;
    
        GetVersionResponse(const Self &other) = default;
    
        GetVersionResponse(Self &&other) noexcept = default;
    
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
                this->version_ = other.version_;
            }
            return *this;
        }
    
        ~GetVersionResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::uint64_t GetVersion() const noexcept
        {
            return this->version_;
        }

        inline void SetVersion(std::uint64_t version) noexcept
        {
            this->version_ = version;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif