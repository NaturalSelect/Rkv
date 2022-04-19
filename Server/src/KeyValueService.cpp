#include <rkv/KeyValueService.hpp>

void rkv::KeyValueService::Apply(rkv::RaftLog log)
{
    std::puts("[Info]Apply log");
    switch (log.GetOperation())
    {
    case rkv::RaftLog::Operation::Put:
        {   
            if(!log.Value().Empty())
            {
                this->Put(std::move(log.Key()),std::move(log.Value()));
            }
        }
        break;
    case rkv::RaftLog::Operation::Delete:
        {
            this->Delete(log.Key());
        }
        break;
    }
}