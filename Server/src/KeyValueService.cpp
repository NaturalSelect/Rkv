#include <rkv/KeyValueService.hpp>

void rkv::KeyValueService::Apply(rkv::RaftLog log)
{
    switch (log.GetOperation())
    {
    case rkv::RaftLog::Operation::Put:
        {
            if(!log.Value().Empty())
            {
                this->table_.Put(std::move(log.Key()),std::move(log.Value()));
            }
        }
        break;
    case rkv::RaftLog::Operation::Delete:
        {
            this->table_.Delete(log.Key());
        }
        break;
    }
}