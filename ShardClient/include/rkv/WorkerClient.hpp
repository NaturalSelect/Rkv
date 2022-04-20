#pragma once
#ifndef _RKV_KVCLIENT_HPP
#define _RKV_KVCLIENT_HPP

#include <random>
#include <unordered_map>

#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/IpEndPoint.hpp>
#include <sharpen/IteratorOps.hpp>

#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/MessageHeader.hpp>
#include <rkv/GetPolicy.hpp>
#include <rkv/Client.hpp>
#include <rkv/GetVersionResponse.hpp>

namespace rkv
{
    class WorkerClient:public rkv::Client
    {
    private:
        using Self = rkv::WorkerClient;

        static std::uint64_t GetVersion(sharpen::NetStreamChannelPtr channel);

        static sharpen::ByteBuffer GetValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key);

        static rkv::MotifyResult PutKeyValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,sharpen::ByteBuffer value,std::uint64_t version);
    
        static rkv::MotifyResult DeleteKey(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,std::uint64_t version);

        void FillLeaderVersion();

        std::uint64_t versionOfLeader_;
    public:
    
        template<typename _Iterator,typename _Rep,typename _Period,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        WorkerClient(sharpen::EventEngine &engine,_Iterator begin,_Iterator end,const std::chrono::duration<_Rep,_Period> &restoreTimeout,std::size_t maxTimeoutCount,std::uint64_t group)
            :Client(engine,begin,end,restoreTimeout,maxTimeoutCount)
            ,versionOfLeader_(0)
        {
            this->Group().Construct(group);
        }
    
        WorkerClient(Self &&other) noexcept = default;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                rkv::Client::operator=(std::move(other));
            }
            return *this;
        }
    
        ~WorkerClient() noexcept = default;
    
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
    };
}

#endif