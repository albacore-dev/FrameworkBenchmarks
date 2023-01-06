#include "handler.hpp"

namespace httpmicroservice_benchmark {

namespace http = boost::beast::http;

usrv::response make_response(http::status status, const usrv::request& req)
{
    usrv::response res{status, req.version()};
    res.set(http::field::server, "usrv");
    res.set(http::field::date, "Wed, 21 Oct 2015 07:28:00 GMT");

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
