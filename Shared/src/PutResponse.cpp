#include <rkv/PutResponse.hpp>

std::size_t rkv::PutResponse::LoadFrom(const char *data,std::size_t size)
{
    return Helper::LoadFrom(this->result_,data,size);
}

std::size_t rkv::PutResponse::UnsafeStoreTo(char *data) const noexcept
{
    return Helper::UnsafeStoreTo(this->result_,data);
}