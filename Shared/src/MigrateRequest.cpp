#include <rkv/MigrateRequest.hpp>

std::size_t rkv::MigrateRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += this->beginKey_.ComputeSize();
    size += this->endKey_.ComputeSize();
    return size;
}

std::size_t rkv::MigrateRequest::LoadFrom(const char *data,std::size_t size)
{
    std::size_t offset{0};
    offset += this->beginKey_.LoadFrom(data,size);
    if(offset == size)
    {
        throw sharpen::DataCorruptionException("migrate request corruption");
    }
    offset += this->endKey_.LoadFrom(data + offset,size - offset);
    return offset;
}

std::size_t rkv::MigrateRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += this->beginKey_.UnsafeStoreTo(data);
    offset += this->endKey_.UnsafeStoreTo(data + offset);
    return offset;
}