#include <rkv/RaftMember.hpp>

#include <rkv/MessageHeader.hpp>
#include <rkv/AppendEntriesRequest.hpp>
#include <rkv/AppendEntriesResponse.hpp>
#include <rkv/VoteRequest.hpp>
#include <rkv/VoteResponse.hpp>

void rkv::RaftMember::ConnectToEndPoint()
{
    if(!this->channel_)
    {
        sharpen::NetStreamChannelPtr channel = sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip);
        sharpen::IpEndPoint ep{0,0};
        channel->Bind(ep);
        channel->Register(*this->engine_);
        this->channel_ = std::move(channel);
        //if node's process pause
        //we fail in here
        this->channel_->ConnectAsync(this->id_);
    }
}

void rkv::RaftMember::Cancel()
{
    if(this->channel_)
    {
        this->channel_->Cancel();
        this->channel_->Close();
    }
}

void rkv::RaftMember::DoProposeAsync(rkv::LogProposal *proposal,sharpen::Future<bool> *result)
{
    assert(result != nullptr);
    assert(proposal != nullptr);
    char ip[21] = {};
    this->id_.GetAddrString(ip,sizeof(ip));
    try
    {
        this->ConnectToEndPoint();
        rkv::AppendEntriesRequest request;
        request.Group() = proposal->Group();
        request.SetCommitIndex(proposal->GetCommitIndex());
        request.SetLeaderTerm(proposal->GetTerm());
        request.SetPrevLogIndex(this->currentIndex_);
        request.LeaderId() = proposal->Id();
        if(this->currentIndex_ != 0)
        {
            request.SetPrevLogTerm(proposal->GetStorage().GetLog(this->currentIndex_).GetTerm());
        }
        else
        {
            request.SetPrevLogTerm(0);
        }
        std::uint64_t lastIndex = (std::min)(this->currentIndex_ + Self::appendLimit_,proposal->GetStorage().GetLastLogIndex());
        for (std::uint64_t i = this->currentIndex_ + 1; i <= lastIndex; ++i)
        {
            request.Logs().emplace_back(proposal->GetStorage().GetLog(i));
        }
        sharpen::ByteBuffer buf;
        request.StoreTo(buf);
        rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::AppendEntriesRequest,buf.GetSize())};
        this->channel_->WriteObjectAsync(header);
        this->channel_->WriteAsync(buf);
        if(this->channel_->ReadObjectAsync(header) != sizeof(header))
        {
            sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
        }
        if(rkv::GetMessageType(header) != rkv::MessageType::AppendEntriesResponse)
        {
            throw std::logic_error("invalid appentires response");
        }
        buf.ExtendTo(header.size_);
        std::size_t size{this->channel_->ReadFixedAsync(buf)};
        if(size != header.size_)
        {
            sharpen::ThrowSystemError(sharpen::ErrorConnectionAborted);
        }
        rkv::AppendEntriesResponse response;
        response.Unserialize().LoadFrom(buf);
        if(response.Success())
        {
            std::printf("[Info]Append entires to %s:%hu current index %llu -> %llu (%llu)\n",ip,this->id_.GetPort(),this->currentIndex_,lastIndex,proposal->Group().Exist() ? proposal->Group().Get():0);
            this->currentIndex_ = lastIndex;
        }
        else
        {
            std::fprintf(stderr,"[Error]Fail to append entires to %s:%hu term %llu -> %llu current index %llu -> %llu (%llu)\n",ip,this->id_.GetPort(),request.GetLeaderTerm(),response.GetTerm(),this->currentIndex_,response.GetAppiledIndex(),proposal->Group().Exist() ? proposal->Group().Get():0);
            this->currentIndex_ = response.GetAppiledIndex();
            proposal->SetMaxTerm(response.GetTerm());
            result->Complete(false);
            return;
        }
        result->Complete(true);
        return;
    }
    catch(const std::system_error &e)
    {
        sharpen::ErrorCode errCode{static_cast<sharpen::ErrorCode>(e.code().value())};
        if (errCode == sharpen::ErrorCancel)
        {
            std::printf("[Info]Append entires to %s:%hu operation canceled (%llu)\n",ip,this->id_.GetPort(),proposal->Group().Exist() ? proposal->Group().Get():0);
        }
        else
        {
            std::printf("[Error]An error occurred during append entires to %s:%hu %s (%llu)\n",ip,this->id_.GetPort(),e.what(),proposal->Group().Exist() ? proposal->Group().Get():0);
        }
    }
    catch(const std::exception &e)
    {
        std::printf("[Error]An error occurred during append entires to %s:%hu %s (%llu)\n",ip,this->id_.GetPort(),e.what(),proposal->Group().Exist() ? proposal->Group().Get():0);
    }
    this->channel_.reset();
    result->Complete(false);
}

