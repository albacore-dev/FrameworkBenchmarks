#include "handler.hpp"

#include <fmt/chrono.h>

#include <ctime>

namespace httpmicroservice_benchmark {

namespace http = boost::beast::http;

constexpr auto kServerName = "usrv";

usrv::response make_response(http::status status, const usrv::request& req)
{
    const std::tm now = fmt::gmtime(std::time(nullptr));

    usrv::response res{status, req.version()};
    res.set(http::field::server, kServerName);
    res.set(http::field::date, fmt::format("{:%a, %d %b %Y %T} GMT", now));

    return res;
}

usrv::response get(const usrv::request& req)
{
    auto res = make_response(http::status::ok, req);
    if (req.target() == "/json") {
        res.set(http::field::content_type, "application/json");
        res.body() = "{\"message\":\"Hello, World!\"}";
    } else if (req.target() == "/plaintext") {
        res.set(http::field::content_type, "text/plain");
        res.body() = "Hello, World!";
    } else {
        res.result(http::status::not_found);
    }

    return res;
}

asio::awaitable<usrv::response> Handler::operator()(usrv::request req) const
{
    switch (req.method()) {
    case http::verb::get:
        co_return get(req);
    default:
        co_return make_response(http::status::method_not_allowed, req);
    }
}

}
