#include <rkv/GetMigrationsResponse.hpp>

std::size_t rkv::GetMigrationsResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->migrations_);
}

std::size_t rkv::GetMigrationsResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->migrations_,data,size);
}

std::size_t rkv::GetMigrationsResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->migrations_,data);
}