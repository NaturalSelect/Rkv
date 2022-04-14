#pragma once
#ifndef _RKV_RAFTSERVER_HPP
#define _RKV_RAFTSERVER_HPP

#include <random>

#include <sharpen/TcpServer.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/TimerLoop.hpp>
#include <sharpen/AsyncMutex.hpp>

#include <rkv/RaftLog.hpp>
#include <rkv/RaftMember.hpp>
#include <rkv/RaftStorage.hpp>
#include <rkv/RaftGroup.hpp>
#include <rkv/KeyValueService.hpp>

#include "MasterServerOption.hpp"
#include "ShardManger.hpp"

namespace rkv
{
    class MasterServer:private sharpen::TcpServer
    {
    protected:
        
        virtual void OnNewChannel(sharpen::NetStreamChannelPtr channel) override;
    private:
        using Self = rkv::MasterServer;
        using Raft = sharpen::RaftWrapper<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;

        static sharpen::ByteBuffer shardsKey_;

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel) const;

        void OnAppendEntires(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetShardById(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetShardByKey(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        std::shared_ptr<rkv::KeyValueService> app_;
        std::unique_ptr<rkv::RaftGroup> group_;
        std::vector<sharpen::IpEndPoint> workers_;
        sharpen::AsyncReadWriteLock shardsLock_;
        std::unique_ptr<rkv::ShardManger> shards_;
    public:
        MasterServer(sharpen::EventEngine &engine,const rkv::MasterServerOption &option);
    
        ~MasterServer() noexcept = default;

        inline void RunAsync()
        {
            this->group_->Start();
            sharpen::TcpServer::RunAsync();
        }

        void Stop()
        {
            sharpen::TcpServer::Stop();
            this->group_->Stop();
        }
    };
}

#endif