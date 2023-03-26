#include "mux.hpp"
#include "model.hpp"

#include <fmt/chrono.h>

namespace skye_benchmark {

constexpr auto kServerName = "skye";
constexpr auto kContentTypeJson = "application/json";
constexpr auto kContentTypeText = "text/plain";
constexpr auto kHelloMessage = "Hello, World!";

asio::awaitable<skye::response> Mux::operator()(skye::request req) const
{
    constexpr std::string_view kJsonPath = "/json";
    constexpr std::string_view kDbPath = "/db";
    constexpr std::string_view kPlaintextPath = "/plaintext";

    if (req.method() != http::verb::get) {
        co_return make_response(http::status::method_not_allowed, req);
    }

    if (req.target() == kJsonPath) {
        co_return json(req);
    } else if (req.target() == kDbPath) {
        co_return db(req);
    } else if (req.target() == kPlaintextPath) {
        co_return plaintext(req);
    }

    co_return make_response(http::status::not_found, req);
}

skye::response Mux::plaintext(const skye::request& req) const
{
    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeText);
    res.body() = kHelloMessage;

    return res;
}

skye::response Mux::json(const skye::request& req) const
{
    const Object model{kHelloMessage};

    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeJson);
    res.body() = fmt::format("{}", model);

    return res;
}

skye::response Mux::db(const skye::request& req) const
{
    const World model{1, 10000};

    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeJson);
    res.body() = fmt::format("{}", model);

    return res;
}

skye::response
Mux::make_response(http::status status, const skye::request& req) const
{
    // const std::tm now = fmt::gmtime(std::time(nullptr));

    skye::response res{status, req.version()};
    res.set(http::field::server, kServerName);
    res.set(http::field::date, fmt::format("{:%a, %d %b %Y %T} GMT", ctx->now));

    return res;
}

} // namespace skye_benchmark
