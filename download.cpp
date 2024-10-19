#include "download.h"
#include "root_cert.h"

#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
inline void fail(beast::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTPS GET and prints the response
void do_https_session(std::string const &host,
                      std::string const &target,
                      int version,
                      net::io_context &ioc,
                      ssl::context &ctx,
                      http::response<http::dynamic_body> &res,
                      http::verb method,
                      net::yield_context yield) {

    beast::error_code ec;

    const std::string port{"443"};
    // These objects perform our I/O
    tcp::resolver resolver(ioc);
    ssl::stream<beast::tcp_stream> stream(ioc, ctx);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        ec.assign(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        std::cerr << ec.message() << "\n";
        return;
    }

    // Look up the domain name
    auto const results = resolver.async_resolve(host, port, yield[ec]);
    if (ec) return fail(ec, "resolve");

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    get_lowest_layer(stream).async_connect(results, yield[ec]);
    if (ec) return fail(ec, "connect");

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    stream.async_handshake(ssl::stream_base::client, yield[ec]);
    if (ec) return fail(ec, "handshake");

    // Set up an HTTP GET request message
    http::request<http::string_body> req{method, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(stream, req, yield[ec]);
    if (ec) return fail(ec, "write");

    // This buffer is used for reading and must be persisted
    beast::flat_buffer b;

    // Receive the HTTP response
    http::async_read(stream, b, res, yield[ec]);
    if (ec) return fail(ec, "read");

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream
    stream.async_shutdown(yield[ec]);

    if (ec && ec != net::ssl::error::stream_truncated) return fail(ec, "shutdown");
}


void do_http_session(
        std::string const &host,
        std::string const &target,
        int version,
        net::io_context &ioc,
        http::response<http::dynamic_body> &res,
        http::verb method,
        net::yield_context yield) {
    beast::error_code ec;
    const std::string port = "80";

    // These objects perform our I/O
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    // Look up the domain name
    auto const results = resolver.async_resolve(host, port, yield[ec]);
    if (ec) return fail(ec, "resolve");

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    stream.async_connect(results, yield[ec]);
    if (ec) return fail(ec, "connect");

    // Set up an HTTP GET request message
    http::request<http::string_body> req{method, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(stream, req, yield[ec]);
    if (ec) return fail(ec, "write");

    // This buffer is used for reading and must be persisted
    beast::flat_buffer b;

    // Receive the HTTP response
    http::async_read(stream, b, res, yield[ec]);
    if (ec) return fail(ec, "read");

    // Write the message to standard out
    //    std::cout << res << std::endl;

    // Gracefully close the socket
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if (ec && ec != beast::errc::not_connected) return fail(ec, "shutdown");

    // If we get here then the connection is closed gracefully
}


//------------------------------------------------------------------------------

bool download(std::string_view url) {
    // Parse arguments.
    const auto hostStart = url.find_first_not_of('/', 6);
    const auto hostEnd = url.find('/', 8);
    const std::string host{url.substr(hostStart, hostEnd - hostStart)};
    const std::string target{url.substr(hostEnd)};

    int version = 10;  // HTTPS 1.1

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_peer);

    http::response<http::dynamic_body> res;

    if (url.at(4) == 'S' || url.at(4) == 's') {
        // Launch the asynchronous operation
        boost::asio::spawn(ioc, std::bind(&do_https_session, host, target, version, std::ref(ioc), std::ref(ctx), std::ref(res), http::verb::get, std::placeholders::_1),
                           [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
    } else {
        // Launch the asynchronous operation
        //        return false;
        boost::asio::spawn(ioc, std::bind(&do_http_session, host, target, version, std::ref(ioc), std::ref(res), http::verb::get, std::placeholders::_1),
                           [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
    }
    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    if (res.result() != http::status::ok) { return false; }

    const std::string path{"./"};
    std::string filename{filenameFromUrl(url)};

    std::fstream file{path + filename, std::ios::binary | std::ios::out};
    if (file.is_open()) {
        file << beast::make_printable(res.body().data());
    } else {
        std::cout << "Unable to open file " << filename << std::endl;
    }
    file.close();
    return true;
}

int32_t getFileSize(std::string_view url) {
    // Parse arguments.
    const auto hostStart = url.find_first_not_of('/', 6);
    const auto hostEnd = url.find('/', 8);
    const std::string host{url.substr(hostStart, hostEnd - hostStart)};
    const std::string target{url.substr(hostEnd)};

    int version = 10;  // HTTPS 1.1

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_peer);

    http::response<http::dynamic_body> res;

    if (url.at(4) == 'S' || url.at(4) == 's') {
        // Launch the asynchronous operation
        boost::asio::spawn(ioc, std::bind(&do_https_session, host, target, version, std::ref(ioc), std::ref(ctx), std::ref(res), http::verb::head, std::placeholders::_1),
                           [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
    } else {
        // Launch the asynchronous operation
        //        return -1;
        boost::asio::spawn(ioc, std::bind(&do_http_session, host, target, version, std::ref(ioc), std::ref(res), http::verb::head, std::placeholders::_1),
                           [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
    }
    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    if (res.result() != http::status::ok) { return -1; }

    return atoll(res.at(http::field::content_length).data());
}

std::string filenameFromUrl(std::string_view url) {
    std::string filename{url.substr(url.rfind('/'))};
    if (const auto n = filename.rfind('?'); n != std::string::npos) {
        filename.erase(n);
    }
    return filename;
}