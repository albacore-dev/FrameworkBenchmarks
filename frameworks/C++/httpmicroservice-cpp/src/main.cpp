#include "handler.hpp"

#include <httpmicroservice/format.hpp>
#include <httpmicroservice/service.hpp>

#include <boost/asio/signal_set.hpp>
#include <fmt/core.h>

#include <cstdio>
#include <exception>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;

void reporter(const usrv::session_stats& stats)
{
    fmt::print("{}\n", stats);
}

int main()
{
    try {
        asio::io_context ioc;

        usrv::async_run(ioc, 8080, httpmicroservice_benchmark::Handler{}, reporter);

        asio::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}
