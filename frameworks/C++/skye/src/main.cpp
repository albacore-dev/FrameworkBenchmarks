#include "context.hpp"
#include "mux.hpp"

#include <boost/asio/thread_pool.hpp>
#include <fmt/core.h>
#include <skye/service.hpp>

#include <cstdio>
#include <exception>
#include <memory>

int main()
{
    namespace asio = boost::asio;

    try {
        const auto context = std::make_shared<skye_benchmark::Context>(
            skye_benchmark::SQLiteContext{"database.db"});

        // Run mux router, handlers, and anything that access the context.
        asio::thread_pool pool{1};

        // Updates context::now every second. Runs in the pool.
        co_spawn(pool, skye_benchmark::async_poll_date(context), [](auto ptr) {
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });

        // Run accept loop and HTTP connection I/O.
        asio::io_context ioc{1};

        skye::async_run(
            ioc, 8080,
            skye::make_co_handler(pool, skye_benchmark::Mux{context}));

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
