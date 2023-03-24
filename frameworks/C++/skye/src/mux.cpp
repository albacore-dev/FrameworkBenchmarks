#include "mux.hpp"

#include <fmt/chrono.h>

namespace skye_benchmark {

constexpr auto kServerName = "skye";

asio::awaitable<skye::response> Mux::operator()(skye::request req) const
{
    constexpr std::string_view kPlaintext{"/plaintext"};
    constexpr std::string_view kJson{"/json"};
    constexpr std::string_view kDb{"/db"};

    if (req.method() != http::verb::get) {
        co_return make_response(http::status::method_not_allowed, req);
    }

    if (req.target() == kPlaintext) {
        co_return plaintext(req);
    } else if (req.target() == kJson) {
        co_return json(req);
    } else if (req.target() == kDb) {
        co_return db(req);
    }

    co_return make_response(http::status::not_found, req);
}

skye::response Mux::plaintext(const skye::request& req) const
{
    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello, World!";

    return res;
}

skye::response Mux::json(const skye::request& req) const
{
    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"message\":\"Hello, World!\"}";

    return res;
}

skye::response Mux::db(const skye::request& req) const
{
    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"id\":1,\"randomNumber\":10000}";

    return res;
}

skye::response
Mux::make_response(http::status status, const skye::request& req) const
{
    const std::tm now = fmt::gmtime(std::time(nullptr));

    skye::response res{status, req.version()};
    res.set(http::field::server, kServerName);
    res.set(http::field::date, fmt::format("{:%a, %d %b %Y %T} GMT", now));

    return res;
}

} // namespace skye_benchmark
