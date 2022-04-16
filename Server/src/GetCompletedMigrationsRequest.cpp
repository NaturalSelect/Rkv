#include <rkv/GetCompletedMigrationsRequest.hpp>

std::size_t rkv::GetCompletedMigrationsRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->source_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->beginId_);
    size += Helper::ComputeSize(builder);
    return size;
}

std::size_t rkv::GetCompletedMigrationsRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 2)
    {
        throw std::invalid_argument("invalid buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->source_ = builder.Get();
    if(size == offset)
    {
        throw sharpen::DataCorruptionException("get completed migrations request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->beginId_ = builder.Get();
    return offset;
}

std::size_t rkv::GetCompletedMigrationsRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->source_};
    offset += Helper::UnsafeStoreTo(builder,data);
    builder.Set(this->beginId_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    return offset;
}