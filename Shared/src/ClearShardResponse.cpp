#include <rkv/ClearShardResponse.hpp>

std::size_t rkv::ClearShardResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->result_);
}

std::size_t rkv::ClearShardResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->result_,data,size);
}

std::size_t rkv::ClearShardResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->result_,data);
}