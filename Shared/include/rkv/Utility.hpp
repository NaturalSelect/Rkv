#pragma once
#ifndef _RKV_UTILITY_HPP
#define _RKV_UTILITY_HPP

#include <string>
#include <iterator>

#include <sharpen/IFileChannel.hpp>
#include <sharpen/EventEngine.hpp>
#include <sharpen/IpEndPoint.hpp>

namespace rkv
{
    template <typename _InserterIterator, typename _Check = decltype(*std::declval<_InserterIterator &>()++ = std::declval<std::string &>())>
    inline void ReadAllLines(sharpen::EventEngine &engine,_InserterIterator inserter, const char *filename)
    {
        sharpen::FileChannelPtr config = sharpen::MakeFileChannel(filename, sharpen::FileAccessModel::Read, sharpen::FileOpenModel::CreateOrOpen);
        config->Register(engine);
        std::uint64_t size{config->GetFileSize()};
        if (!size)
        {
            std::printf("[Info]Please edit members config file %s\n", filename);
            return;
        }
        std::uint64_t offset{0};
        sharpen::ByteBuffer buf{4096};
        std::string line;
        while (offset != size)
        {
            std::size_t sz{config->ReadAsync(buf, offset)};
            offset += sz;
            for (std::size_t i = 0; i != sz; ++i)
            {
                if (buf[i] == '\r' || buf[i] == '\n')
                {
                    if (!line.empty())
                    {
                        std::string tmp;
                        std::swap(tmp, line);
                        *inserter++ = std::move(tmp);
                    }
                    continue;
                }
                line.push_back(buf[i]);
            }
        }
        if (!line.empty())
        {
            std::string tmp;
            std::swap(tmp, line);
            *inserter++ = std::move(tmp);
        }
    }

    inline sharpen::IpEndPoint ConvertStringToEndPoint(const std::string &str)
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
}

#endif