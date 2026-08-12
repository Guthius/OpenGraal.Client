#include <net/acceptor.hpp>

using namespace std;
using asio::ip::tcp;

namespace og::net {
    acceptor::acceptor(asio::io_context &io, handler on_accepted)
        : io_(io), acceptor_(io), on_accepted_(std::move(on_accepted)) {
    }

    auto acceptor::listen(const uint16_t port) -> bool {
        asio::error_code code;

        const auto endpoint = tcp::endpoint(tcp::v4(), port);

        if (acceptor_.open(endpoint.protocol(), code); code) {
            return false;
        }

        acceptor_.set_option(tcp::acceptor::reuse_address(true), code);

        if (acceptor_.bind(endpoint, code); code) {
            return false;
        }

        if (acceptor_.listen(tcp::socket::max_listen_connections, code); code) {
            return false;
        }

        accept_next();

        return true;
    }

    void acceptor::close() {
        asio::error_code code;
        acceptor_.close(code);
    }

    auto acceptor::port() const -> uint16_t {
        asio::error_code code;
        const auto endpoint = acceptor_.local_endpoint(code);

        return code ? 0 : endpoint.port();
    }

    void acceptor::accept_next() {
        acceptor_.async_accept([this](const asio::error_code &code, tcp::socket socket) {
            if (code) {
                return;
            }

            if (on_accepted_) {
                on_accepted_(connection::create(std::move(socket)));
            }

            accept_next();
        });
    }
}
