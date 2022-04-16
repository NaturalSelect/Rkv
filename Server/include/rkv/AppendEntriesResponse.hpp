#pragma once
#ifndef _RKV_APPENDENTIRESRESPONSE_HPP
#define _RKV_APPENDENTIRESRESPONSE_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class AppendEntriesResponse:public sharpen::BinarySerializable<rkv::AppendEntriesResponse>
    {
    private:
        using Self = rkv::AppendEntriesResponse;
    
        bool success_;
        std::uint64_t term_;
        std::uint64_t appiledIndex_;
    public:
        AppendEntriesResponse() = default;

        explicit AppendEntriesResponse(bool success);
    
        AppendEntriesResponse(const Self &other) = default;
    
        AppendEntriesResponse(Self &&other) noexcept = default;
    
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
                this->term_ = other.term_;
                this->appiledIndex_ = other.appiledIndex_;
            }
            return *this;
        }
    
        ~AppendEntriesResponse() noexcept = default;

        inline bool Success() const noexcept
        {
            return this->success_;
        }

        inline void SetResult(bool success) noexcept
        {
            this->success_ = success;
        }

        inline bool Fail() const noexcept
        {
            return !this->success_;
        }

        inline std::uint64_t GetTerm() const noexcept
        {
            return this->term_;
        }

        inline void SetTerm(std::uint64_t term) noexcept
        {
            this->term_ = term;
        }

        inline std::uint64_t GetAppiledIndex() const noexcept
        {
            return this->appiledIndex_;
        }

        inline void SetAppiledIndex(std::uint64_t index) noexcept
        {
            this->appiledIndex_ = index;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif