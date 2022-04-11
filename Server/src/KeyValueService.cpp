#include <rkv/KeyValueService.hpp>

void rkv::KeyValueService::Apply(rkv::RaftLog log)
{
    switch (log.GetOperation())
    {
    case rkv::RaftLog::Operation::Put:
        {   
            if(!log.Value().Empty())
            {
                std::fputs("[Info]Apply put log key is ",stdout);
                for (std::size_t i = 0; i != log.Key().GetSize(); ++i)
                {
                    std::putchar(log.Key()[i]);
                }
                std::fputs(" value is ",stdout);
                for (std::size_t i = 0; i != log.Value().GetSize(); ++i)
                {
                    std::putchar(log.Value()[i]);
                }
                std::putchar('\n');
                this->Put(std::move(log.Key()),std::move(log.Value()));
            }
        }
        break;
    case rkv::RaftLog::Operation::Delete:
        {
            std::fputs("[Info]Apply delete log key is ",stdout);
            for (std::size_t i = 0; i != log.Key().GetSize(); ++i)
            {
                std::putchar(log.Key()[i]);
            }
            std::putchar('\n');
            this->Delete(log.Key());
        }
        break;
    }
}