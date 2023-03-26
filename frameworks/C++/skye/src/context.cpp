#include "context.hpp"

#include <boost/asio/system_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <fmt/chrono.h>

namespace skye_benchmark {

asio::awaitable<void> async_poll_date(std::shared_ptr<Context> ctx)
{
    asio::system_timer timer{co_await asio::this_coro::executor};
    for (;;) {
        const auto now = std::chrono::system_clock::now();

        ctx->now = fmt::gmtime(std::chrono::system_clock::to_time_t(now));

        timer.expires_at(now + std::chrono::seconds{1});
        co_await timer.async_wait(asio::use_awaitable);
    }
}

} // namespace skye_benchmark
