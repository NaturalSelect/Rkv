#include <rkv/ShardSet.hpp>

rkv::ShardSet::Iterator rkv::ShardSet::Find(const sharpen::ByteBuffer &key) noexcept
{
    using FnPtr = bool(*)(const rkv::Shard&,const sharpen::ByteBuffer&);
    return std::lower_bound(this->Begin(),this->End(),key,static_cast<FnPtr>(&Self::Compare));
}

rkv::ShardSet::ConstIterator rkv::ShardSet::Find(const sharpen::ByteBuffer &key) const noexcept
{
    using FnPtr = bool(*)(const rkv::Shard&,const sharpen::ByteBuffer&);
    return std::lower_bound(this->Begin(),this->End(),key,static_cast<FnPtr>(&Self::Compare));
}

rkv::ShardSet::Iterator rkv::ShardSet::Put(rkv::Shard shard)
{
    // if(this->Empty())
    // {
    //     this->shards_.emplace_back(std::move(shard));
    //     return this->Begin();
    // }
    // auto ite = this->Find(shard.Key());
    // if(ite->Key() == shard.Key())
    // {
    //     ite->Workers() = std::move(shard.Workers());
    //     return ite;
    // }
    // ite = this->shards_.emplace(ite,std::move(shard));
    // return ite;
}

rkv::ShardSet::Iterator rkv::ShardSet::Delete(Iterator ite)
{
    return this->shards_.erase(ite);
}

rkv::ShardSet::Iterator rkv::ShardSet::Delete(const sharpen::ByteBuffer &key)
{
    auto ite = this->Find(key);
    // if(ite->Key() == key)
    // {
    //     ite = this->shards_.erase(ite);
    //     return ite;
    // }
    return ite;
}

std::size_t rkv::ShardSet::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->shards_);
}

std::size_t rkv::ShardSet::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->shards_,data,size);
}

std::size_t rkv::ShardSet::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->shards_,data);
}