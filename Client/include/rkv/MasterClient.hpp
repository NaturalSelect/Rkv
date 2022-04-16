#pragma once
#ifndef _RKV_MASTERCLIENT_HPP
#define _RKV_MASTERCLIENT_HPP

#include "ClientOperator.hpp"

namespace rkv
{
    class MasterClient
    {
    private:
        using Self = rkv::MasterClient;
    

    public:
    
        MasterClient();
    
        MasterClient(const Self &other);
    
        MasterClient(Self &&other) noexcept;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~MasterClient() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }
    };
}

#endif