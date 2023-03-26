#include "mux.hpp"
#include "model.hpp"

#include <boost/asio/system_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/url/src.hpp>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <charconv>

namespace skye_benchmark {

constexpr auto kServerName = "skye";
constexpr auto kContentTypeJson = "application/json";
constexpr auto kContentTypeText = "text/plain";
constexpr auto kHelloMessage = "Hello, World!";
constexpr int kMinParam = 1;
constexpr int kMaxParam = 500;

// Parse an integer query parameter from the request. Returns kMinParam if no
// matching parameter is found.
int parse_param(const skye::request& req, std::string_view name);

asio::awaitable<skye::response> Mux::operator()(skye::request req) const
{
    constexpr std::string_view kJsonPath = "/json";
    constexpr std::string_view kDbPath = "/db";
    constexpr std::string_view kQueriesPath = "/queries";
    constexpr std::string_view kPlaintextPath = "/plaintext";

    if (req.method() != http::verb::get) {
        co_return make_response(http::status::method_not_allowed, req);
    }

    if (req.target() == kJsonPath) {
        co_return json(req);
    } else if (req.target() == kDbPath) {
        co_return db(req);
    } else if (req.target().starts_with(kQueriesPath)) {
        co_return queries(req);
    } else if (req.target() == kPlaintextPath) {
        co_return plaintext(req);
    }

    co_return make_response(http::status::not_found, req);
}

skye::response Mux::json(const skye::request& req) const
{
    const Model model{kHelloMessage};

    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeJson);
    res.body() = fmt::format("{}", model);

    return res;
}

skye::response Mux::db(const skye::request& req) const
{
    const auto model = ctx->db.getRandomModel();
    if (!model) {
        return make_response(http::status::not_found, req);
    }

    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeJson);
    res.body() = fmt::format("{}", *model);

    return res;
}

skye::response Mux::queries(const skye::request& req) const
{
    const int queries = parse_param(req, "queries");
    if ((queries < kMinParam) || (queries > kMaxParam)) {
        return make_response(http::status::internal_server_error, req);
    }

    std::vector<World> models;
    models.reserve(queries);

    for (int i = 0; i < queries; ++i) {
        const auto model = ctx->db.getRandomModel();
        if (!model) {
            return make_response(http::status::not_found, req);
        }

        models.push_back(*model);
    }

    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeJson);
    res.body() = fmt::format("{}", models);

    return res;
}

skye::response Mux::plaintext(const skye::request& req) const
{
    auto res = make_response(http::status::ok, req);
    res.set(http::field::content_type, kContentTypeText);
    res.body() = kHelloMessage;

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

int parse_param(const skye::request& req, std::string_view name)
{
    const auto result = boost::urls::parse_origin_form(req.target());
    if (!result.has_value()) {
        return kMinParam;
    }

    // Linear scan through the query parameters. Return the first one that:
    // - Matches the name
    // - Contains an integer, even if its out of the [1, 500] range
    const auto url_view = result.value();
    for (const auto& qp : url_view.params()) {
        if ((qp.key == name) && qp.has_value) {
            int value = kMinParam;
            if (std::from_chars(
                    qp.value.data(), qp.value.data() + qp.value.size(), value)
                    .ec == std::errc{}) {
                return std::max(std::min(value, kMaxParam), kMinParam);
            }
        }
    }

    return kMinParam;
}

} // namespace skye_benchmark
