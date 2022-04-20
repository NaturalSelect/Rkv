#include <rkv/WorkerClient.hpp>

#include <unordered_set>

std::uint64_t rkv::WorkerClient::GetVersion(sharpen::NetStreamChannelPtr channel)
{
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetVersionRequest,0)};
    Self::WriteMessage(channel,header);
    sharpen::ByteBuffer buf;
    rkv::GetVersionResponse response;
    Self::ReadMessage(channel,rkv::MessageType::GetVersionResponse,buf);
    response.Unserialize().LoadFrom(buf);
    return response.GetVersion();
}

sharpen::ByteBuffer rkv::WorkerClient::GetValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
{
    rkv::GetRequest request;
    request.Key() = std::move(key);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetRequest,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    Self::ReadMessage(channel,rkv::MessageType::GetResponse,buf);
    rkv::GetResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Value();
}

rkv::MotifyResult rkv::WorkerClient::PutKeyValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,sharpen::ByteBuffer value,std::uint64_t version)
{
    rkv::PutRequest request;
    request.SetVersion(version);
    request.Key() = std::move(key);
    request.Value() = std::move(value);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::PutRequest,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    Self::ReadMessage(channel,rkv::MessageType::PutResponse,buf);
    rkv::PutResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.GetResult();
}

rkv::MotifyResult rkv::WorkerClient::DeleteKey(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,std::uint64_t version)
{
    rkv::DeleteRequest request;
    request.SetVersion(version);
    request.Key() = std::move(key);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeleteReqeust,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    Self::ReadMessage(channel,rkv::MessageType::DeleteResponse,buf);
    rkv::DeleteResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.GetResult();
}

sharpen::Optional<sharpen::ByteBuffer> rkv::WorkerClient::Get(sharpen::ByteBuffer key,rkv::GetPolicy policy)
{
    char ip[21] ={};
    sharpen::NetStreamChannelPtr conn{nullptr};
    sharpen::IpEndPoint id;
    if(policy == rkv::GetPolicy::Strict)
    {
        this->FillLeaderId();
        id = this->leaderId_.Get();
    }
    else
    {
        id = this->GetRandomId();
    }
    std::unordered_set<sharpen::IpEndPoint> set;
    do
    {
        try
        {
            conn = this->GetConnection(id);
        }
        catch(const std::exception &e)
        {
            id.GetAddrString(ip,sizeof(ip));
            std::fprintf(stderr,"[Error]Cannot connect with %s:%hu because %s\n",ip,id.GetPort(),e.what());
            if(policy == rkv::GetPolicy::Strict)
            {
                throw;
            }
            set.emplace(id);
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
    } while (!conn);
    sharpen::ByteBuffer val;
    try
    {
        val = Self::GetValue(conn,std::move(key));
    }
    catch(const std::exception&)
    {
        this->EraseConnection(id);
        if(policy == rkv::GetPolicy::Strict)
        {
            id.GetAddrString(ip,sizeof(ip));
            std::fprintf(stderr,"[Error]Leader %s:%hu maybe dead\n",ip,id.GetPort());
            this->leaderId_.Reset();
        }
        throw;
    }
    if(val.Empty())
    {
        return sharpen::EmptyOpt;
    }
    return val;
}

void rkv::WorkerClient::FillLeaderVersion()
{
    this->FillLeaderId();
    if(!this->versionOfLeader_)
    {
        sharpen::NetStreamChannelPtr conn = this->GetConnection(this->leaderId_.Get());
        this->versionOfLeader_ = Self::GetVersion(conn);
    }
}

rkv::MotifyResult rkv::WorkerClient::Put(sharpen::ByteBuffer key,sharpen::ByteBuffer value)
{
    rkv::MotifyResult result{rkv::MotifyResult::NotCommit};
    try
    {
        this->FillLeaderVersion();
        sharpen::NetStreamChannelPtr conn = this->GetConnection(this->leaderId_.Get());
        result = Self::PutKeyValue(conn,key,value,this->versionOfLeader_);
    }
    catch(const std::exception &e)
    {
        std::fprintf(stderr,"[Error]A error has occured %s\n",e.what());
    }
    if(result == rkv::MotifyResult::NotCommit)
    {
        if(this->leaderId_.Exist())
        {
            this->EraseConnection(this->leaderId_.Get());
            this->leaderId_.Reset();
        }
        this->versionOfLeader_ = 0;
    }
    return result;
}

rkv::MotifyResult rkv::WorkerClient::Delete(sharpen::ByteBuffer key)
{
    rkv::MotifyResult result{rkv::MotifyResult::NotCommit};
    try
    {
        this->FillLeaderVersion();
        sharpen::NetStreamChannelPtr conn = this->GetConnection(this->leaderId_.Get());
        result = Self::DeleteKey(conn,key,this->versionOfLeader_);
    }
    catch(const std::exception &e)
    {
        std::fprintf(stderr,"[Error]A error has occured %s\n",e.what());
    }
    if(result == rkv::MotifyResult::NotCommit)
    {
        if(this->leaderId_.Exist())
        {
            this->EraseConnection(this->leaderId_.Get());
            this->leaderId_.Reset();
        }
        this->versionOfLeader_ = 0;
    }
    return result;
}