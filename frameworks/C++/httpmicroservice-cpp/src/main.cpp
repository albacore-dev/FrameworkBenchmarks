#if 0

#include "handler.hpp"

#include <httpmicroservice/format.hpp>
#include <httpmicroservice/service.hpp>

#include <boost/asio/signal_set.hpp>
#include <fmt/core.h>

#include <cstdio>
#include <exception>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;

int main()
{
    try {
        asio::io_context ioc{BOOST_ASIO_CONCURRENCY_HINT_UNSAFE};

        usrv::async_run(ioc, 8080, httpmicroservice_benchmark::Handler{});

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

#endif

#if 0

#include <fmt/core.h>

#include <boost/asio/signal_set.hpp>
#include <cstdio>
#include <exception>
#include <httpmicroservice/service.hpp>

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
        const int port = usrv::getenv_port();

        asio::io_context ioc;

        // Single threaded. The request handler runs in the http service thread.
        usrv::async_run(ioc, port, hello);

        // SIGTERM is sent by Docker to ask us to stop (politely)
        // SIGINT handles local Ctrl+C in a terminal
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

#endif

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/system_timer.hpp>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include <ctime>
#include <array>
#include <cstdio>
#include <exception>

namespace asio = boost::asio;

using asio::ip::tcp;
using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;

constexpr std::string_view kDate{"Wed, 17 Apr 2013 12:00:00"};

struct Context {
    std::array<char, kDate.size()> date;
};

asio::awaitable<void> echo(Context* ctx, tcp_socket socket)
{
    constexpr std::string_view kDelim{"\r\n\r\n"};
    constexpr std::string_view kResponse{
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "Server: Example\r\n"
        "Date: Wed, 17 Apr 2013 12:00:00 GMT\r\n"
        "\r\n"
        "Hello, World!"};
    constexpr std::size_t kDateOffset = kResponse.find(kDate);

    std::array<char, 4096> in{};
    std::array<char, 8192> out{};

    for (;;) {
        auto [e1, nread] = co_await socket.async_read_some(asio::buffer(in));
        if (nread == 0) {
           break;
        }

        auto itr = out.begin();

        std::string_view view{in.begin(), in.begin() + nread};
        while (!view.empty()) {
            const auto pos = view.find(kDelim);
            if (pos == std::string_view::npos) {
                break;
            }

            itr = std::copy(kResponse.begin(), kResponse.begin() + kDateOffset, itr);
            itr = std::copy(ctx->date.begin(), ctx->date.end(), itr);
            itr = std::copy(kResponse.begin() + kDateOffset + kDate.size(), kResponse.end(), itr);

            if (pos + kDelim.size() >= view.size()) {
                break;
            }

            view.remove_prefix(pos + kDelim.size());
        }

        const std::size_t max_size_in_bytes = std::distance(out.begin(), itr);

        auto [e2, nwritten] = co_await asio::async_write(socket, asio::buffer(out, max_size_in_bytes));
        if (nwritten != max_size_in_bytes) {
            break;
        }
    }
}

asio::awaitable<void> update(Context* ctx)
{
    default_token::as_default_on_t<asio::system_timer> timer{co_await asio::this_coro::executor};

    for (;;) {
        const auto now = std::chrono::system_clock::now();
        fmt::format_to_n(ctx->date.begin(), ctx->date.size(), "{:%a, %d %b %Y %T}", now);

        // fmt::print("{}\n", std::string_view{ctx->date.data(), ctx->date.size()});

        timer.expires_at(now + std::chrono::seconds{1});
        co_await timer.async_wait();
    }
}

asio::awaitable<void> listener(Context* ctx)
{
    auto ex = co_await asio::this_coro::executor;

    co_spawn(ex, update(ctx), [](auto ptr) {
        if (ptr) {
            std::rethrow_exception(ptr);
        }
    });

    tcp_acceptor acceptor{ex, {tcp::v4(), 8080}};
    for (;;) {
        auto [ec, socket] = co_await acceptor.async_accept();
        if (!socket.is_open()) {
            continue;
        }

        socket.set_option(tcp::no_delay{true}, ec);

        co_spawn(ex, echo(ctx, std::move(socket)), [](auto ptr) {
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });
    }
}

int main() {
    try {
        auto ctx = std::make_shared<Context>();

        asio::io_context ioc{1};

        asio::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        co_spawn(ioc, listener(ctx.get()), [](auto ptr) {
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        std::fprintf(stderr, "Exception: %s\n", e.what());
    }

    return -1;
}
