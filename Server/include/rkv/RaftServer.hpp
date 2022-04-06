#pragma once
#ifndef _RKV_RAFTSERVER_HPP
#define _RKV_RAFTSERVER_HPP

#include <random>

#include <sharpen/TcpServer.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/TimerLoop.hpp>

#include <rkv/RaftLog.hpp>
#include <rkv/RaftMember.hpp>
#include <rkv/RaftStorage.hpp>
#include <rkv/KeyValueService.hpp>

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

        std::mt19937 random_;
        std::shared_ptr<rkv::KeyValueService> app_;
        std::unique_ptr<Raft> raft_;
        sharpen::SpinLock voteLock_;
        sharpen::TimerLoop leaderLoop_;
        sharpen::TimerLoop followerLoop_;
    public:
    
        RaftServer(sharpen::EventEngine &engine,sharpen::IpEndPoint &selfId);
    
        ~RaftServer() noexcept = default;

        void RunAsync();
    };
}

#endif