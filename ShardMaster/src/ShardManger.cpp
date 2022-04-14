#include <rkv/ShardManger.hpp>

sharpen::ByteBuffer rkv::ShardManger::shardCountKey_;

std::once_flag rkv::ShardManger::flag_;

void rkv::ShardManger::InitKeys()
{
    shardCountKey_.ExtendTo(2);
    shardCountKey_[0] = 's';
    shardCountKey_[1] = 'c';
}

sharpen::ByteBuffer rkv::ShardManger::FormatShardKey(std::uint64_t index)
{
    sharpen::Varuint64 builder{index};
    sharpen::ByteBuffer key{1};
    builder.StoreTo(key,1);
    key[0] = 's';
    return key;
}

rkv::ShardManger::ShardManger(rkv::KeyValueService &service)
    :service_(&service)
    ,shards_()
{
    using FnPtr = void(*)();
    std::call_once(this->flag_,static_cast<FnPtr>(&Self::InitKeys));
    this->Flush();
}

void rkv::ShardManger::Flush()
{
    this->shards_.clear();
    sharpen::Optional<sharpen::ByteBuffer> indexBuf{this->service_->TryGet(Self::shardCountKey_)};
    if(indexBuf.Exist())
    {
        std::uint64_t count{indexBuf.Get().As<std::uint64_t>()};
        this->shards_.reserve(count);
        for (std::uint64_t i = 0; i != count; ++i)
        {
            rkv::Shard shard;
            sharpen::ByteBuffer buf{this->service_->Get(this->FormatShardKey(i))};
            shard.Unserialize().LoadFrom(buf);
            this->shards_.emplace_back(std::move(shard));
        }
    }
}

const rkv::Shard *rkv::ShardManger::GetShardPtr(const sharpen::ByteBuffer &key) const noexcept
{
    if(this->shards_.empty())
    {
        return nullptr;
    }
    const rkv::Shard *shard{nullptr};
    for (auto begin = this->shards_.begin(),end = this->shards_.end(); begin != end; ++begin)
    {
        if(begin->BeginKey() == key)
        {
            return std::addressof(*begin);
        }
        else if(begin->BeginKey() < key)
        {
            if(!shard)
            {
                shard = std::addressof(*begin);
            }
            else if(shard->BeginKey() < begin->BeginKey())
            {
                shard = std::addressof(*begin);
            }
        }
    }
    return shard;
}