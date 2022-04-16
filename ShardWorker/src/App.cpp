#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <sharpen/CtrlHandler.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/Utility.hpp>
#include <rkv/MasterClient.hpp>

// static void StopServer(rkv::KvServer *server)
// {
//     assert(server != nullptr);
//     server->Stop();
// }

static void Entry()
{
    sharpen::StartupNetSupport();
    // std::puts("start server");
    // //cretae config directory
    // std::puts("[Info]Reading configurations");
    // sharpen::MakeDirectory("./Config");
    // //read config
    // sharpen::IpEndPoint id;
    // std::vector<sharpen::IpEndPoint> members;
    // std::vector<std::string> lines;
    // rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),"./Config/Id.txt");
    // if(lines.empty())
    // {
    //     std::fputs("[Error]Please edit ./Config/Id.txt to set server id(ip port)\n",stderr);
    //     return;
    // }
    // std::string first{std::move(lines.front())};
    // try
    // {
    //     sharpen::IpEndPoint tmp{rkv::ConvertStringToEndPoint(first)};
    //     id = std::move(tmp);
    // }
    // catch(const std::exception& e)
    // {
    //     std::fprintf(stderr,"[Error]Connot parse id configuration %s because %s\n",first.c_str(),e.what());
    //     return;   
    // }
    // lines.clear();
    // rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),"./Config/Members.txt");
    // std::puts("[Info]Member configurations are");
    // for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    // {
    //     std::printf("\t%s\n",begin->c_str());
    // }
    // for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    // {
    //     try
    //     {
    //         sharpen::IpEndPoint ep{rkv::ConvertStringToEndPoint(*begin)};
    //         members.emplace_back(std::move(ep));
    //     }
    //     catch(const std::exception& e)
    //     {
    //         std::fprintf(stderr,"[Error]Cannot parse member configuration line %s because %s\n",begin->data(),e.what());
    //         return;
    //     }
    // }
    // rkv::KvServerOption opt{id,members.begin(),members.end()};
    // //start server
    // std::puts("[Info]Start server");
    // std::puts("[Info]Please use ctrl+c to stop server");
    // rkv::KvServer server{sharpen::EventEngine::GetEngine(),opt};
    // sharpen::RegisterCtrlHandler(sharpen::CtrlType::Interrupt,std::bind(&StopServer,&server));
    // server.RunAsync();
    // std::puts("[Info]Server Stopped");
    std::vector<sharpen::IpEndPoint> members;
    {
        sharpen::IpEndPoint ep;
        ep.SetAddrByString("127.0.0.1");
        for (size_t i = 0; i != 3; ++i)
        {
            ep.SetPort(8080 + i);   
            members.emplace_back(ep);
        }
    }
    sharpen::ByteBuffer key{};
    rkv::MasterClient client{sharpen::EventEngine::GetEngine(),members.begin(),members.end(),std::chrono::seconds{1},10};
    sharpen::IpEndPoint id;
    id.SetAddrByString("127.0.0.1");
    id.SetPort(8083);
    std::vector<rkv::Shard> shards;
    sharpen::ByteBuffer beginKey{"123",3};
    sharpen::ByteBuffer endKey{"456",3};
    auto r = client.DeriveShard(0,beginKey,endKey);
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupEngine();
    engine.Startup(&Entry);
    return 0;
}