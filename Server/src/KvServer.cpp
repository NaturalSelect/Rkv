#include <rkv/KvServer.hpp>

#include <sharpen/Quorum.hpp>
#include <sharpen/FileOps.hpp>

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

rkv::KvServer::KvServer(sharpen::EventEngine &engine,const rkv::KvServerOption &option)
    :sharpen::TcpServer(sharpen::AddressFamily::Ip,option.SelfId(),engine)
    ,app_(nullptr)
    ,group_(nullptr)
{
    //make directories
    sharpen::MakeDirectory("./Storage");
    this->app_ = std::make_shared<rkv::KeyValueService>(engine,"./Storage/Kvdb");
    this->group_.reset(new rkv::RaftGroup{engine,option.SelfId(),rkv::RaftStorage{engine,"./Storage/Raft"},this->app_});
    if(!this->group_)
    {
        throw std::bad_alloc();
    }
    std::uint64_t lastAppiled{this->group_->Raft().GetLastApplied()};
    for (auto begin = option.MembersBegin(),end = option.MembersEnd(); begin != end; ++begin)
    {
        rkv::RaftMember member{*begin,engine};
        member.SetCurrentIndex(lastAppiled);
        this->group_->Raft().Members().emplace(*begin,std::move(member));
    }
}

void rkv::KvServer::OnLeaderRedirect(sharpen::INetStreamChannel &channel) const
{
    rkv::LeaderRedirectResponse response;
    response.SetKnowLeader(this->group_->Raft().KnowLeader());
    if(response.KnowLeader())
    {
        response.Endpoint() = this->group_->Raft().GetLeaderId();
    }
    char buf[sizeof(bool) + sizeof(sharpen::IpEndPoint)];
    std::size_t sz{response.StoreTo(buf,sizeof(buf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(buf,sz);
}

void rkv::KvServer::OnGet(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf) const
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

void rkv::KvServer::OnPutFail(sharpen::INetStreamChannel &channel)
{
    rkv::PutResponse response{rkv::MotifyResult::NotCommit};
    char resBuf[sizeof(std::uint64_t)];
    std::size_t sz{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::PutResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,sz);
}

void rkv::KvServer::OnPut(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    if(this->group_->Raft().GetRole() != sharpen::RaftRole::Leader)
    {
        return this->OnPutFail(channel);
    }
    rkv::PutRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::RaftLog log;
    log.SetOperation(rkv::RaftLog::Operation::Put);
    log.Key() = std::move(request.Key());
    log.Value() = std::move(request.Value());
    std::fputs("[Info]Try to put key is ",stdout);
    for (std::size_t i = 0; i != log.Key().GetSize(); ++i)
    {
        std::putchar(log.Key()[i]);
    }
    std::fputs(" value is ",stdout);
    for (std::size_t i = 0; i != log.Value().GetSize(); ++i)
    {
        std::putchar(log.Value()[i]);
    }
    std::putchar('\n');
    bool result{false};
    std::size_t commitSize{0};
    this->group_->DelayCycle();
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
        std::uint64_t index{this->group_->Raft().GetLastIndex()};
        log.SetIndex(index + 1);
        log.SetTerm(this->group_->Raft().GetCurrentTerm());
        this->group_->Raft().AppendLog(std::move(log));
        do
        {
            commitSize = 0;
            //result = this->ProposeAppendEntires();
            result = this->group_->ProposeAppendEntires();
            if(!result)
            {
                break;
            }
            for (auto begin = this->group_->Raft().Members().begin(),end = this->group_->Raft().Members().end(); begin != end; ++begin)
            {
                if(begin->second.GetCurrentIndex() >= index + 1)
                {
                    commitSize += 1;
                }
            }
        }
        while(commitSize < this->group_->Raft().MemberMajority());
        if(result)
        {
            this->group_->Raft().SetCommitIndex(index + 1);
            this->group_->Raft().ApplyLogs(Raft::LostPolicy::Ignore);
        }
    }
    rkv::PutResponse response{result ? rkv::MotifyResult::Appiled:rkv::MotifyResult::Commited};
    char resBuf[sizeof(std::uint64_t)];
    std::size_t sz{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::PutResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,sz);
}

void rkv::KvServer::OnDeleteFail(sharpen::INetStreamChannel &channel)
{
    rkv::PutResponse response{rkv::MotifyResult::NotCommit};
    char resBuf[sizeof(std::uint64_t)];
    std::size_t sz{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeleteResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,sz);
}

void rkv::KvServer::OnDelete(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    if(this->group_->Raft().GetRole() != sharpen::RaftRole::Leader)
    {
        return this->OnDeleteFail(channel);
    }
    rkv::DeleteRequest request;
    request.Unserialize().LoadFrom(buf);
    rkv::RaftLog log;
    log.SetOperation(rkv::RaftLog::Operation::Delete);
    log.Key() = std::move(request.Key());
    bool result{false};
    std::size_t commitSize{0};
    {
        std::unique_lock<sharpen::AsyncMutex> lock{this->group_->GetRaftLock()};
        std::uint64_t index{this->group_->Raft().GetLastIndex()};
        log.SetIndex(index + 1);
        log.SetTerm(this->group_->Raft().GetCurrentTerm());
        this->group_->Raft().AppendLog(std::move(log));
        do
        {
            commitSize = 0;
            result = this->group_->ProposeAppendEntires();
            if(!result)
            {
                break;
            }
            for (auto begin = this->group_->Raft().Members().begin(),end = this->group_->Raft().Members().end(); begin != end; ++begin)
            {
                if(begin->second.GetCurrentIndex() >= index + 1)
                {
                    commitSize += 1;
                }
            }
        }
        while(commitSize < this->group_->Raft().MemberMajority());
        if(result)
        {
            this->group_->Raft().SetCommitIndex(index + 1);
            this->group_->Raft().ApplyLogs(Raft::LostPolicy::Ignore);
        }
    }
    rkv::PutResponse response{result ? rkv::MotifyResult::Appiled:rkv::MotifyResult::Commited};
    char resBuf[sizeof(std::uint64_t)];
    std::size_t sz{response.Serialize().StoreTo(resBuf,sizeof(resBuf))};
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeleteResponse,sz)};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf,sz);
}

