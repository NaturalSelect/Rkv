#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <rkv/RaftStorage.hpp>
#include <rkv/KeyValueService.hpp>

static void Entry()
{
    sharpen::StartupNetSupport();
    std::puts("start server");
    //create raft storage directory
    sharpen::MakeDirectory("./rs");
    //create kv storage directory
    sharpen::MakeDirectory("./kv");
    //cretae config directory
    sharpen::MakeDirectory("./config");

    std::puts("stop server");
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    engine.Startup(&Entry);
    return 0;
}