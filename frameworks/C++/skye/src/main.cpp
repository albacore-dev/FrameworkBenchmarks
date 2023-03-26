#include "mux.hpp"

#include <fmt/core.h>
#include <skye/service.hpp>

#include <cstdio>
#include <exception>
#include <memory>

int main()
{
    namespace asio = boost::asio;

    try {
        const skye_benchmark::Mux mux{
            std::make_shared<skye_benchmark::Context>()};

        asio::io_context ioc{1};

        co_spawn(ioc, skye_benchmark::async_poll_date(mux.ctx), [](auto ptr) {
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });

        skye::async_run(ioc, 8080, mux);

        asio::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        ioc.run();

        return EXIT_SUCCESS;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return EXIT_FAILURE;
}
