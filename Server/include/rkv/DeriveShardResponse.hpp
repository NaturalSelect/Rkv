#pragma once
#ifndef _RKV_DERIVESHARDRESPONSE_HPP
#define _RKV_DERIVESHARDRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

#include "DeriveResult.hpp"

namespace rkv
{
    class DeriveShardResponse:public sharpen::BinarySerializable<rkv::DeriveShardResponse>
    {
    private:
        using Self = rkv::DeriveShardResponse;
    
        rkv::DeriveResult result_;
    public:
    
        DeriveShardResponse() = default;
    
        DeriveShardResponse(const Self &other) = default;
    
        DeriveShardResponse(Self &&other) noexcept = default;
    
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
    
        ~DeriveShardResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline rkv::DeriveResult GetResult() const noexcept
        {
            return this->result_;
        }

        inline void SetResult(rkv::DeriveResult result) noexcept
        {
            this->result_ = result;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif