#include <httpmicroservice/types.hpp>

#include <boost/asio/awaitable.hpp>

namespace httpmicroservice_benchmark {

namespace asio = boost::asio;
namespace usrv = httpmicroservice;

struct Handler {
    asio::awaitable<usrv::response> operator()(usrv::request req) const;
};

}

