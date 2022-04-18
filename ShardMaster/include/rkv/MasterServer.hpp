#pragma once
#ifndef _RKV_RAFTSERVER_HPP
#define _RKV_RAFTSERVER_HPP

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

#include "MasterServerOption.hpp"
#include "ShardManger.hpp"
#include "CompletedMigrationManager.hpp"
#include "MigrationManger.hpp"

namespace rkv
{
    class MasterServer:private sharpen::TcpServer
    {
    protected:

        virtual void OnNewChannel(sharpen::NetStreamChannelPtr channel) override;
    private:
        using Self = rkv::MasterServer;
        using Raft = sharpen::RaftWrapper<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;

        static constexpr std::size_t replicationFactor_{3};

        static constexpr std::size_t reverseLogsCount_{16};

        static sharpen::ByteBuffer zeroKey_;

        sharpen::IpEndPoint GetRandomWorkerId() const noexcept;

        bool TryConnect(const sharpen::IpEndPoint &endpoint) const noexcept;

        void NotifyMigrationCompleted(const sharpen::IpEndPoint &engpoint) const noexcept;

        void NotifyStartMigration(const sharpen::IpEndPoint &engpoint);

        void FlushStatus();

        inline void FlushStatusWithLock()
        {
            this->statusLock_.LockWrite();
            std::unique_lock<sharpen::AsyncReadWriteLock> lock{this->statusLock_,std::adopt_lock};
            this->FlushStatus();
        }

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const sharpen::IpEndPoint&>())>
        inline std::size_t SelectWorkers(_InsertIterator inserter,std::size_t count) const noexcept
        {
            std::unordered_set<sharpen::IpEndPoint> set{count};
            std::unordered_set<sharpen::IpEndPoint> badSet;
            do
            {
                sharpen::IpEndPoint id{this->GetRandomWorkerId()};
                if(!set.count(id))
                {
                    if (this->TryConnect(id))
                    {
                        *inserter++ = id;
                        set.emplace(id);
                    }
                    else
                    {
                        badSet.emplace(id);
                    }
                }
            } while (set.size() != count && set.size() + badSet.size() != this->workers_.size());
            return set.size();
        }

        template<typename _InsertIterator,typename _Iterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::Migration&>(),std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>())>
        inline void GenrateMigrations(_InsertIterator inserter,_Iterator begin,_Iterator end,std::uint64_t beginId,std::uint64_t groupId,std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endkey)
        {
            while (begin != end)
            {
                const sharpen::IpEndPoint &id{*begin};
                rkv::Migration migration;
                migration.Destination() = id;
                migration.SetSource(source);
                migration.SetId(beginId++);
                migration.SetGroupId(groupId);
                migration.BeginKey() = beginKey;
                migration.EndKey() = endkey;
                *inserter++ = std::move(migration);
                ++begin;   
            }
        }

        template<typename _Iterator,typename _Check = decltype(std::declval<rkv::RaftLog&>() = *std::declval<_Iterator&>())>
        inline rkv::AppendEntriesResult AppendEntries(_Iterator begin,_Iterator end,std::uint64_t commitIndex)
        {
            this->group_->DelayCycle();
            std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
            if(this->group_->Raft().GetRole() != sharpen::RaftRole::Leader)
            {
                return rkv::AppendEntriesResult::NotCommit;
            }
            while (begin != end)
            {
                this->group_->Raft().AppendLog(*begin);
                ++begin;
            }
            std::size_t commitCount{0};
            bool result{false};
            do
            {
                result = this->group_->ProposeAppendEntries();
                if(!result)
                {
                    break;
                }
                for (auto memberBegin = this->group_->Raft().Members().begin(),memberEnd = this->group_->Raft().Members().end(); memberBegin != memberEnd; ++memberBegin)
                {
                    if (memberBegin->second.GetCurrentIndex() >= commitIndex)
                    {
                        commitCount += 1;   
                    }
                }
            } while (commitCount < this->group_->Raft().MemberMajority());
            if(result)
            {
                this->group_->Raft().SetCommitIndex(commitIndex);
                this->group_->Raft().ApplyLogs(Raft::LostPolicy::Ignore);
                this->FlushStatus();
                return rkv::AppendEntriesResult::Appiled;
            }
            return rkv::AppendEntriesResult::Commited;
        }

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel);

        void OnAppendEntries(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetShardByWorkerId(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetShardByKey(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDerviveShard(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetCompletedMigrations(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetMigrations(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnCompleteMigration(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnGetShardById(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        std::shared_ptr<rkv::KeyValueService> app_;
        std::unique_ptr<rkv::RaftGroup> group_;
        mutable std::minstd_rand random_;
        std::uniform_int_distribution<std::size_t> distribution_;
        std::vector<sharpen::IpEndPoint> workers_;
        sharpen::AsyncReadWriteLock statusLock_;
        std::unique_ptr<rkv::ShardManger> shards_;
        std::unique_ptr<rkv::MigrationManger> migrations_;
        std::unique_ptr<rkv::CompletedMigrationManager> completedMigrations_;
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