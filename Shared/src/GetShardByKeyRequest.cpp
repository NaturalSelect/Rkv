#include <rkv/GetShardByKeyRequest.hpp>

std::size_t rkv::GetShardByKeyRequest::ComputeSize() const noexcept
{
    return this->key_.ComputeSize();
}

std::size_t rkv::GetShardByKeyRequest::LoadFrom(const char *data,std::size_t size)
{
    return this->key_.LoadFrom(data,size);
}

std::size_t rkv::GetShardByKeyRequest::UnsafeStoreTo(char *data) const noexcept
{
    return this->key_.UnsafeStoreTo(data);
}