#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/IpEndPoint.hpp>
#include <sharpen/Converter.hpp>
#include <sharpen/IInputPipeChannel.hpp>
#include <sharpen/FileOps.hpp>
#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/MessageHeader.hpp>
#include <rkv/KvClient.hpp>
#include <rkv/Utility.hpp>

static void Entry()
{
    const char *masterName = "./Config/Masters.txt";
    sharpen::MakeDirectory("./Config");
    std::vector<std::string> lines;
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),"./Config/Servers.txt");
    if(lines.empty())
    {
        std::fputs("[Error]Please edit ./Config/Servers.txt to set server id(ip port)\n",stderr);
        return;
    }
    std::vector<sharpen::IpEndPoint> ids;
    std::puts("[Info]Server configurations are");
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        std::printf("\t%s\n",begin->c_str());
    }
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        try
        {
            sharpen::IpEndPoint ep{rkv::ConvertStringToEndPoint(*begin)};
            ids.emplace_back(std::move(ep));
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot parse server configuration line %s because %s\n",begin->data(),e.what());
            return;
        }
    }
    sharpen::StartupNetSupport();
    rkv::KvClient client{sharpen::EventEngine::GetEngine(),ids.begin(),ids.end(),std::chrono::seconds(3),10,0};
    std::puts("[Info]Command list");
    std::puts("\tget <key> - get a value\n"
                "\tput <key> <value> - put a key value pair\n"
                "\tdelete <key> - delete a key value pair\n"
                "\tquit - exist client");
    std::puts("[Info]Enter interactive model");
    sharpen::InputPipeChannelPtr input = sharpen::MakeStdinPipe();
    input->Register(sharpen::EventEngine::GetEngine());
    while (1)
    {
        std::string line{input->GetsAsync()};
        if(line == "quit")
        {
            break;
        }
        char commandBuf[7] = {};
        int r{std::sscanf(line.data(),"%6s",commandBuf)};
        std::string command{commandBuf};
        if(r == -1)
        {
            std::fputs("[Error]Please re-enter command\n",stderr);
            continue;
        }
        if(command == "get" && line.size() > 4)
        {
            sharpen::ByteBuffer key{line.size() - 4};
            std::memcpy(key.Data(),line.data() + 4,line.size() - 4);
            sharpen::Optional<sharpen::ByteBuffer> buf;
            try
            {
                buf = client.Get(key,rkv::GetPolicy::Relaxed);
            }
            catch(const std::exception& e)
            {
                std::fprintf(stderr,"[Error]Cannot get key %s because %s\n",line.data() + 4,e.what());
                continue;
            }
            if(!buf.Exist())
            {
                std::printf("[Info]Key %s doesn't exist\n",line.data() + 4);
                continue;
            }
            std::fputs("[Info]Value is ",stdout);
            for (std::size_t i = 0; i != buf.Get().GetSize(); ++i)
            {
                std::putchar(buf.Get()[i]);
            }
            std::putchar('\n');
        }
        else if(command == "put" && line.size() > 5)
        {
            std::size_t first = line.find(' ');
            std::size_t last = line.find_last_of(' ');
            if(first == line.npos || last == line.npos || first == last || last == line.size() - 1)
            {
                std::fprintf(stderr,"[Error]Unknown command %s\n",line.data());
                continue;
            }
            sharpen::ByteBuffer key{last - first - 1};
            std::memcpy(key.Data(),line.data() + first + 1,last - first - 1);
            sharpen::ByteBuffer value{line.size() - last - 1};
            std::memcpy(value.Data(),line.data() + last + 1,line.size() - last - 1);
            rkv::MotifyResult result{client.Put(key,value)};
            switch (result)
            {
                case rkv::MotifyResult::NotCommit:
                {
                    std::fprintf(stderr,"[Error]Cannot put the key and value\n");
                    std::puts("[Info]Disconnect with leader");
                }
                break;
                case rkv::MotifyResult::Commited:
                {
                    std::puts("[Info]Operation commited");
                }
                break;
                case rkv::MotifyResult::Appiled:
                {
                    std::puts("[Info]Operation appiled");
                }
                break;
            }
        }
        else if (command == "delete" && line.size() > 6)
        {
            sharpen::ByteBuffer key{line.size() - 7};
            std::memcpy(key.Data(),line.data() + 7,line.size() - 7);
            rkv::MotifyResult result{client.Delete(std::move(key))};
            switch (result)
            {
                case rkv::MotifyResult::NotCommit:
                {
                    std::fprintf(stderr,"[Error]Cannot put the key and value\n");
                    std::puts("[Info]Disconnect with leader");
                }
                break;
                case rkv::MotifyResult::Commited:
                {        
                    std::puts("[Info]Operation commited");
                }
                break;
                case rkv::MotifyResult::Appiled:
                {
                    std::puts("[Info]Operation appiled");
                }
                break;
            }
        }
        else
        {
            std::fprintf(stderr,"[Error]Unknown command %s\n",line.data());
        }
    }
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    engine.Startup(&Entry);
    return 0;
}