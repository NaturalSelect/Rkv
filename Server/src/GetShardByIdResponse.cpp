#include <rkv/GetShardByIdResponse.hpp>

std::size_t rkv::GetShardByIdResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->shards_);
}

std::size_t rkv::GetShardByIdResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->shards_,data,size);
}