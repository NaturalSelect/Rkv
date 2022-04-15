#include <rkv/ShardManger.hpp>

sharpen::ByteBuffer rkv::ShardManger::shardCountKey_;

std::once_flag rkv::ShardManger::flag_;

void rkv::ShardManger::InitKeys()
{
    shardCountKey_.ExtendTo(2);
    shardCountKey_[0] = 's';
    shardCountKey_[1] = 'c';
}

bool rkv::ShardManger::CompareShards(const rkv::Shard &left,const rkv::Shard &right) noexcept
{
    return left.BeginKey() < right.BeginKey();
}

bool rkv::ShardManger::CompareShards(const rkv::Shard &left,const sharpen::ByteBuffer &right) noexcept
{
    return left.BeginKey() < right;
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
        //sort by begin key
        using FnPtr = bool(*)(const rkv::Shard &,const rkv::Shard &);
        std::sort(this->shards_.begin(),this->shards_.end(),static_cast<FnPtr>(&Self::CompareShards));
    }
}

const rkv::Shard *rkv::ShardManger::GetShardPtr(const sharpen::ByteBuffer &key) const noexcept
{
    const rkv::Shard *shard{nullptr};
    if(!this->shards_.empty())
    {
        using FnPtr = bool(*)(const rkv::Shard&,const sharpen::ByteBuffer&);
        auto ite = std::lower_bound(this->shards_.begin(),this->shards_.end(),key,static_cast<FnPtr>(&Self::CompareShards));
        if(ite == this->shards_.end())
        {
            ite = sharpen::IteratorBackward(ite,1);
        }
        else if(ite != this->shards_.begin() && ite->BeginKey() > key)
        {
            ite = sharpen::IteratorBackward(ite,1);
        }
        if(ite->BeginKey() <= key)
        {
            shard = std::addressof(*ite);
        }
    }
    return shard;
}