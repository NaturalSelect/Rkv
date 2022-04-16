#pragma once
#ifndef _RKV_MASTERCLIENT_HPP
#define _RKV_MASTERCLIENT_HPP

#include <rkv/Shard.hpp>
#include <rkv/Migration.hpp>
#include <rkv/CompletedMigration.hpp>
#include <rkv/Client.hpp>
#include <rkv/GetShardByIdRequest.hpp>
#include <rkv/GetShardByIdResponse.hpp>

namespace rkv
{
    class MasterClient:rkv::Client
    {
    private:
        using Self = rkv::MasterClient;
    
        static sharpen::Optional<rkv::Shard> GetShardByKey(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &key);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Shard&>())>
        static void GetShardById(sharpen::INetStreamChannel &channel,_InsertIterator inserter,const sharpen::IpEndPoint &id)
        {
            rkv::GetShardByIdRequest request;
            request.Id() = id;
            sharpen::ByteBuffer buf;
            request.Serialize().StoreTo(buf);
            rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByIdRequest,buf.GetSize())};
            Self::WriteMessage(channel,header,buf);
            Self::ReadMessage(channel,rkv::MessageType::GetShardByIdResponse,buf);
            rkv::GetShardByIdResponse response;
            response.Unserialize().LoadFrom(buf);
            for (auto begin = response.ShardsBegin(),end = response.ShardsEnd(); begin != end; ++begin)
            {
                *inserter++ = std::move(*begin);
            }
        }
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

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Shard&>())>
        void GetShard(_InsertIterator inserter,const sharpen::IpEndPoint &id)
        {
            this->FillLeaderId();
            auto conn{this->GetConnection(this->leaderId_.Get())};
            Self::GetShardById(*conn,inserter,id);
        }
    };
}

#endif