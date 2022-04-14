#include <rkv/Shard.hpp>

std::size_t rkv::Shard::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->id_};
    size += Helper::ComputeSize(builder);
    size += Helper::ComputeSize(this->beginKey_);
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
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->id_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("shard corruption");
    }
    offset += Helper::LoadFrom(this->beginKey_,data + offset,size - offset);
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
    sharpen::Varuint64 builder{this->id_};
    offset += Helper::UnsafeStoreTo(builder,data);
    offset += Helper::UnsafeStoreTo(this->beginKey_,data + offset);
    offset += Helper::UnsafeStoreTo(this->workers_,data + offset);
    return offset;
}