#include <Server/ServerType.h>

#include <vector>
#include <algorithm>

#include <magic_enum.hpp>


namespace DB
{

namespace
{
    std::vector<std::string> getTypeIndexToTypeName()
    {
        constexpr std::size_t types_size = magic_enum::enum_count<ServerType::Type>();

        std::vector<std::string> type_index_to_type_name;
        type_index_to_type_name.resize(types_size);

        auto entries = magic_enum::enum_entries<ServerType::Type>();
        for (const auto & [entry, str] : entries)
        {
            auto str_copy = String(str);
            std::replace(str_copy.begin(), str_copy.end(), '_', ' ');
            type_index_to_type_name[static_cast<UInt64>(entry)] = std::move(str_copy);
        }

        return type_index_to_type_name;
    }
}

const char * ServerType::serverTypeToString(ServerType::Type type)
{
    /** During parsing if SystemQuery is not parsed properly it is added to Expected variants as description check IParser.h.
      * Description string must be statically allocated.
      */
    static std::vector<std::string> type_index_to_type_name = getTypeIndexToTypeName();
    const auto & type_name = type_index_to_type_name[static_cast<UInt64>(type)];
    return type_name.data();
}

bool ServerType::shouldStart(Type server_type, const std::string & server_custom_name) const
{
    if (type == Type::QUERIES_ALL)
        return true;

    if (type == Type::QUERIES_DEFAULT)
    {
        switch (server_type)
        {
            case Type::TCP:
            case Type::TCP_WITH_PROXY:
            case Type::TCP_SECURE:
            case Type::HTTP:
            case Type::HTTPS:
            case Type::MYSQL:
            case Type::GRPC:
            case Type::POSTGRESQL:
            case Type::PROMETHEUS:
            case Type::INTERSERVER_HTTP:
            case Type::INTERSERVER_HTTPS:
                return true;
            default:
                return false;
        }
    }

    if (type == Type::QUERIES_CUSTOM)
    {
        switch (server_type)
        {
            case Type::CUSTOM:
                return true;
            default:
                return false;
        }
    }

    if (type == Type::CUSTOM)
        return server_type == type && server_custom_name == "protocols." + custom_name + ".port";

    return server_type == type;
}

bool ServerType::shouldStop(const std::string & port_name) const
{
    Type port_type;

    if (port_name == "http_port")
        port_type = Type::HTTP;

    else if (port_name == "https_port")
        port_type = Type::HTTPS;

    else if (port_name == "tcp_port")
        port_type = Type::TCP;

    else if (port_name == "tcp_with_proxy_port")
        port_type = Type::TCP_WITH_PROXY;

    else if (port_name == "tcp_port_secure")
        port_type = Type::TCP_SECURE;

    else if (port_name == "mysql_port")
        port_type = Type::MYSQL;

    else if (port_name == "postgresql_port")
        port_type = Type::POSTGRESQL;

    else if (port_name == "grpc_port")
        port_type = Type::GRPC;

    else if (port_name == "prometheus.port")
        port_type = Type::PROMETHEUS;

    else if (port_name == "interserver_http_port")
        port_type = Type::INTERSERVER_HTTP;

    else if (port_name == "interserver_https_port")
        port_type = Type::INTERSERVER_HTTPS;

    else if (port_name.starts_with("protocols.") && port_name.ends_with(".port"))
        port_type = Type::CUSTOM;

    else
        return false;

    return shouldStart(port_type, port_name);
}

}
