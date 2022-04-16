#include <rkv/GetMigrationsRequest.hpp>

std::size_t rkv::GetMigrationsRequest::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->destination_);
}

std::size_t rkv::GetMigrationsRequest::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->destination_,data,size);
}

std::size_t rkv::GetMigrationsRequest::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->destination_,data);
}