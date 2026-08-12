#pragma once

#include <net/connection.hpp>

#include <functional>

namespace og::net {
    class acceptor {
      public:
        using handler = std::function<void(const connection_ptr &link)>;

        acceptor(asio::io_context &io, handler on_accepted);

        auto listen(uint16_t port) -> bool;
        void close();

        [[nodiscard]] auto listening() const -> bool { return acceptor_.is_open(); }
        [[nodiscard]] auto port() const -> uint16_t;

      private:
        void accept_next();

        asio::io_context &io_;
        asio::ip::tcp::acceptor acceptor_;
        handler on_accepted_;
    };
}
