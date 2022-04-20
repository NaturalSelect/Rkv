#pragma once
#ifndef _RKV_SHARDCLIENT_HPP
#define _RKV_SHARDCLIENT_HPP

#include <map>

#include <rkv/MasterClient.hpp>

#include "WorkerClient.hpp"

namespace rkv
{
    class ShardClient
    {
    private:
        using Self = rkv::ShardClient;

        sharpen::Optional<rkv::Shard> GetShardByKey(const sharpen::ByteBuffer &key,rkv::GetPolicy policy);
    
        rkv::WorkerClient *OpenClient(const rkv::Shard &shard);

        sharpen::EventEngine *engine_;
        std::chrono::milliseconds restoreTimeout_;
        std::size_t maxTimeoutCount_;
        rkv::MasterClient masterClient_;
        std::map<sharpen::ByteBuffer,rkv::Shard> shardMap_;
        std::map<std::uint64_t,std::unique_ptr<rkv::WorkerClient>> clientMap_;
    public:
    
        template<typename _Iterator,typename _Rep,typename _Period,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        ShardClient(sharpen::EventEngine &engine,_Iterator begin,_Iterator end,const std::chrono::duration<_Rep,_Period> &restoreTimeout,std::size_t maxTimeoutCount)
            :engine_(&engine)
            ,restoreTimeout_(restoreTimeout)
            ,maxTimeoutCount_(maxTimeoutCount)
            ,masterClient_(engine,begin,end,restoreTimeout,maxTimeoutCount)
            ,shardMap_()
            ,clientMap_()
        {}
    
        ShardClient(Self &&other) noexcept;
    
        Self &operator=(Self &&other) noexcept;
    
        ~ShardClient() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        sharpen::Optional<sharpen::ByteBuffer> Get(sharpen::ByteBuffer key,rkv::GetPolicy policy);

        inline sharpen::Optional<sharpen::ByteBuffer> Get(sharpen::ByteBuffer key)
        {
            return this->Get(std::move(key),rkv::GetPolicy::Strict);
        }

        rkv::MotifyResult Put(sharpen::ByteBuffer key,sharpen::ByteBuffer value);

        rkv::MotifyResult Delete(sharpen::ByteBuffer key);

        void ClearCache();
    };
}

#endif