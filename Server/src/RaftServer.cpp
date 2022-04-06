#include <rkv/RaftServer.hpp>

#include <rkv/MessageHeader.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/AppendEntiresRequest.hpp>
#include <rkv/AppendEntiresResponse.hpp>
#include <rkv/VoteRequest.hpp>
#include <rkv/VoteResponse.hpp>

void rkv::RaftServer::OnLeaderRedirect(sharpen::INetStreamChannel &channel) const
{
    rkv::LeaderRedirectResponse response;
    response.SetKnowLeader(this->raft_->KnowLeader());
    if(response.KnowLeader())
    {
        response.Endpoint() = this->raft_->LeaderId();
    }
    char buf[sizeof(bool) + sizeof(sharpen::IpEndPoint)];
    std::size_t sz{response.StoreTo(buf,sizeof(buf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(buf,sz);
}

void rkv::RaftServer::OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const
{
    rkv::GetRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::GetResponse response;
    sharpen::Optional<sharpen::ByteBuffer> val{this->app_->TryGet(request.Key())};
    if(!val.Exist())
    {
        std::fputs("[Info]Try to get a not-existed key ",stdout);
        for (std::size_t i = 0; i != request.Key().GetSize(); ++i)
        {
            std::putchar(request.Key()[i]);
        }
        std::putchar('\n');
    }
    else
    {
        response.Value() = std::move(val.Get());
        std::fputs("[Info]Try to get a key value pair and key is ",stdout);
        for (std::size_t i = 0; i != request.Key().GetSize(); ++i)
        {
            std::putchar(request.Key()[i]);
        }
        std::fputs(" value is ",stdout);
        for (std::size_t i = 0; i != response.Value().GetSize(); ++i)
        {
            std::putchar(response.Value()[i]);
        }
        std::putchar('\n');
    }
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::RaftServer::OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{

}

void rkv::RaftServer::OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{

}

void rkv::RaftServer::OnAppendEntires(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{

}

void rkv::RaftServer::OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{

}

void rkv::RaftServer::OnNewChannel(sharpen::NetStreamChannelPtr channel)
{
    sharpen::IpEndPoint ep;
    channel->GetRemoteEndPoint(ep);
    char ip[21] = {};
    ep.GetAddrString(ip,sizeof(ip));
    std::printf("[Info]A new channel %s:%hu connect to host\n",ip,ep.GetPort());
    while (1)
    {
        rkv::MessageHeader header;
        std::size_t sz{channel->ReadObjectAsync(header)};
        if(sz != sizeof(header))
        {
            break;
        }
        std::printf("[Info]Receive a new message from %s:%hu type is %llu size is %llu\n",ip,ep.GetPort(),header.type_,header.size_);
        rkv::MessageType type{rkv::GetMessageType(header)};
        if(type == rkv::MessageType::LeaderRedirectRequest)
        {
            std::printf("[Info]Channel %s:%hu ask who is leader\n",ip,ep.GetPort());
            this->OnLeaderRedirect(*channel);
            continue;
        }
        sharpen::ByteBuffer buf{sharpen::IntCast<std::size_t>(header.size_)};
        sz = channel->ReadFixedAsync(buf);
        if(sz != buf.GetSize())
        {
            break;
        }
        switch (type)
        {
        case rkv::MessageType::GetRequest:
            std::printf("[Info]Channel %s:%hu want to get a value\n",ip,ep.GetPort());
            this->OnGet(*channel,buf);
            break;
        case rkv::MessageType::PutRequest:
            std::printf("[Info]Channel %s:%hu want to put a key value pair\n",ip,ep.GetPort());
            this->OnPut(*channel,buf);
            break;
        case rkv::MessageType::DeleteReqeust:
            std::printf("[Info]Channel %s:%hu want to delete a key value pair\n",ip,ep.GetPort());
            break;
        case rkv::MessageType::AppendEntiresRequest:
            std::printf("[Info]Channel %s:%hu want to append entires\n",ip,ep.GetPort());
            break;
        case rkv::MessageType::VoteRequest:
            std::printf("[Info]Channel %s:%hu want to request a vote\n",ip,ep.GetPort());
            break;
        }
    }
    std::printf("[Info]A channel %s:%hu disconnect with host\n",ip,ep.GetPort());
}