void rkv::KvServer::OnAppendEntires(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    this->group_->DelayCycle();
    rkv::AppendEntiresRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{false};
    std::uint64_t currentTerm{0};
    std::uint64_t lastAppiled{0};
    std::printf("[Info]Channel want to append entries to host term is %llu prev log index is %llu prev log term is %llu commit index is %llu\n",request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
    {
        result = this->group_->Raft().AppendEntries(request.Logs().begin(),request.Logs().end(),request.LeaderId(),request.GetLeaderTerm(),request.GetPrevLogIndex(),request.GetPrevLogTerm(),request.GetCommitIndex());
        currentTerm = this->group_->Raft().GetCurrentTerm();
        lastAppiled = this->group_->Raft().GetLastApplied();
    }
    if(result)
    {
        std::printf("[Info]Leader append %zu entires to host\n",request.Logs().size());
    }
    else
    {
        std::printf("[Info]Channel want to append %zu entires to host but failure\n",request.Logs().size());
    }
    rkv::AppendEntiresResponse response;
    response.SetResult(result);
    response.SetTerm(currentTerm);
    response.SetAppiledIndex(lastAppiled);
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::AppendEntiresResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::KvServer::OnRequestVote(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &buf)
{
    rkv::VoteRequest request;
    request.Unserialize().LoadFrom(buf);
    bool result{false};
    std::uint64_t currentTerm{0};
    {
        result = this->group_->Raft().RequestVote(request.GetTerm(),request.Id(),request.GetLastIndex(),request.GetLastTerm());
        currentTerm = this->group_->Raft().GetCurrentTerm();
    }
    std::printf("[Info]Candidate want to get vote from host term is %llu last log index is %llu last log term is %llu\n",request.GetTerm(),request.GetLastIndex(),request.GetLastTerm());
    if(result)
    {
        std::puts("[Info]Candidate got vote from host");
    }
    else
    {
        std::puts("[Info]Candidate cannot get vote from host");
    }
    rkv::VoteResponse response;
    response.SetResult(result);
    response.SetTerm(currentTerm);
    sharpen::ByteBuffer resBuf;
    response.Serialize().StoreTo(resBuf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::VoteResponse,resBuf.GetSize())};
    channel.WriteObjectAsync(header);
    channel.WriteAsync(resBuf);
}

void rkv::KvServer::OnNewChannel(sharpen::NetStreamChannelPtr channel)
{
    try
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
                this->OnDelete(*channel,buf);
                break;
            case rkv::MessageType::AppendEntiresRequest:
                std::printf("[Info]Channel %s:%hu want to append entires\n",ip,ep.GetPort());
                this->OnAppendEntires(*channel,buf);
                break;
            case rkv::MessageType::VoteRequest:
                std::printf("[Info]Channel %s:%hu want to request a vote\n",ip,ep.GetPort());
                this->OnRequestVote(*channel,buf);
                break;
            }
        }
        std::printf("[Info]A channel %s:%hu disconnect with host\n",ip,ep.GetPort());
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]An error has occurred %s\n",e.what());
    }
}