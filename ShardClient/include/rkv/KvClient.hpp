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

namespace rkv
{
    class KvClient:public rkv::Client
    {
    private:
        using Self = rkv::KvClient;

        static sharpen::ByteBuffer GetValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key);

        static rkv::MotifyResult PutKeyValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,sharpen::ByteBuffer value);
    
        static rkv::MotifyResult DeleteKey(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key);

        static constexpr std::uint32_t waitElectionMs_{5*1000};
        static constexpr std::size_t maxWaitElectionCount_{10};
    public:
    
        template<typename _Iterator,typename _Rep,typename _Period,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        KvClient(sharpen::EventEngine &engine,_Iterator begin,_Iterator end,const std::chrono::duration<_Rep,_Period> &restoreTimeout,std::size_t maxTimeoutCount,std::uint64_t group)
            :Client(engine,begin,end,restoreTimeout,maxTimeoutCount)
        {
            this->Group().Construct(group);
        }
    
        KvClient(Self &&other) noexcept = default;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                rkv::Client::operator=(std::move(other));
            }
            return *this;
        }
    
        ~KvClient() noexcept = default;
    
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