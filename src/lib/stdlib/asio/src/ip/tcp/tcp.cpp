#include <asio_cube.hpp>
#include <functional>
#include <map>

using namespace std;
using namespace asio;
using ip::tcp;

extern "C"
{
    EXPORTED cube_native_var *tcp_resolve(char *hostname, char *service)
    {
        asio::error_code ec;
        tcp::resolver resolver(context);
        tcp::resolver::query query(hostname, service);
        tcp::resolver::iterator it = resolver.resolve(query, ec);
        if (ec)
            return NATIVE_NULL();

        cube_native_var *ret = NATIVE_LIST();

        asio::ip::tcp::resolver::iterator it_end;
        for (; it != it_end; ++it)
        {
            asio::ip::tcp::endpoint ep = it->endpoint();

            cube_native_var *item = NATIVE_DICT();
            ADD_NATIVE_DICT(item, COPY_STR("port"), NATIVE_NUMBER(ep.port()));
            ADD_NATIVE_DICT(item, COPY_STR("address"), NATIVE_STRING_COPY(ep.address().to_string().c_str()));
            ADD_NATIVE_DICT(item, COPY_STR("ipv6"), NATIVE_BOOL(ep.address().is_v6()));

            ADD_NATIVE_LIST(ret, item);
        }

        return ret;
    }
}