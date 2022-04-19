#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <sharpen/CtrlHandler.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/MasterServer.hpp>
#include <rkv/Utility.hpp>

static void StopServer(rkv::MasterServer *server)
{
    assert(server != nullptr);
    server->Stop();
}

static void Entry()
{
    const char *bindName = "./Config/Bind.txt";
    const char *idName = "./Config/MasterId.txt";
    const char *membersName = "./Config/MasterMembers.txt";
    const char *workerName = "./Config/Workers.txt";
    std::puts("[Info]Starting master server");
    //cretae config directory
    std::puts("[Info]Reading configurations");
    sharpen::MakeDirectory("./Config");
    //read config
    sharpen::IpEndPoint bind;
    sharpen::IpEndPoint id;
    std::vector<sharpen::IpEndPoint> members;
    std::vector<std::string> lines;
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),bindName);
    if (lines.empty())
    {
        std::fprintf(stderr,"[Error]Please edit %s to set server bind endpoint(ip port)\n",bindName);
        return;
    }
    std::string first{std::move(lines.front())};
    try
    {
        sharpen::IpEndPoint tmp{rkv::ConvertStringToEndPoint(first)};
        bind = std::move(tmp);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Connot parse bind configuration %s because %s\n",first.c_str(),e.what());
        return;
    }
    lines.clear();
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),idName);
    if(lines.empty())
    {
        std::fprintf(stderr,"[Error]Please edit %s to set server id(ip port)\n",idName);
        return;
    }
    first = std::move(lines.front());
    try
    {
        sharpen::IpEndPoint tmp{rkv::ConvertStringToEndPoint(first)};
        id = std::move(tmp);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Connot parse id configuration %s because %s\n",first.c_str(),e.what());
        return;   
    }
    lines.clear();
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),membersName);
    std::puts("[Info]Member configurations are");
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        std::printf("\t%s\n",begin->c_str());
    }
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        try
        {
            sharpen::IpEndPoint ep{rkv::ConvertStringToEndPoint(*begin)};
            members.emplace_back(std::move(ep));
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot parse member configuration line %s because %s\n",begin->data(),e.what());
            return;
        }
    }
    lines.clear();
    std::vector<sharpen::IpEndPoint> workers;
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),workerName);
    if(lines.empty())
    {
        std::fprintf(stderr,"[Error]Please edit %s to set workers id(ip port)\n",workerName);
        return;
    }
    std::puts("[Info]Worker configurations are");
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        std::printf("\t%s\n",begin->c_str());
    }
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        try
        {
            sharpen::IpEndPoint ep{rkv::ConvertStringToEndPoint(*begin)};
            workers.emplace_back(std::move(ep));
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot parse worker configuration line %s because %s\n",begin->data(),e.what());
            return;
        }
    }
    rkv::MasterServerOption opt{bind,id,members.begin(),members.end(),workers.begin(),workers.end()};
    //start server
    sharpen::StartupNetSupport();
    std::puts("[Info]Server started");
    std::puts("[Info]Please use ctrl+c to stop server");
    try
    {
        rkv::MasterServer server{sharpen::EventEngine::GetEngine(),opt};
        sharpen::RegisterCtrlHandler(sharpen::CtrlType::Interrupt,std::bind(&StopServer,&server));
        server.RunAsync();
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Server terminated because %s\n",e.what());
    }
    std::puts("[Info]Server Stopped");
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupEngine();
    engine.Startup(&Entry);
    return 0;
}