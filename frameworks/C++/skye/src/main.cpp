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

#if 1

#include "mux.hpp"

#include <boost/asio/system_timer.hpp>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <skye/service.hpp>

#include <cstdio>
#include <exception>
#include <memory>

namespace asio = boost::asio;

asio::awaitable<void> update(std::shared_ptr<skye_benchmark::Context> ctx)
{
    using namespace std::chrono_literals;

    asio::system_timer timer{co_await asio::this_coro::executor};
    for (;;) {
        const auto now = std::chrono::system_clock::now();

        ctx->now = fmt::gmtime(std::chrono::system_clock::to_time_t(now));

        timer.expires_at(now + std::chrono::seconds{1});
        co_await timer.async_wait(asio::use_awaitable);
    }
}

int main()
{
    try {
        const skye_benchmark::Mux mux{
            std::make_shared<skye_benchmark::Context>()};

        asio::io_context ioc{1};

        co_spawn(ioc, update(mux.ctx), [](auto ptr) {
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

#elif 1

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <fmt/core.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "fields_alloc.hpp"

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace asio = boost::asio;     // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_stream = default_token::as_default_on_t<tcp::socket>;
using alloc_t = fields_alloc<char>;

using request_type = http::request<http::string_body>;
using response_type = http::response<http::string_body>;

// Return a response for the given request.
response_type handle_request(request_type&& req)
{
    // Respond to GET request
    response_type res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello, World!";
    res.keep_alive(req.keep_alive());
    res.prepare_payload();

    return res;
}

//------------------------------------------------------------------------------

// Handles an HTTP server connection
net::awaitable<void> do_session(tcp_stream stream)
{
    // This buffer is required to persist across reads
    beast::flat_buffer buffer{8192};
    alloc_t alloc{8192};

    // This lambda is used to send messages
    for (;;) {
        // Read a request
        request_type req;
        {
            auto [ec, bytes_read] =
                co_await http::async_read(stream, buffer, req);
            if (ec) {
                break;
            }
        }

        // Handle the request
        auto res = handle_request(std::move(req));

        // Determine if we should close the connection
        const bool keep_alive = res.keep_alive();

        // Send the response
        auto [ec, bytes_write] =
            co_await http::async_write(stream, std::move(res));

        if (ec) {
            break;
        }

        if (!keep_alive) {
            break;
        }
    }

    // Send a TCP shutdown
    beast::error_code ec;
    stream.shutdown(tcp_stream::shutdown_send, ec);

    // At this point the connection is closed gracefully
    // we ignore the error because the client might have
    // dropped the connection already.
}

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
net::awaitable<void> do_listen(tcp::endpoint endpoint)
{
    // Open the acceptor
    auto acceptor = tcp_acceptor{co_await net::this_coro::executor};
    acceptor.open(endpoint.protocol());

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true));

    // Bind to the server address
    acceptor.bind(endpoint);

    // Start listening for connections
    acceptor.listen(net::socket_base::max_listen_connections);

    for (;;) {
        auto [ec, stream] = co_await acceptor.async_accept();
        if (ec) {
            continue;
        }

        asio::co_spawn(
            acceptor.get_executor(), do_session(std::move(stream)),
            [](auto ptr) {
                if (ptr) {
                    std::rethrow_exception(ptr);
                }
            });
    }
}

int main()
{
    try {
        asio::io_context ioc{1};

        asio::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        // Spawn a listening port
        asio::co_spawn(
            ioc, do_listen(tcp::endpoint{tcp::v4(), 8080}), [](auto ptr) {
                if (ptr) {
                    std::rethrow_exception(ptr);
                }
            });

        ioc.run();
    } catch (const std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
        return EXIT_FAILURE;
    }
}

#elif 1

//
// Copyright (c) 2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, small
//
//------------------------------------------------------------------------------

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <fmt/core.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace my_program_state {
std::size_t request_count()
{
    static std::size_t count = 0;
    return ++count;
}

std::time_t now()
{
    return std::time(0);
}
} // namespace my_program_state

class http_connection : public std::enable_shared_from_this<http_connection> {
public:
    http_connection(tcp::socket socket) : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void start()
    {
        read_request();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::string_body> request_;

    // The response message.
    http::response<http::string_body> response_;

    // Asynchronously receive a complete request message.
    void read_request()
    {
        http::async_read(
            socket_, buffer_, request_,
            [self = shared_from_this()](auto ec, auto bytes_transferred) {
                if (!ec) {
                    self->process_request();
                }
            });
    }

    // Determine what needs to be done with the request message.
    void process_request()
    {
        response_ = {http::status::ok, request_.version()};
        response_.keep_alive(true);
        response_.set(http::field::content_type, "text/plain");
        response_.body() = "Hello, World!";
        response_.prepare_payload();

        write_response();
    }

    // Asynchronously transmit the response message.
    void write_response()
    {
        http::async_write(
            socket_, response_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (!ec) {
                    self->read_request();
                }
            });
    }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket, [&](beast::error_code ec) {
        if (!ec)
            std::make_shared<http_connection>(std::move(socket))->start();
        http_server(acceptor, socket);
    });
}

int main()
{
    try {
        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {tcp::v4(), 8080}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    } catch (const std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
        return EXIT_FAILURE;
    }
}

#elif 1

//
// Copyright (c) 2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, fast
//
//------------------------------------------------------------------------------

#include "fields_alloc.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

// Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if (iequals(ext, ".htm"))
        return "text/html";
    if (iequals(ext, ".html"))
        return "text/html";
    if (iequals(ext, ".php"))
        return "text/html";
    if (iequals(ext, ".css"))
        return "text/css";
    if (iequals(ext, ".txt"))
        return "text/plain";
    if (iequals(ext, ".js"))
        return "application/javascript";
    if (iequals(ext, ".json"))
        return "application/json";
    if (iequals(ext, ".xml"))
        return "application/xml";
    if (iequals(ext, ".swf"))
        return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))
        return "video/x-flv";
    if (iequals(ext, ".png"))
        return "image/png";
    if (iequals(ext, ".jpe"))
        return "image/jpeg";
    if (iequals(ext, ".jpeg"))
        return "image/jpeg";
    if (iequals(ext, ".jpg"))
        return "image/jpeg";
    if (iequals(ext, ".gif"))
        return "image/gif";
    if (iequals(ext, ".bmp"))
        return "image/bmp";
    if (iequals(ext, ".ico"))
        return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff"))
        return "image/tiff";
    if (iequals(ext, ".tif"))
        return "image/tiff";
    if (iequals(ext, ".svg"))
        return "image/svg+xml";
    if (iequals(ext, ".svgz"))
        return "image/svg+xml";
    return "application/text";
}

