#include <rkv/Shard.hpp>

std::size_t rkv::Shard::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->key_);
    size += Helper::ComputeSize(this->workers_);
    return size;
}

std::size_t rkv::Shard::LoadFrom(const char *data,std::size_t size)
{
    if(size < 3)
    {
        throw std::invalid_argument("invalid shard buffer");
    }
    std::size_t offset{0};
    offset += Helper::LoadFrom(this->key_,data,size);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("shard corruption");
    }
    offset += Helper::LoadFrom(this->workers_,data + offset,size - offset);
    return offset;
}

std::size_t rkv::Shard::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += Helper::UnsafeStoreTo(this->key_,data);
    offset += Helper::UnsafeStoreTo(this->workers_,data + offset);
    return offset;
}