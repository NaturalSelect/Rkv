#pragma once
#ifndef _RKV_COMPLETEMIGRATIONREQUEST_HPP
#define _RKV_COMPLETEMIGRATIONREQUEST_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class CompleteMigrationRequest:public sharpen::BinarySerializable<rkv::CompleteMigrationRequest>
    {
    private:
        using Self = rkv::CompleteMigrationRequest;
    
        std::uint64_t groupId_;
        sharpen::IpEndPoint id_;
    public:
    
        CompleteMigrationRequest() = default;
    
        CompleteMigrationRequest(const Self &other) = default;
    
        CompleteMigrationRequest(Self &&other) noexcept = default;
    
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
                this->groupId_ = other.groupId_;
            }
            return *this;
        }
    
        ~CompleteMigrationRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline std::uint64_t GetGroupId() const noexcept
        {
            return this->groupId_;
        }

        inline void SetGroupId(std::uint64_t id) noexcept
        {
            this->groupId_ = id;
        }

        inline sharpen::IpEndPoint &Id() noexcept
        {
            return this->id_;
        }

        inline const sharpen::IpEndPoint &Id() const noexcept
        {
            return this->id_;
        }

        std::size_t ComputeSize() const noexcept;

        std::size_t LoadFrom(const char *data,std::size_t size);

        std::size_t UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif