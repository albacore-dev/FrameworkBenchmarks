#include "handler.hpp"

#include <httpmicroservice/service.hpp>

#include <boost/asio/signal_set.hpp>
//#include <fmt/core.h>

// #include <cstdio>
#include <exception>

namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace usrv = httpmicroservice;

asio::awaitable<usrv::response> hello(usrv::request req)
{
    usrv::response res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    co_return res;
}

int main()
{
    try {
        asio::io_context ioc;

        usrv::async_run(ioc, 8080, httpmicroservice_benchmark::Handler{});

        asio::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        // fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        // fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}

