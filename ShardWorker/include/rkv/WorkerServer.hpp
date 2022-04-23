#pragma once
#ifndef _RKV_WORKER_SERVER_HPP
#define _RKV_WORKER_SERVER_HPP

#include <random>
#include <unordered_set>

#include <sharpen/TcpServer.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/TimerLoop.hpp>
#include <sharpen/AsyncMutex.hpp>
#include <sharpen/AsyncReadWriteLock.hpp>

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

        static constexpr std::uint32_t maxKeysPerShard_{5}; //5000

        static constexpr std::uint32_t migrationTimeout_{5000};

        static constexpr std::uint32_t masterTimeout_{5000};

        static std::string FormatStorageName(std::uint64_t id);

        static void CancelClient(sharpen::Future<bool> &future,rkv::MasterClient *client)
        {
            if (future.Get())
            {
                assert(client);
                client->Cancel();
            }
        }

        sharpen::Optional<std::pair<std::uint64_t,sharpen::ByteBuffer>> GetShardId(const sharpen::ByteBuffer &key) const noexcept;

        std::uint64_t GetKeyCounter(std::uint64_t id,const sharpen::ByteBuffer &beginKey);

        std::uint64_t ScanKeyCount(std::uint64_t id,const sharpen::ByteBuffer &beginKey) const;

        void IncreaseKeyCount(std::uint64_t id);

        void ClearKeyCount(std::uint64_t id);

        sharpen::Optional<sharpen::ByteBuffer> ScanKeys(const sharpen::ByteBuffer &beginKey,std::uint64_t count) const;

        rkv::AppendEntriesResult ProposeAppendEntries(rkv::RaftGroup &group,std::uint64_t commitIndex);

        std::vector<rkv::Shard> FlushShard(const std::set<sharpen::ByteBuffer> *excludedSet,bool started);

        void DeriveNewShard(std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endKey);

        bool ExecuteMigration(const rkv::Migration &migration);

        bool ExecuteMigrationAndNotify(const rkv::Migration &migration);

        void CleaupCompletedMigration(const rkv::CompletedMigration &migration);

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnMigrate(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnClearShard(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnStartMigration(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetVersion(sharpen::INetStreamChannel &channel);

        sharpen::IpEndPoint selfId_;
        std::shared_ptr<rkv::KeyValueService> app_;
        sharpen::AsyncMutex clientLock_;
        std::unique_ptr<rkv::MasterClient> client_;
        mutable sharpen::AsyncReadWriteLock groupLock_;
        std::map<sharpen::ByteBuffer,std::uint64_t> shardMap_;
        std::map<std::uint64_t,std::unique_ptr<rkv::RaftGroup>> groups_;
        std::map<std::uint64_t,std::size_t> keyCounter_;
        sharpen::FileChannelPtr counterFile_;
        sharpen::AsyncMutex migrationLock_;
        sharpen::SpinLock keyCounterLock_;
        std::atomic_bool started_;
        std::uint64_t version_;
    public:
        WorkerServer(sharpen::EventEngine &engine,const rkv::WorkerServerOption &option);
    
        ~WorkerServer() noexcept = default;

        inline void RunAsync()
        {
            for(auto begin = this->groups_.begin(),end = this->groups_.end(); begin != end; ++begin)
            {
                std::printf("[Info]Starting shard %llu\n",begin->first);
                begin->second->Start();   
            }
            this->started_.store(true);
            sharpen::TcpServer::RunAsync();
        }

        void Stop()
        {
            this->started_.store(false);
            sharpen::TcpServer::Stop();
            for(auto begin = this->groups_.begin(),end = this->groups_.end(); begin != end; ++begin)
            {
                std::printf("[Info]Stopping shard %llu\n",begin->first);
                begin->second->Stop();   
            }
        }
    };
}

#endif