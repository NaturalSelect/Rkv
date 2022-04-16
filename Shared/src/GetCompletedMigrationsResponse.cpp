#include <rkv/GetCompletedMigrationsResponse.hpp>

std::size_t rkv::GetCompletedMigrationsResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->migrations_);
}

std::size_t rkv::GetCompletedMigrationsResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->migrations_,data,size);
}

std::size_t rkv::GetCompletedMigrationsResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->migrations_,data);
}