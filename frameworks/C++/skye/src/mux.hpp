#include <boost/asio/awaitable.hpp>
#include <skye/types.hpp>

#include <memory>

namespace skye_benchmark {

namespace asio = boost::asio;
namespace http = boost::beast::http;

struct Mux {
    asio::awaitable<skye::response> operator()(skye::request req) const;

    skye::response plaintext(const skye::request& req) const;
    skye::response json(const skye::request& req) const;
    skye::response db(const skye::request& req) const;

    skye::response
    make_response(http::status status, const skye::request& req) const;
};

} // namespace skye_benchmark
