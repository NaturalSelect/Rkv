#pragma once
#ifndef _RKV_CLEARSHARDRESPONSE_HPP
#define _RKV_CLEARSHARDRESPONSE_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class ClearShardResponse:public sharpen::BinarySerializable<rkv::ClearShardResponse>
    {
    private:
        using Self = rkv::ClearShardResponse;
    
        bool result_;
    public:
    
        ClearShardResponse() = default;
    
        ClearShardResponse(const Self &other) = default;
    
        ClearShardResponse(Self &&other) noexcept = default;
    
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
    
        ~ClearShardResponse() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline bool GetResult() const noexcept
        {
            return this->result_;
        }

        inline void SetResult(bool result) noexcept
        {
            this->result_ = result;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif