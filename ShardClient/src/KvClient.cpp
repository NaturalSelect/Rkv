#include <rkv/KvClient.hpp>

#include <unordered_set>

sharpen::ByteBuffer rkv::KvClient::GetValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
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

rkv::MotifyResult rkv::KvClient::PutKeyValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,sharpen::ByteBuffer value)
{
    rkv::PutRequest request;
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

rkv::MotifyResult rkv::KvClient::DeleteKey(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
{
    rkv::DeleteRequest request;
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

sharpen::Optional<sharpen::ByteBuffer> rkv::KvClient::Get(sharpen::ByteBuffer key,rkv::GetPolicy policy)
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

rkv::MotifyResult rkv::KvClient::Put(sharpen::ByteBuffer key,sharpen::ByteBuffer value)
{
    rkv::MotifyResult result{rkv::MotifyResult::NotCommit};
    while (result == rkv::MotifyResult::NotCommit)
    {
        try
        {
            this->FillLeaderId();
            sharpen::NetStreamChannelPtr conn = this->GetConnection(this->leaderId_.Get());
            result = Self::PutKeyValue(conn,key,value);
            if(result == rkv::MotifyResult::NotCommit)
            {
                this->leaderId_.Reset();
            }
        }
        catch(const std::exception&)
        {
            if(this->leaderId_.Exist())
            {
                this->EraseConnection(this->leaderId_.Get());
                this->leaderId_.Reset();
            }
            break;
        }
    }
    return result;
}

rkv::MotifyResult rkv::KvClient::Delete(sharpen::ByteBuffer key)
{
    rkv::MotifyResult result{rkv::MotifyResult::NotCommit};
    while (result == rkv::MotifyResult::NotCommit)
    {
        try
        {
            this->FillLeaderId();
            sharpen::NetStreamChannelPtr conn = this->GetConnection(this->leaderId_.Get());
            result = Self::DeleteKey(conn,key);
            if(result == rkv::MotifyResult::NotCommit)
            {
                this->leaderId_.Reset();
            }
        }
        catch(const std::exception&)
        {
            if(this->leaderId_.Exist())
            {
                this->EraseConnection(this->leaderId_.Get());
                this->leaderId_.Reset();
            }
            break;
        }
    }
    return result;
}