class http_worker {
public:
    http_worker(http_worker const&) = delete;
    http_worker& operator=(http_worker const&) = delete;

    http_worker(tcp::acceptor& acceptor, const std::string& doc_root)
        : acceptor_(acceptor), doc_root_(doc_root)
    {
    }

    void start()
    {
        accept();
    }

private:
    using alloc_t = fields_alloc<char>;
    // using request_body_t =
    // http::basic_dynamic_body<beast::flat_static_buffer<1024 * 1024>>;
    using request_body_t = http::string_body;

    // The acceptor used to listen for incoming connections.
    tcp::acceptor& acceptor_;

    // The path to the root of the document directory.
    std::string doc_root_;

    // The socket for the currently connected client.
    tcp::socket socket_{acceptor_.get_executor()};

    // The buffer for performing reads
    beast::flat_static_buffer<8192> buffer_;

    // The allocator used for the fields in the request and reply.
    alloc_t alloc_{8192};

    // The parser for reading the requests
    boost::optional<http::request_parser<request_body_t, alloc_t>> parser_;

    // The timer putting a time limit on requests.
    net::steady_timer request_deadline_{
        acceptor_.get_executor(),
        (std::chrono::steady_clock::time_point::max)()};

    // The string-based response message.
    boost::optional<
        http::response<http::string_body, http::basic_fields<alloc_t>>>
        string_response_;

    // The string-based response serializer.
    boost::optional<http::response_serializer<
        http::string_body, http::basic_fields<alloc_t>>>
        string_serializer_;

    // The file-based response message.
    boost::optional<
        http::response<http::file_body, http::basic_fields<alloc_t>>>
        file_response_;

    // The file-based response serializer.
    boost::optional<
        http::response_serializer<http::file_body, http::basic_fields<alloc_t>>>
        file_serializer_;

    void accept()
    {
        // Clean up any previous connection.
        beast::error_code ec;
        socket_.close(ec);
        buffer_.consume(buffer_.size());

        acceptor_.async_accept(socket_, [this](beast::error_code ec) {
            if (ec) {
                accept();
            } else {
                // Request must be fully processed within 60 seconds.
                request_deadline_.expires_after(std::chrono::seconds(60));

                read_request();
            }
        });
    }

    void read_request()
    {
        // On each read the parser needs to be destroyed and
        // recreated. We store it in a boost::optional to
        // achieve that.
        //
        // Arguments passed to the parser constructor are
        // forwarded to the message object. A single argument
        // is forwarded to the body constructor.
        //
        // We construct the dynamic body with a 1MB limit
        // to prevent vulnerability to buffer attacks.
        //
        parser_.emplace(
            std::piecewise_construct, std::make_tuple(),
            std::make_tuple(alloc_));

        http::async_read(
            socket_, buffer_, *parser_,
            [this](beast::error_code ec, std::size_t) {
                if (ec)
                    accept();
                else
                    process_request(parser_->get());
            });
    }

    void process_request(
        http::request<request_body_t, http::basic_fields<alloc_t>> const& req)
    {
        switch (req.method()) {
        case http::verb::get:
            send_file(req.target());
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            send_bad_response(
                http::status::bad_request,
                "Invalid request-method '" + std::string(req.method_string()) +
                    "'\r\n");
            break;
        }
    }

