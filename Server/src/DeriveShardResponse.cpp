#include <rkv/DeriveShardResponse.hpp>

std::size_t rkv::DeriveShardResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->result_);
}

std::size_t rkv::DeriveShardResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->result_,data,size);
}

std::size_t rkv::DeriveShardResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->result_,data);
}