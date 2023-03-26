#pragma once

#include "context.hpp"

#include <boost/asio/awaitable.hpp>
#include <skye/types.hpp>

#include <memory>

namespace skye_benchmark {

namespace http = boost::beast::http;

// Route HTTP requests for this web service to a handler. Mux is a copy
// constructable function object that adheres to the skye Handler concept.
//
// Shared resource data (database connection, current server time) are
// stored in the Context which is accessible from the handlers.
struct Mux {
    std::shared_ptr<Context> ctx;

    asio::awaitable<skye::response> operator()(skye::request req) const;

    skye::response json(const skye::request& req) const;
    skye::response db(const skye::request& req) const;
    skye::response queries(const skye::request& req) const;
    skye::response plaintext(const skye::request& req) const;

    // Initialize a response with:
    // - Status code
    // - HTTP/1.x version
    // - Server header
    // - Date header
    skye::response
    make_response(http::status status, const skye::request& req) const;
};

} // namespace skye_benchmark
