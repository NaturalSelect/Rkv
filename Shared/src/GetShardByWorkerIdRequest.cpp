#include <rkv/GetShardByWorkerIdRequest.hpp>

std::size_t rkv::GetShardByWorkerIdRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->id_);
    return size;
}

std::size_t rkv::GetShardByWorkerIdRequest::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->id_,data,size);
}

std::size_t rkv::GetShardByWorkerIdRequest::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->id_,data);
}