#include <rkv/MasterClient.hpp>

#include <rkv/GetShardByIdRequest.hpp>
#include <rkv/GetShardByIdResponse.hpp>
#include <rkv/GetCompletedMigrationsRequest.hpp>
#include <rkv/GetCompletedMigrationsResponse.hpp>
#include <rkv/GetMigrationsRequest.hpp>
#include <rkv/GetMigrationsResponse.hpp>
#include <rkv/GetShardByKeyRequest.hpp>
#include <rkv/GetShardByKeyResponse.hpp>
#include <rkv/DeriveShardRequest.hpp>
#include <rkv/DeriveShardResponse.hpp>
#include <rkv/CompleteMigrationRequest.hpp>
#include <rkv/CompleteMigrationResponse.hpp>

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
