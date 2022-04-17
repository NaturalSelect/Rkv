#pragma once
#ifndef _RKV_WORKER_SERVER_HPP
#define _RKV_WORKER_SERVER_HPP

#include <random>
#include <unordered_set>

#include <sharpen/TcpServer.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/TimerLoop.hpp>
#include <sharpen/AsyncMutex.hpp>

#include <rkv/RaftLog.hpp>
#include <rkv/RaftMember.hpp>
#include <rkv/RaftStorage.hpp>
#include <rkv/RaftGroup.hpp>
#include <rkv/KeyValueService.hpp>
#include <rkv/AppendEntriesResult.hpp>
#include <rkv/MasterClient.hpp>

#include "WorkerServerOption.hpp"

namespace rkv
{
    class WorkerServer:private sharpen::TcpServer
    {
    protected:
        
        virtual void OnNewChannel(sharpen::NetStreamChannelPtr channel) override;
    private:
        using Self = rkv::WorkerServer;
        using Raft = sharpen::RaftWrapper<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel) const;

        void OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const;

        void OnPutFail(sharpen::INetStreamChannel &channel);

        void OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDeleteFail(sharpen::INetStreamChannel &channel);

        void OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        std::shared_ptr<rkv::KeyValueService> app_;
        rkv::MasterClient client_;
        std::map<std::uint64_t,std::unique_ptr<rkv::RaftGroup>> group_;
    public:
        WorkerServer(sharpen::EventEngine &engine,rkv::MasterClient client,const rkv::WorkerServerOption &option);
    
        ~WorkerServer() noexcept = default;

        inline void RunAsync()
        {
            //this->group_->Start();
            sharpen::TcpServer::RunAsync();
        }

        void Stop()
        {
            sharpen::TcpServer::Stop();
            //this->group_->Stop();
        }
    };
}

#endif