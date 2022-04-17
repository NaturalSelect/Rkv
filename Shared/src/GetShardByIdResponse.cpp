#include <rkv/GetShardByIdResponse.hpp>

std::size_t rkv::GetShardByIdResponse::ComputeSize() const noexcept
{
    bool exist{this->shard_.Exist()};
    std::size_t size{0};
    size += Helper::ComputeSize(exist);
    if(exist)
    {
        size += Helper::ComputeSize(this->shard_.Get());
    }
    return size;
}

std::size_t rkv::GetShardByIdResponse::LoadFrom(const char *data,std::size_t size)
{
    std::size_t offset{0};
    bool exist;
    offset += Helper::LoadFrom(exist,data,size);
    if(exist)
    {
        if(size == offset)
        {
            throw sharpen::DataCorruptionException("get shard by id response corruption");
        }
        this->shard_.Construct();
        offset += Helper::LoadFrom(this->shard_.Get(),data + offset,size - offset);
    }
    return offset;
}

std::size_t rkv::GetShardByIdResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    bool exist{this->shard_.Exist()};
    offset += Helper::UnsafeStoreTo(exist,data);
    if (exist)
    {
        offset += Helper::UnsafeStoreTo(this->shard_.Get(),data + offset);
    }
    return offset;
}