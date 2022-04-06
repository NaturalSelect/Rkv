#pragma once
#ifndef _RKV_PUTRESPONSE_HPP
#define _RKV_PUTRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class PutResponse:public sharpen::BinarySerializable<rkv::PutResponse>
    {
    private:
        using Self = rkv::PutResponse;
    
        bool success_;
    public:
    
        PutResponse() = default;

        explicit PutResponse(bool success)
            :success_(success)
        {}
    
        PutResponse(const Self &other) = default;
    
        PutResponse(Self &&other) noexcept = default;
    
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
                this->success_ = other.success_;
            }
            return *this;
        }
    
        ~PutResponse() noexcept = default;

        inline bool Success() const noexcept
        {
            return this->success_;
        }

        inline bool Fail() const noexcept
        {
            return !this->success_;
        }

        inline void SetResult(bool result) noexcept
        {
            this->success_ = result;
        }

        static constexpr std::size_t ComputeSize() noexcept
        {
            return sizeof(bool);
        }

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif