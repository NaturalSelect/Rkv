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

        static constexpr std::size_t maxKeysPerShard_{5000};

        static std::string FormatStorageName(std::uint64_t id);

        void ExecuteMigration(const rkv::Migration &migration);

        void ExecuteMigrationAndNotify(const rkv::Migration &migration);

        void CleaupCompletedMigration(const rkv::CompletedMigration &migration);

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel) const;

        void OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const;

        void OnPutFail(sharpen::INetStreamChannel &channel);

        void OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDeleteFail(sharpen::INetStreamChannel &channel);

        void OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        sharpen::IpEndPoint selfId_;
        std::shared_ptr<rkv::KeyValueService> app_;
        sharpen::AsyncMutex clientLock_;
        std::unique_ptr<rkv::MasterClient> client_;
        std::map<std::uint64_t,std::unique_ptr<rkv::RaftGroup>> groups_;
        sharpen::FileChannelPtr counterFile_;
    public:
        WorkerServer(sharpen::EventEngine &engine,const rkv::WorkerServerOption &option);
    
        ~WorkerServer() noexcept = default;

        inline void RunAsync()
        {
            for(auto begin = this->groups_.begin(),end = this->groups_.end(); begin != end; ++begin)
            {
                begin->second->Start();   
            }
            sharpen::TcpServer::RunAsync();
        }

        void Stop()
        {
            sharpen::TcpServer::Stop();
            for(auto begin = this->groups_.begin(),end = this->groups_.end(); begin != end; ++begin)
            {
                begin->second->Stop();   
            }
        }
    };
}

#endif