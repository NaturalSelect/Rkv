#include <rkv/GetShardByKeyRequest.hpp>

std::size_t rkv::GetShardByKeyRequest::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->key_);
}

std::size_t rkv::GetShardByKeyRequest::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->key_,data,size);
}

std::size_t rkv::GetShardByKeyRequest::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->key_,data);
}