#include <rkv/GetCompletedMigrationsRequest.hpp>

std::size_t rkv::GetCompletedMigrationsRequest::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->source_);
}

std::size_t rkv::GetCompletedMigrationsRequest::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->source_,data,size);
}

std::size_t rkv::GetCompletedMigrationsRequest::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->source_,data);
}