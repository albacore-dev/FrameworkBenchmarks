#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <skye/types.hpp>

#include <ctime>
#include <memory>

namespace skye_benchmark {

namespace asio = boost::asio;
namespace http = boost::beast::http;

struct Context {
    std::tm now{};
};

struct Mux {
    asio::awaitable<skye::response> operator()(skye::request req) const;

    skye::response json(const skye::request& req) const;
    skye::response db(const skye::request& req) const;
    skye::response queries(const skye::request& req) const;
    skye::response plaintext(const skye::request& req) const;

    skye::response
    make_response(http::status status, const skye::request& req) const;

    std::shared_ptr<Context> ctx;
};

asio::awaitable<void> async_poll_date(std::shared_ptr<Context> ctx);

} // namespace skye_benchmark
