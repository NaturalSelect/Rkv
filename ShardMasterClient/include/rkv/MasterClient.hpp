#pragma once
#ifndef _RKV_MASTERCLIENT_HPP
#define _RKV_MASTERCLIENT_HPP

#include <rkv/Shard.hpp>
#include <rkv/Migration.hpp>
#include <rkv/CompletedMigration.hpp>
#include <rkv/Client.hpp>
#include <rkv/GetShardByWorkerIdRequest.hpp>
#include <rkv/GetShardByWorkerIdResponse.hpp>
#include <rkv/GetShardByIdRequest.hpp>
#include <rkv/GetShardByIdResponse.hpp>
#include <rkv/GetCompletedMigrationsRequest.hpp>
#include <rkv/GetCompletedMigrationsResponse.hpp>
#include <rkv/GetMigrationsRequest.hpp>
#include <rkv/GetMigrationsResponse.hpp>
#include <rkv/GetShardByKeyRequest.hpp>
#include <rkv/GetShardByKeyResponse.hpp>
#include <rkv/DeriveShardRequest.hpp>
#include <rkv/DeriveShardResponse.hpp>
#include <rkv/CompleteMigrationRequest.hpp>
#include <rkv/CompleteMigrationResponse.hpp>

namespace rkv
{
    class MasterClient:public rkv::Client
    {
    private:
        using Self = rkv::MasterClient;
    
        static sharpen::Optional<rkv::Shard> GetShardByKey(sharpen::NetStreamChannelPtr channel,const sharpen::ByteBuffer &key);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Shard&&>())>
        static void GetShardByWorkerId(sharpen::NetStreamChannelPtr channel,_InsertIterator inserter,const sharpen::IpEndPoint &id)
        {
            rkv::GetShardByWorkerIdRequest request;
            request.WorkerId() = id;
            sharpen::ByteBuffer buf;
            request.Serialize().StoreTo(buf);
            rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByWorkerIdRequest,buf.GetSize())};
            Self::WriteMessage(channel,header,buf);
            Self::ReadMessage(channel,rkv::MessageType::GetShardByWorkerIdResponse,buf);
            rkv::GetShardByWorkerIdResponse response;
            response.Unserialize().LoadFrom(buf);
            for (auto begin = response.ShardsBegin(),end = response.ShardsEnd(); begin != end; ++begin)
            {
                *inserter++ = std::move(*begin);
            }
        }

        static rkv::DeriveResult DeriveNewShard(sharpen::NetStreamChannelPtr channel,std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endKey);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Migration&&>())>
        static void GetMigrationsByDestination(sharpen::NetStreamChannelPtr channel,_InsertIterator inserter,const sharpen::IpEndPoint &destination)
        {
            rkv::GetMigrationsRequest request;
            request.Destination() = destination;
            sharpen::ByteBuffer buf;
            request.Serialize().StoreTo(buf);
            rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetMigrationsRequest,buf.GetSize())};
            Self::WriteMessage(channel,header,buf);
            Self::ReadMessage(channel,rkv::MessageType::GetMigrationsResponse,buf);
            rkv::GetMigrationsResponse response;
            response.Unserialize().LoadFrom(buf);
            for (auto begin = response.MigrationBegin(),end = response.MigrationEnd(); begin != end; ++begin)
            {
                *inserter++ = *begin;
            }
        }

        static rkv::CompleteMigrationResult CompleteMigration(sharpen::NetStreamChannelPtr channel,std::uint64_t groupId,const sharpen::IpEndPoint &id);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const rkv::CompletedMigration&>())>
        static void GetCompletedMigrations(sharpen::NetStreamChannelPtr channel,_InsertIterator inserter,std::uint64_t beginId,std::uint64_t source)
        {
            rkv::GetCompletedMigrationsRequest request;
            request.SetSource(source);
            request.SetBeginId(beginId);
            sharpen::ByteBuffer buf;
            request.Serialize().StoreTo(buf);
            rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetCompletedMigrationsRequest,buf.GetSize())};
            Self::WriteMessage(channel,header,buf);
            rkv::GetCompletedMigrationsResponse response;
            Self::ReadMessage(channel,rkv::MessageType::GetCompletedMigrationsResponse,buf);
            response.Unserialize().LoadFrom(buf);
            for (auto begin = response.MigrationsBegin(),end = response.MigrationsEnd(); begin != end; ++begin)
            {
                *inserter++ = *begin;
            }
        }

        static sharpen::Optional<rkv::Shard> GetShardById(sharpen::NetStreamChannelPtr channel,std::uint64_t id);
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

        sharpen::Optional<rkv::Shard> GetShard(std::uint64_t id);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Shard&>())>
        void GetShard(_InsertIterator inserter,const sharpen::IpEndPoint &id)
        {
            this->FillLeaderId();
            try
            {
                auto conn{this->GetConnection(this->leaderId_.Get())};
                Self::GetShardByWorkerId(conn,inserter,id);
            }
            catch(const std::exception&)
            {
                this->leaderId_.Reset();
                throw;
            }
        }

        rkv::DeriveResult DeriveShard(std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endKey);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Migration&&>())>
        void GetMigrations(_InsertIterator inserter,const sharpen::IpEndPoint &destination)
        {
            this->FillLeaderId();
            try
            {
                auto conn{this->GetConnection(this->leaderId_.Get())};
                Self::GetMigrationsByDestination(conn,inserter,destination);
            }
            catch(const std::exception&)
            {
                this->leaderId_.Reset();
                throw;
            }
        }

        rkv::CompleteMigrationResult CompleteMigration(std::uint64_t groupId,const sharpen::IpEndPoint &id);

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const rkv::CompletedMigration&>())>
        void GetCompletedMigrations(_InsertIterator inserter,std::uint64_t beginId,std::uint64_t source)
        {
            this->FillLeaderId();
            try
            {
                auto conn{this->GetConnection(this->leaderId_.Get())};
                Self::GetCompletedMigrations(conn,inserter,beginId,source);
            }
            catch(const std::exception&)
            {
                this->leaderId_.Reset();
                throw;
            }
        }
    };
}

#endif