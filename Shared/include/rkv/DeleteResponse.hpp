#pragma once
#ifndef _RKV_DELETERESPONSE_HPP
#define _RKV_DELETERESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

#include "MotifyResult.hpp"

namespace rkv
{
    class DeleteResponse:public sharpen::BinarySerializable<rkv::DeleteResponse>
    {
    private:
        using Self = rkv::DeleteResponse;
    
        rkv::MotifyResult result_;
    public:
    
        DeleteResponse() = default;

        explicit DeleteResponse(rkv::MotifyResult result)
            :result_(result)
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
                this->result_ = other.result_;
            }
            return *this;
        }
    
        ~DeleteResponse() noexcept = default;

        inline rkv::MotifyResult GetResult() const noexcept
        {
            return this->result_;
        }

        inline void SetResult(rkv::MotifyResult result) noexcept
        {
            this->result_ = result;
        }

        static constexpr std::size_t ComputeSize() noexcept
        {
            return sizeof(rkv::MotifyResult);
        }

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif