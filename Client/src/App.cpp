#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/IpEndPoint.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/MessageHeader.hpp>

static sharpen::NetStreamChannelPtr ConnectTo(const char *ip,std::uint16_t port)
{
    sharpen::IpEndPoint ep{0,0};
    sharpen::NetStreamChannelPtr channel = sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip);
    channel->Bind(ep);
    channel->Register(sharpen::EventEngine::GetEngine());
    ep.SetAddrByString(ip);
    ep.SetPort(port);
    channel->ConnectAsync(ep);
    return channel;
}

static void WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header)
{
    std::size_t sz{channel->WriteObjectAsync(header)};
    if(sz != sizeof(header))
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

static void WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header,const sharpen::ByteBuffer &request)
{
    WriteMessage(channel,header);
    std::size_t sz{channel->WriteAsync(request)};
    if(sz != request.GetSize())
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

static void ReadMessage(sharpen::NetStreamChannelPtr channel,rkv::MessageType expectedType,sharpen::ByteBuffer &response)
{
    rkv::MessageHeader header;
    std::size_t sz{channel->ReadObjectAsync(header)};
    if(sz != sizeof(header))
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
    if(rkv::GetMessageType(header) != expectedType)
    {
        throw std::logic_error("got unexpected response");
    }
    response.ExtendTo(sharpen::IntCast<std::size_t>(header.size_));
    sz = channel->ReadFixedAsync(response);
    if(sz != response.GetSize())
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

static sharpen::Optional<sharpen::IpEndPoint> RedirectLeader(sharpen::NetStreamChannelPtr channel)
{
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectRequest,0)};
    WriteMessage(channel,header);
    sharpen::ByteBuffer buf;
    ReadMessage(channel,rkv::MessageType::LeaderRedirectResponse,buf);
    rkv::LeaderRedirectResponse response;
    response.Unserialize().LoadFrom(buf);
    if(response.KnowLeader())
    {
        char ip[21] = {};
        response.Endpoint().GetAddrString(ip,sizeof(ip));
        std::printf("[Info]Got leader id %s:%hu\n",ip,response.Endpoint().GetPort());
        return response.Endpoint();
    }
    std::puts("[Info]Cannot get leader id");
    return sharpen::EmptyOpt;
}

static sharpen::ByteBuffer GetValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
{
    rkv::GetRequest request;
    request.Key() = std::move(key);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetRequest,buf.GetSize())};
    WriteMessage(channel,header,buf);
    ReadMessage(channel,rkv::MessageType::GetResponse,buf);
    rkv::GetResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Value();
}

static bool PutKeyValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,sharpen::ByteBuffer value)
{
    rkv::PutRequest request;
    request.Key() = std::move(key);
    request.Value() = std::move(value);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::PutRequest,buf.GetSize())};
    WriteMessage(channel,header,buf);
    ReadMessage(channel,rkv::MessageType::PutResponse,buf);
    rkv::PutResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Success();
}

static bool DeleteKey(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
{
    rkv::DeleteRequest request;
    request.Key() = std::move(key);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeleteReqeust,buf.GetSize())};
    WriteMessage(channel,header,buf);
    ReadMessage(channel,rkv::MessageType::DeleteResponse,buf);
    rkv::DeleteResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Success();
}

static void Entry(const char *ip,std::uint16_t port)
{
    sharpen::StartupNetSupport();
    std::printf("[Info]Connecting to %s:%hu\n",ip,port);
    sharpen::NetStreamChannelPtr channel;
    try
    {
        channel = ConnectTo(ip,port);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Cannot connect to %s:%hu because %s\n",ip,port,e.what());
        sharpen::CleanupNetSupport();
        return;
    }
    std::puts("[Info]Try to redirect to leader");
    auto id{RedirectLeader(channel)};
    if(!id.Exist())
    {
        std::fputs("[Error]Cannot redirect to leader",stderr);
        sharpen::CleanupNetSupport();
        return;
    }
    char tmp[21] = {};
    id.Get().GetAddrString(tmp,sizeof(tmp));
    if(id.Get().GetPort() != port || std::strcmp(ip,tmp))
    {
        try
        {
            channel = ConnectTo(ip,port);
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot connect to %s:%hu because %s\n",ip,port,e.what());
            sharpen::CleanupNetSupport();
            return;
        }
    }
    std::puts("[Info]Command list");
    std::puts("\tget <key> - get a value\n"
                "\tput <key> <value> - put a key value pair\n"
                "\tdelete <key> - delete a key value pair");
    std::puts("[Info]Enter interactive model");
    
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    if(argc < 3)
    {
        std::puts("usage: <ip> <port>");
        return 0;
    }
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    std::uint16_t port{sharpen::Atoi<std::uint16_t>(argv[2],std::strlen(argv[2]))};
    engine.Startup(&Entry,argv[1],port);
    return 0;
}