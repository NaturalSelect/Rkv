#pragma once
#ifndef _RKV_MASTERCLIENT_HPP
#define _RKV_MASTERCLIENT_HPP

#include <rkv/Shard.hpp>
#include <rkv/Migration.hpp>
#include <rkv/CompletedMigration.hpp>

#include "Client.hpp"

namespace rkv
{
    class MasterClient:rkv::Client
    {
    private:
        using Self = rkv::MasterClient;
    
    public:
    
        template<typename _Iterator,typename _Rep,typename _Period,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        MasterClient(sharpen::EventEngine &engine,_Iterator begin,_Iterator end,const std::chrono::duration<_Rep,_Period> &restoreTimeout,std::size_t maxTimeoutCount)
            :Client(engine,begin,end,restoreTimeout,maxTimeoutCount)
        {}
    
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