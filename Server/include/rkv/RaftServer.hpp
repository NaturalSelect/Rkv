#pragma once
#ifndef _RKV_RAFTSERVER_HPP
#define _RKV_RAFTSERVER_HPP

#include <random>

#include <sharpen/TcpServer.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/TimerLoop.hpp>

#include "RaftLog.hpp"
#include "RaftMember.hpp"
#include "RaftStorage.hpp"
#include "KeyValueService.hpp"
#include "RaftServerOption.hpp"

namespace rkv
{
    class RaftServer:private sharpen::TcpServer
    {
    protected:
        
        virtual void OnNewChannel(sharpen::NetStreamChannelPtr channel) override;
    private:
        using Self = rkv::RaftServer;
        using Raft = sharpen::RaftWrapper<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel) const;

        void OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const;

        void OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnAppendEntires(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        sharpen::TimerLoop::LoopStatus FollowerLoop();

        sharpen::TimerLoop::LoopStatus LeaderLoop();

        std::chrono::milliseconds GenerateWaitTime() const;

        bool ProposeAppendEntires();

        void RequestVoteCallback() noexcept;

        static constexpr std::uint32_t followerMinWaitMs{5*1000};
        static constexpr std::uint32_t followerMaxWaitMs{10*1000};
        static constexpr std::uint32_t leaderMaxWaitMs{3*1000};
        static constexpr std::uint32_t electionMaxWaitMs{1*1000};
        static constexpr std::uint32_t appendEntriesMaxWaitMs{1*1000};

        mutable std::minstd_rand random_;
        std::uniform_int_distribution<std::uint32_t> distribution_;
        std::shared_ptr<rkv::KeyValueService> app_;
        std::unique_ptr<Raft> raft_;
        sharpen::SpinLock voteLock_;
        sharpen::TimerPtr proposeTimer_;
        sharpen::TimerLoop leaderLoop_;
        sharpen::TimerLoop followerLoop_;
    public:
        RaftServer(sharpen::EventEngine &engine,const rkv::RaftServerOption &option);
    
        ~RaftServer() noexcept = default;

        inline void RunAsync()
        {
            this->followerLoop_.Start();
            sharpen::TcpServer::RunAsync();
        }

        void Stop()
        {
            sharpen::TcpServer::Stop();
            this->followerLoop_.Terminate();
            this->leaderLoop_.Terminate();
        }
    };
}

#endif