void rkv::RaftMember::DoProposeAsync(rkv::VoteProposal *proposal,sharpen::Future<bool> *result)
{
    assert(result != nullptr);
    assert(proposal != nullptr);
    char ip[21] = {};
    this->id_.GetAddrString(ip,sizeof(ip));
    try
    {
        this->ConnectToEndPoint();
        rkv::VoteRequest reqeust;
        reqeust.Group() = proposal->Group();
        reqeust.Id() = proposal->Id();
        reqeust.SetTerm(proposal->GetTerm());
        reqeust.SetLastIndex(proposal->GetLastIndex());
        reqeust.SetLastTerm(proposal->GetLastTerm());
        sharpen::ByteBuffer buf;
        reqeust.Serialize().StoreTo(buf);
        rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::VoteRequest,buf.GetSize())};
        this->channel_->WriteObjectAsync(header);
        this->channel_->WriteAsync(buf);
        if(this->channel_->ReadObjectAsync(header) != sizeof(header))
        {
            std::printf("[Error]Channel %s:%hu close\n",ip,this->id_.GetPort());
            sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
        }
        if(rkv::GetMessageType(header) != rkv::MessageType::VoteResponse)
        {
            throw std::logic_error("invalid request vote response");
        }
        buf.ExtendTo(header.size_);
        std::size_t size{this->channel_->ReadFixedAsync(buf)};
        if(size != header.size_)
        {
            sharpen::ThrowSystemError(sharpen::ErrorConnectionAborted);
        }
        rkv::VoteResponse response;
        response.Unserialize().LoadFrom(buf);
        if(response.Success())
        {
            std::printf("[Info]Got vote from %s:%hu (%llu)\n",ip,this->id_.GetPort(),proposal->Group().Exist() ? proposal->Group().Get():0);
            if(proposal->Callback())
            {
                proposal->Callback()();
            }
        }
        else
        {
            std::printf("[Info]Cannot get vote from %s:%hu term is %llu (%llu)\n",ip,this->id_.GetPort(),response.GetTerm(),proposal->Group().Exist() ? proposal->Group().Get():0);
            proposal->SetMaxTerm(response.GetTerm());
        }
        result->Complete(response.Success());
        return;
    }
    catch(const std::system_error &e)
    {
        sharpen::ErrorCode errCode{static_cast<sharpen::ErrorCode>(e.code().value())};
        if (errCode == sharpen::ErrorCancel)
        {
            std::printf("[Info]Request vote from %s:%hu operation cancel (%llu)\n",ip,this->id_.GetPort(),proposal->Group().Exist() ? proposal->Group().Get():0);
        }
        else
        {
            std::printf("[Error]An error occurred during request vote from %s:%hu %s (%llu)\n",ip,this->id_.GetPort(),e.what(),proposal->Group().Exist() ? proposal->Group().Get():0);
        }
    }
    catch(const std::exception &e)
    {
        std::printf("[Error]An error occurred during request vote from %s:%hu %s (%llu)\n",ip,this->id_.GetPort(),e.what(),proposal->Group().Exist() ? proposal->Group().Get():0);
    }
    this->channel_.reset();
    result->Complete(false);
}

void rkv::RaftMember::ProposeAsync(rkv::LogProposal &proposal,sharpen::Future<bool> &result)
{
    using FnPtr = void(Self::*)(rkv::LogProposal *,sharpen::Future<bool> *);
    this->engine_->Launch(static_cast<FnPtr>(&Self::DoProposeAsync),this,&proposal,&result);
}

void rkv::RaftMember::ProposeAsync(rkv::VoteProposal &proposal,sharpen::Future<bool> &result)
{
    using FnPtr = void(Self::*)(rkv::VoteProposal *,sharpen::Future<bool> *);
    this->engine_->Launch(static_cast<FnPtr>(&Self::DoProposeAsync),this,&proposal,&result);
}