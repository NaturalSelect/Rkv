#include <rkv/DeriveShardRequest.hpp>

std::size_t rkv::DeriveShardRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->source_};
    size += Helper::ComputeSize(builder);
    size += Helper::ComputeSize(this->beginKey_);
    return size;
}

std::size_t rkv::DeriveShardRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 2)
    {
        throw std::invalid_argument("invalid adjust shard request buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    if(size == offset)
    {
        throw sharpen::DataCorruptionException("adjust shard request corruption");
    }
    offset += Helper::LoadFrom(this->beginKey_,data + offset,size - offset);
    return offset;
}

std::size_t rkv::DeriveShardRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->source_};
    offset += Helper::UnsafeStoreTo(builder,data);
    offset += Helper::UnsafeStoreTo(this->beginKey_,data + offset);
    return offset;
}