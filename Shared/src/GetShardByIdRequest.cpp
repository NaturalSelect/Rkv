#include <rkv/GetShardByIdRequest.hpp>

std::size_t rkv::GetShardByIdRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->id_};
    size += Helper::ComputeSize(builder);
    return size;
}

std::size_t rkv::GetShardByIdRequest::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->id_ = builder.Get();
    return offset;
}

std::size_t rkv::GetShardByIdRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->id_};
    offset += Helper::UnsafeStoreTo(builder,data);
    return offset;
}