#include <rkv/MasterClient.hpp>

sharpen::Optional<rkv::Shard> rkv::MasterClient::GetShardByKey(sharpen::INetStreamChannel &channel,const sharpen::ByteBuffer &key)
{
    rkv::GetShardByKeyRequest request;
    request.Key() = key;
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetShardByKeyRequest,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    Self::ReadMessage(channel,rkv::MessageType::GetShardByKeyResponse,buf);
    rkv::GetShardByKeyResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Shard();
}

sharpen::Optional<rkv::Shard> rkv::MasterClient::GetShard(const sharpen::ByteBuffer &key)
{
    this->FillLeaderId();
    auto conn{this->GetConnection(this->leaderId_.Get())};
    return Self::GetShardByKey(*conn,key);
}

rkv::DeriveResult rkv::MasterClient::DeriveNewShard(sharpen::INetStreamChannel &channel,std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endKey)
{
    rkv::DeriveShardRequest request;
    request.SetSource(source);
    request.BeginKey() = beginKey;
    request.EndKey() = endKey;
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeriveShardRequest,buf.GetSize())};
    Self::WriteMessage(channel,header,buf);
    Self::ReadMessage(channel,rkv::MessageType::DeriveShardReponse,buf);
    rkv::DeriveShardResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.GetResult();
}

rkv::DeriveResult rkv::MasterClient::DeriveShard(std::uint64_t source,const sharpen::ByteBuffer &beginKey,const sharpen::ByteBuffer &endKey)
{
    rkv::DeriveResult result{rkv::DeriveResult::NotCommit};
    while (result == rkv::DeriveResult::NotCommit)
    {
        try
        {
            this->FillLeaderId();
            auto conn{this->GetConnection(this->leaderId_.Get())};
            result = Self::DeriveNewShard(*conn,source,beginKey,endKey);
            if (result == rkv::DeriveResult::NotCommit)
            {
                this->leaderId_.Reset();
            }
        }
        catch(const std::exception &)
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