#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <sharpen/CtrlHandler.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/RaftServer.hpp>

template<typename _InserterIterator,typename _Check = decltype(*std::declval<_InserterIterator&>()++ = std::declval<std::string&>())>
inline void ReadAllLines(_InserterIterator inserter,const char *filename)
{
    sharpen::FileChannelPtr config = sharpen::MakeFileChannel(filename,sharpen::FileAccessModel::Read,sharpen::FileOpenModel::CreateOrOpen);
    config->Register(sharpen::EventEngine::GetEngine());
    std::uint64_t size{config->GetFileSize()};
    if(!size)
    {
        std::printf("[Info]Please edit members config file %s\n",filename);
        return;
    }
    std::uint64_t offset{0};
    sharpen::ByteBuffer buf{4096};
    std::string line;
    while (offset != size)
    {
        std::size_t sz{config->ReadAsync(buf,offset)};
        offset += sz;
        for (std::size_t i = 0; i != sz; ++i)
        {
            if(buf[i] == '\r' || buf[i] == '\n')
            {
                if(!line.empty())
                {
                    std::string tmp;
                    std::swap(tmp,line);
                    *inserter++ = std::move(tmp);
                }
                continue;
            }
            line.push_back(buf[i]);   
        }
    }
    if(!line.empty())
    {
        std::string tmp;
        std::swap(tmp,line);
        *inserter++ = std::move(tmp);
    }
}

static sharpen::IpEndPoint ConvertStringToEndPoint(const std::string &str)
{
    size_t pos{str.find(' ')};
    if(str.empty() || pos == str.npos || !pos || pos == str.size() - 1)
    {
        throw std::invalid_argument("not a format string(ip port)");
    }
    sharpen::IpEndPoint ep;
    std::string ip{str.substr(0,pos)};
    std::string portStr{str.substr(pos + 1,str.size() - pos - 1)};
    std::uint16_t port{sharpen::Atoi<std::uint16_t>(portStr.data(),portStr.size())};
    ep.SetAddrByString(ip.data());
    ep.SetPort(port);
    return ep;
}

static void StopServer(rkv::RaftServer *server)
{
    assert(server != nullptr);
    server->Stop();
}

static void Entry()
{
    sharpen::StartupNetSupport();
    std::puts("start server");
    //cretae config directory
    std::puts("[Info]Reading configurations");
    sharpen::MakeDirectory("./Config");
    //read config
    sharpen::IpEndPoint id;
    std::vector<sharpen::IpEndPoint> members;
    std::vector<std::string> lines;
    ReadAllLines(std::back_inserter(lines),"./Config/Id.txt");
    if(lines.empty())
    {
        std::fputs("[Error]Please edit ./Config/Id.txt to set server id(ip port)\n",stderr);
        return;
    }
    std::string first{std::move(lines.front())};
    try
    {
        sharpen::IpEndPoint tmp{ConvertStringToEndPoint(first)};
        id = std::move(tmp);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Connot parse id configuration %s because %s\n",first.c_str(),e.what());
        return;   
    }
    lines.clear();
    ReadAllLines(std::back_inserter(lines),"./Config/Members.txt");
    std::puts("[Info]Member configurations are");
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        std::printf("\t%s\n",begin->c_str());
    }
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        try
        {
            sharpen::IpEndPoint ep{ConvertStringToEndPoint(*begin)};
            members.emplace_back(std::move(ep));
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot parse member configuration line %s because %s\n",begin->data(),e.what());
            return;
        }
    }
    rkv::RaftServerOption opt{id,members.begin(),members.end()};
    //start server
    std::puts("[Info]Start server");
    std::puts("[Info]Please use ctrl+c to stop server");
    rkv::RaftServer server{sharpen::EventEngine::GetEngine(),opt};
    sharpen::RegisterCtrlHandler(sharpen::CtrlType::Interrupt,std::bind(&StopServer,&server));
    server.RunAsync();
    std::puts("stop server");
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    engine.Startup(&Entry);
    return 0;
}