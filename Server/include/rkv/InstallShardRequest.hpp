#pragma once
#ifndef _RKV_INSTALLSHARDREQUEST_HPP
#define _RKV_INSTALLSHARDREQUEST_HPP

#include <utility>

namespace rkv
{
    class InstallShardRequest
    {
    private:
        using Self = rkv::InstallShardRequest;
    
        
    public:
    
        InstallShardRequest();
    
        InstallShardRequest(const Self &other);
    
        InstallShardRequest(Self &&other) noexcept;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~InstallShardRequest() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }
    };
}

#endif