#include <rkv/StartMigrationResponse.hpp>

std::size_t rkv::StartMigrationResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->result_);
}

std::size_t rkv::StartMigrationResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->result_,data,size);
}

std::size_t rkv::StartMigrationResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->result_,data);
}