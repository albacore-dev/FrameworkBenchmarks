#pragma once

#include "database.hpp"

#include <boost/asio/awaitable.hpp>

#include <ctime>
#include <memory>

namespace skye_benchmark {

namespace asio = boost::asio;

// Shared resource data for the handlers.
struct Context {
    SQLiteContext db;
    std::tm now{};
};

asio::awaitable<void> async_poll_date(std::shared_ptr<Context> ctx);

} // namespace skye_benchmark
