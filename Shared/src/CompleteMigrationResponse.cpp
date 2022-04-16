#include <rkv/CompleteMigrationResponse.hpp>

std::size_t rkv::CompleteMigrationResponse::ComputeSize() const noexcept
{
    return Helper::ComputeSize(this->result_);
}

std::size_t rkv::CompleteMigrationResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->result_,data,size);
}

std::size_t rkv::CompleteMigrationResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->result_,data);
}