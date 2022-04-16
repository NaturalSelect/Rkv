#pragma once
#ifndef _RKV_COMPLETEMIGRATIONRESPONSE_HPP
#define _RKV_COMPLETEMIGRATIONRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

#include "CompleteMigrationResult.hpp"

namespace rkv
{
    class CompleteMigrationResponse:public sharpen::BinarySerializable<rkv::CompleteMigrationResponse>
    {
    private:
        using Self = rkv::CompleteMigrationResponse;
    
        rkv::CompleteMigrationResult result_;
    public:
    
        CompleteMigrationResponse() = default;
    
        CompleteMigrationResponse(const Self &other) = default;
    
        CompleteMigrationResponse(Self &&other) noexcept = default;
    
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
    
        ~CompleteMigrationResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline rkv::CompleteMigrationResult GetResult() const noexcept
        {
            return this->result_;
        }

        inline void SetResult(rkv::CompleteMigrationResult result) noexcept
        {
            this->result_ = result;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };   
}

#endif