#include <rkv/ShardClient.hpp>

sharpen::Optional<rkv::Shard> rkv::ShardClient::GetShardByKey(const sharpen::ByteBuffer &key,rkv::GetPolicy policy)
{
    sharpen::Optional<rkv::Shard> result{sharpen::EmptyOpt};
    if(policy == rkv::GetPolicy::Relaxed)
    {
        auto ite = this->shardMap_.find(key);
        if(ite != this->shardMap_.begin())
        {
            --ite;
            result.Construct(ite->second);
        }
    }
    if(!result.Exist())
    {
        sharpen::Optional<rkv::Shard> shard{this->masterClient_.GetShard(key)};
        if(shard.Exist())
        {
            std::printf("[Info]Load shard %llu\n",shard.Get().GetId());
            result.Construct(shard.Get());
            this->shardMap_.emplace(shard.Get().BeginKey(),shard.Get());
        }
    }
    return result;
}

rkv::WorkerClient *rkv::ShardClient::OpenClient(const rkv::Shard &shard)
{
    auto ite = this->clientMap_.find(shard.GetId());
    if(ite == this->clientMap_.end())
    {
        std::unique_ptr<rkv::WorkerClient> client{new rkv::WorkerClient{*this->engine_,shard.Workers().begin(),shard.Workers().end(),this->restoreTimeout_,this->maxTimeoutCount_,shard.GetId()}};
        rkv::WorkerClient *clientPtr{client.get()};
        this->clientMap_.emplace(shard.GetId(),std::move(client));
        return clientPtr;
    }
    return ite->second.get();
}

void rkv::ShardClient::ClearCache()
{
    this->shardMap_.clear();
}

sharpen::Optional<sharpen::ByteBuffer> rkv::ShardClient::Get(sharpen::ByteBuffer key,rkv::GetPolicy policy)
{
    sharpen::Optional<rkv::Shard> shard{this->GetShardByKey(key,policy)};
    sharpen::Optional<sharpen::ByteBuffer> value{sharpen::EmptyOpt};
    if(shard.Exist())
    {
        rkv::WorkerClient *client{this->OpenClient(shard.Get())};
        return client->Get(key,policy);
    }
    return value;
}

rkv::MotifyResult rkv::ShardClient::Put(sharpen::ByteBuffer key,sharpen::ByteBuffer value)
{
    sharpen::Optional<rkv::Shard> shard{this->GetShardByKey(key,rkv::GetPolicy::Relaxed)};
    rkv::MotifyResult result{rkv::MotifyResult::NotCommit};
    if(shard.Exist())
    {
        rkv::WorkerClient *client{this->OpenClient(shard.Get())};
        result = client->Put(key,value);
        if(result == rkv::MotifyResult::NotCommit)
        {
            shard = this->GetShardByKey(key,rkv::GetPolicy::Strict);
            if(shard.Exist())
            {
                client = this->OpenClient(shard.Get());
                result = client->Put(std::move(key),std::move(value));
            }
        }
    }
    return result;
}

rkv::MotifyResult rkv::ShardClient::Delete(sharpen::ByteBuffer key)
{
    sharpen::Optional<rkv::Shard> shard{this->GetShardByKey(key,rkv::GetPolicy::Relaxed)};
    rkv::MotifyResult result{rkv::MotifyResult::NotCommit};
    if(shard.Exist())
    {
        rkv::WorkerClient *client{this->OpenClient(shard.Get())};
        result = client->Delete(key);
        if(result == rkv::MotifyResult::NotCommit)
        {
            shard = this->GetShardByKey(key,rkv::GetPolicy::Strict);
            if(shard.Exist())
            {
                client = this->OpenClient(shard.Get());
                result = client->Delete(std::move(key));
            }
        }
    }
    return result;
}