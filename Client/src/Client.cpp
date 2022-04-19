#include <rkv/Client.hpp>

#include <unordered_set>

rkv::Client::Client(Self &&other) noexcept
    :engine_(other.engine_)
    ,random_(std::move(other.random_))
    ,distribution_(std::move(other.distribution_))
    ,leaderId_(std::move(other.leaderId_))
    ,serverMap_(std::move(other.serverMap_))
    ,timer_(std::move(other.timer_))
    ,restoreTimeout_(other.restoreTimeout_)
    ,maxTimeoutCount_(other.maxTimeoutCount_)
{}

sharpen::IpEndPoint rkv::Client::GetRandomId() const noexcept
{
    assert(!this->serverMap_.empty());
    std::size_t index{this->distribution_(this->random_) - 1};
    auto ite = sharpen::IteratorForward(this->serverMap_.begin(),index);
    return ite->first;
}

sharpen::NetStreamChannelPtr rkv::Client::MakeConnection(sharpen::EventEngine &engine,const sharpen::IpEndPoint &id)
{
    sharpen::NetStreamChannelPtr conn = sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip);
    sharpen::IpEndPoint ep{0,0};
    conn->Bind(ep);
    conn->Register(engine);
    conn->ConnectAsync(id);
    return conn;
}

void rkv::Client::WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header)
{
    std::size_t sz{channel->WriteObjectAsync(header)};
    if(sz != sizeof(header))
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

void rkv::Client::WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header,const sharpen::ByteBuffer &request)
{
    Self::WriteMessage(channel,header);
    std::size_t sz{channel->WriteAsync(request)};
    if(sz != request.GetSize())
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

void rkv::Client::ReadMessage(sharpen::NetStreamChannelPtr channel,rkv::MessageType expectedType,sharpen::ByteBuffer &response)
{
    rkv::MessageHeader header;
    std::size_t sz{channel->ReadObjectAsync(header)};
    if(sz != sizeof(header))
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
    if(rkv::GetMessageType(header) != expectedType)
    {
        throw std::logic_error("got unexpected response");
    }
    response.ExtendTo(sharpen::IntCast<std::size_t>(header.size_));
    sz = channel->ReadFixedAsync(response);
    if(sz != response.GetSize())
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

sharpen::NetStreamChannelPtr rkv::Client::GetConnection(const sharpen::IpEndPoint &id) const
{
    auto ite = this->serverMap_.find(id);
    assert(ite != this->serverMap_.end());
    if(!ite->second)
    {
        ite->second = Self::MakeConnection(*this->engine_,id);
    }
    return ite->second;
}

void rkv::Client::EraseConnection(const sharpen::IpEndPoint &id)
{
    auto ite = this->serverMap_.find(id);
    assert(ite != this->serverMap_.end());
    ite->second.reset();
}

sharpen::Optional<sharpen::IpEndPoint> rkv::Client::GetLeaderId(sharpen::NetStreamChannelPtr channel,std::uint64_t group)
{
    rkv::LeaderRedirectRequest request;
    request.Group().Construct(group);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectRequest,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    try
    {
        Self::ReadMessage(channel,rkv::MessageType::LeaderRedirectResponse,buf);
    }
    catch(const std::exception &)
    {
        throw std::logic_error("operation has been cancel");
    }
    rkv::LeaderRedirectResponse response;
    response.Unserialize().LoadFrom(buf);
    if(response.KnowLeader())
    {
        char ip[21] = {};
        response.Endpoint().GetAddrString(ip,sizeof(ip));
        std::printf("[Info]Got leader id %s:%hu\n",ip,response.Endpoint().GetPort());
        return response.Endpoint();
    }
    std::puts("[Info]Cannot get leader id");
    return sharpen::EmptyOpt;
}

sharpen::Optional<sharpen::IpEndPoint> rkv::Client::GetLeaderId(sharpen::NetStreamChannelPtr channel)
{
    rkv::LeaderRedirectRequest request;
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectRequest,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    try
    {
        Self::ReadMessage(channel,rkv::MessageType::LeaderRedirectResponse,buf);
    }
    catch(const std::exception &)
    {
        throw std::logic_error("operation has been cancel");
    }
    rkv::LeaderRedirectResponse response;
    response.Unserialize().LoadFrom(buf);
    if(response.KnowLeader())
    {
        char ip[21] = {};
        response.Endpoint().GetAddrString(ip,sizeof(ip));
        std::printf("[Info]Got leader id %s:%hu\n",ip,response.Endpoint().GetPort());
        return response.Endpoint();
    }
    std::puts("[Info]Cannot get leader id");
    return sharpen::EmptyOpt;
}

void rkv::Client::FillLeaderId()
{
    if(this->leaderId_.Exist())
    {
        return;
    }
    auto conn{this->MakeRandomConnection()};
    sharpen::IpEndPoint id;
    conn->GetRemoteEndPoint(id);
    char ip[21] = {};
    id.GetAddrString(ip,sizeof(ip));
    std::size_t count{0};
    do
    {
        try
        {
            sharpen::Optional<sharpen::IpEndPoint> tmp;
            if(this->group_.Exist())
            {
                tmp =  Self::GetLeaderId(conn,this->group_.Get());
            }
            else
            {
                tmp = Self::GetLeaderId(conn);
            }
            if(tmp.Exist())
            {
                this->leaderId_.Construct(tmp.Get());
            }
            if(!this->leaderId_.Exist())
            {
                if(count == this->maxTimeoutCount_)
                {
                    throw std::logic_error("cannot find leader");
                }
                std::printf("[Info]Waiting election complete(%zu/%zu)\n",count,this->maxTimeoutCount_);
                this->timer_->Await(this->restoreTimeout_);
                ++count;
            }
        }
        catch(const std::logic_error &)
        {
            throw;
        }
        catch(const std::exception &e)
        {
            std::fprintf(stderr,"[Error]Cannot fill leader id because %s\n",e.what());
            std::printf("[Info]Drop drity channel %s:%hu\n",ip,id.GetPort());
            this->EraseConnection(id);
            conn = this->MakeRandomConnection();
            conn->GetRemoteEndPoint(id);
            id.GetAddrString(ip,sizeof(ip));
        }
    } while(!this->leaderId_.Exist());
}

sharpen::NetStreamChannelPtr rkv::Client::MakeRandomConnection() const
{
    char ip[21] = {};
    sharpen::NetStreamChannelPtr conn{nullptr};
    sharpen::IpEndPoint id{this->GetRandomId()};
    std::unordered_set<sharpen::IpEndPoint> set;
    do
    {
        try
        {
            conn = this->GetConnection(id);
        }
        catch(const std::exception &e)
        {
            set.emplace(id);
            id.GetAddrString(ip,sizeof(ip));
            std::fprintf(stderr,"[Error]Cannot connect with %s:%hu because %s\n",ip,id.GetPort(),e.what());
            if(set.size() == this->serverMap_.size())
            {
                throw;
            }
            auto tmp{this->GetRandomId()};
            while(set.count(tmp))
            {
                tmp = GetRandomId();
            }
            id = tmp;
        }
    } while(!conn);
    return conn;
}