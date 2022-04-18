#include <rkv/ClearShardRequest.hpp>

std::size_t rkv::ClearShardRequest::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->migration_);
}

std::size_t rkv::ClearShardRequest::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->migration_,data,size);
}

std::size_t rkv::ClearShardRequest::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->migration_,data);
}