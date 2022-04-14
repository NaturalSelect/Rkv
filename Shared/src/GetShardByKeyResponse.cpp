#include <rkv/GetShardByKeyResponse.hpp>

std::size_t rkv::GetShardByKeyResponse::ComputeSize() const noexcept
{
    bool exist{this->shard_.Exist()};
    std::size_t size{Helper::ComputeSize(exist)};
    if(exist)
    {
        size += Helper::ComputeSize(this->shard_.Get());
    }
    return size;
}

std::size_t rkv::GetShardByKeyResponse::LoadFrom(const char *data,std::size_t size)
{
    std::size_t offset{0};
    bool exist{false};
    std::memcpy(&exist,data,sizeof(exist));
    offset += sizeof(exist);
    if(exist)
    {
        if(offset == size)
        {
            throw sharpen::DataCorruptionException("get shard by key response corruption");
        }
        offset += Helper::LoadFrom(this->shard_.Get(),data + offset,size - offset);
    }
    else
    {
        this->shard_.Reset();
    }
    return offset;
}

std::size_t rkv::GetShardByKeyResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    bool exist{this->shard_.Exist()};
    std::memcpy(data,&exist,sizeof(exist));
    offset += sizeof(exist);
    if(exist)
    {
        offset += Helper::UnsafeStoreTo(this->shard_.Get(),data + offset);
    }
    return offset;
}