    void send_bad_response(http::status status, std::string const& error)
    {
        string_response_.emplace(
            std::piecewise_construct, std::make_tuple(),
            std::make_tuple(alloc_));

        string_response_->result(status);
        string_response_->keep_alive(false);
        string_response_->set(http::field::server, "Beast");
        string_response_->set(http::field::content_type, "text/plain");
        string_response_->body() = error;
        string_response_->prepare_payload();

        string_serializer_.emplace(*string_response_);

        http::async_write(
            socket_, *string_serializer_,
            [this](beast::error_code ec, std::size_t) {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                string_serializer_.reset();
                string_response_.reset();
                accept();
            });
    }

    void send_file(beast::string_view target)
    {
        // Request path must be absolute and not contain "..".
        if (target.empty() || target[0] != '/' ||
            target.find("..") != std::string::npos) {
            send_bad_response(http::status::not_found, "File not found\r\n");
            return;
        }

        std::string full_path = doc_root_;
        full_path.append(target.data(), target.size());

        http::file_body::value_type file;
        beast::error_code ec;
        file.open(full_path.c_str(), beast::file_mode::read, ec);
        if (ec) {
            send_bad_response(http::status::not_found, "File not found\r\n");
            return;
        }

        file_response_.emplace(
            std::piecewise_construct, std::make_tuple(),
            std::make_tuple(alloc_));

        file_response_->result(http::status::ok);
        file_response_->keep_alive(false);
        file_response_->set(http::field::server, "Beast");
        file_response_->set(
            http::field::content_type, mime_type(std::string(target)));
        file_response_->body() = std::move(file);
        file_response_->prepare_payload();

        file_serializer_.emplace(*file_response_);

        http::async_write(
            socket_, *file_serializer_,
            [this](beast::error_code ec, std::size_t) {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                file_serializer_.reset();
                file_response_.reset();
                accept();
            });
    }
};

int main(int argc, char* argv[])
{
    try {
        // Check command line arguments.
        if (argc != 6) {
            std::cerr << "Usage: http_server_fast <address> <port> <doc_root> "
                         "<num_workers> {spin|block}\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    http_server_fast 0.0.0.0 80 . 100 block\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    http_server_fast 0::0 80 . 100 block\n";
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));
        std::string doc_root = argv[3];
        int num_workers = std::atoi(argv[4]);
        bool spin = (std::strcmp(argv[5], "spin") == 0);

        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};

        std::list<http_worker> workers;
        for (int i = 0; i < num_workers; ++i) {
            workers.emplace_back(acceptor, doc_root);
            workers.back().start();
        }

        if (spin)
            for (;;)
                ioc.poll();
        else
            ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

#elif 0

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/write.hpp>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <array>
#include <cstdio>
#include <ctime>
#include <exception>

namespace asio = boost::asio;

using asio::ip::tcp;
using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;

constexpr std::string_view kDate{"Sat, 01 Jan 2000 12:00:00"};

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
        "Date: Sat, 01 Jan 2000 12:00:00 GMT\r\n"
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

            itr = std::copy(
                kResponse.begin(), kResponse.begin() + kDateOffset, itr);
            itr = std::copy(ctx->date.begin(), ctx->date.end(), itr);
            itr = std::copy(
                kResponse.begin() + kDateOffset + kDate.size(), kResponse.end(),
                itr);

            if (pos + kDelim.size() >= view.size()) {
                break;
            }

            view.remove_prefix(pos + kDelim.size());
        }

        const std::size_t max_size_in_bytes = std::distance(out.begin(), itr);

        auto [e2, nwritten] = co_await asio::async_write(
            socket, asio::buffer(out, max_size_in_bytes));
        if (nwritten != max_size_in_bytes) {
            break;
        }
    }
}

asio::awaitable<void> update(Context* ctx)
{
    default_token::as_default_on_t<asio::system_timer> timer{
        co_await asio::this_coro::executor};

    for (;;) {
        const auto now = std::chrono::system_clock::now();
        fmt::format_to_n(
            ctx->date.begin(), ctx->date.size(), "{:%a, %d %b %Y %T}", now);

        // fmt::print("{}\n", std::string_view{ctx->date.data(),
        // ctx->date.size()});

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

int main()
{
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
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}

#else

#include "mux.hpp"

#include <fmt/core.h>
#include <skye/service.hpp>

#include <cstdio>
#include <exception>

int main()
{
    try {
        const skye_benchmark::Mux mux;

        skye::run(8080, mux);

        return 0;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}

#endif

/*
int half(int x)
{
    return x / 2;
}

int main()
{
    int y = 0;
    for (int x = 0; x < 1000000; ++x) {
        for (int z = 0; z < 100000; ++z) {
            y = half(x) + half(z);
        }
    }

    return 0;
}
*/