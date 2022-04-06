#pragma once
#ifndef _RKV_DELETERESPONSE_HPP
#define _RKV_DELETERESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class DeleteResponse:public sharpen::BinarySerializable<rkv::DeleteResponse>
    {
    private:
        using Self = rkv::DeleteResponse;
    
        bool success_;
    public:
    
        DeleteResponse() = default;

        explicit DeleteResponse(bool success)
            :success_(success)
        {}
    
        DeleteResponse(const Self &other) = default;
    
        DeleteResponse(Self &&other) noexcept = default;
    
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
    
        ~DeleteResponse() noexcept = default;

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