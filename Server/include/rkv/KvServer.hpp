#pragma once
#ifndef _RKV_RAFTSERVER_HPP
#define _RKV_RAFTSERVER_HPP

#include <random>

#include <sharpen/TcpServer.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/TimerLoop.hpp>
#include <sharpen/AsyncMutex.hpp>

#include "RaftLog.hpp"
#include "RaftMember.hpp"
#include "RaftStorage.hpp"
#include "RaftGroup.hpp"
#include "KeyValueService.hpp"
#include "KvServerOption.hpp"

namespace rkv
{
    class KvServer:private sharpen::TcpServer
    {
    protected:
        
        virtual void OnNewChannel(sharpen::NetStreamChannelPtr channel) override;
    private:
        using Self = rkv::KvServer;
        using Raft = sharpen::RaftWrapper<sharpen::IpEndPoint,rkv::RaftMember,rkv::RaftLog,rkv::KeyValueService,rkv::RaftStorage>;

        void OnLeaderRedirect(sharpen::INetStreamChannel &channel) const;

        void OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const;

        void OnPutFail(sharpen::INetStreamChannel &channel);

        void OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnDeleteFail(sharpen::INetStreamChannel &channel);

        void OnAppendEntires(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        void OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf);

        std::shared_ptr<rkv::KeyValueService> app_;
        std::unique_ptr<rkv::RaftGroup> group_;
    public:
        KvServer(sharpen::EventEngine &engine,const rkv::KvServerOption &option);
    
        ~KvServer() noexcept = default;

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