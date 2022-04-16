#include <rkv/CompleteMigrationRequest.hpp>

std::size_t rkv::CompleteMigrationRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->groupId_};
    size += Helper::ComputeSize(builder);
    size += Helper::ComputeSize(this->id_);
    return size;
}

std::size_t rkv::CompleteMigrationRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 1 + Helper::ComputeSize(this->id_))
    {
        throw std::invalid_argument("invalid buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->groupId_ = builder.Get();
    if(size == offset)
    {
        throw sharpen::DataCorruptionException("completed migrations request corruption");
    }
    offset += Helper::LoadFrom(this->id_,data + offset,size - offset);
    return offset;
}

std::size_t rkv::CompleteMigrationRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->groupId_};
    offset += Helper::UnsafeStoreTo(builder,data);
    offset += Helper::UnsafeStoreTo(this->id_,data + offset);
    return offset;
}