#include <rkv/GetShardByWorkerIdResponse.hpp>

std::size_t rkv::GetShardByWorkerIdResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->shards_);
}

std::size_t rkv::GetShardByWorkerIdResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->shards_,data,size);
}

std::size_t rkv::GetShardByWorkerIdResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->shards_,data);
}