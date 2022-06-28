// Copyright (c) Vector Informatik GmbH. All rights reserved.
#pragma once

#include <string>
#include <sstream>

#include "asio/local/stream_protocol.hpp"
#include "asio/ip/tcp.hpp"

// URI encoding of asio endpoint types.
// NB: Very limited implementation for internal use only -- nothing close to standard RFC 3986

namespace ib {
namespace mw {

class Uri
{
public:
    // public Enums
    enum class UriType
    {
        Undefined,
        Tcp,
        Local
    };

public:
    // public CTor
    //!< Decomposes uriRepr into Host+Port or Path
    explicit Uri(const asio::local::stream_protocol::endpoint& ep);
    explicit Uri(const asio::ip::tcp::endpoint& ep);
    explicit Uri(const std::string& host, uint16_t port);
    explicit Uri(const std::string& uriStr);
    Uri(const Uri&) = default;
    Uri& operator=(const Uri&) = default;
public:
    // public static methods
    static auto Parse(std::string uriStr) -> Uri;
public:
    // public methods
    auto EncodedString() const -> const std::string&;
    auto Scheme() const -> const std::string&;
    auto Host() const -> const std::string&;
    auto Port() const -> uint16_t;
    //!< Path currently returns everything after the '/', including queries and fragments
    auto Path() const -> const std::string&;
    auto Type() const -> UriType;
    void SetType(UriType newType);

private:
    // private ctor
    Uri() = default;
private:
    // private members
    UriType _type{ UriType::Undefined };
    std::string _scheme;
    std::string _host;
    uint16_t _port{0};
    std::string _path;
    std::string _uriString;
};

} // namespace mw
} // namespace ib
