#pragma once
#ifndef _RKV_MASTERCLIENT_HPP
#define _RKV_MASTERCLIENT_HPP

#include <rkv/Shard.hpp>
#include <rkv/Migration.hpp>
#include <rkv/CompletedMigration.hpp>
#include <rkv/Client.hpp>

namespace rkv
{
    class MasterClient:rkv::Client
    {
    private:
        using Self = rkv::MasterClient;
    
        static sharpen::Optional<rkv::Shard> GetShardByKey(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &key);

        
    public:
    
        template<typename _Iterator,typename _Rep,typename _Period,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        MasterClient(sharpen::EventEngine &engine,_Iterator begin,_Iterator end,const std::chrono::duration<_Rep,_Period> &restoreTimeout,std::size_t maxTimeoutCount)
            :Client(engine,begin,end,restoreTimeout,maxTimeoutCount)
        {}
    
        MasterClient(Self &&other) noexcept = default;
    
        Self &operator=(Self &&other) noexcept = default;
    
        ~MasterClient() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        sharpen::Optional<rkv::Shard> GetShard(const sharpen::ByteBuffer &key);
    };
}

#endif