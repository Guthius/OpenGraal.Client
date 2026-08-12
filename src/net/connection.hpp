#pragma once

#include <net/message.hpp>

#include <asio.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <optional>
#include <string>

namespace og::net {
    class connection;

    using connection_ptr = std::shared_ptr<connection>;

    class connection : public std::enable_shared_from_this<connection> {
      public:
        explicit connection(asio::ip::tcp::socket socket);

        connection(const connection &) = delete;
        auto operator=(const connection &) -> connection & = delete;

        static auto create(asio::ip::tcp::socket socket) -> connection_ptr;
        static auto connect(asio::io_context &io, std::string_view host, uint16_t port, std::chrono::milliseconds timeout = std::chrono::seconds(5)) -> connection_ptr;

        void start();
        void close();

        [[nodiscard]] auto connected() const -> bool { return open_; }
        [[nodiscard]] auto endpoint() const -> const std::string & { return endpoint_; }

        void send(const packet_writer &writer);
        auto poll() -> std::optional<packet_reader>;

      private:
        void read_some();
        void decode();
        void write_next();
        void fail();

        asio::ip::tcp::socket socket_;
        std::array<uint8_t, 8192> buffer_{};
        std::vector<uint8_t> incoming_;
        std::deque<std::vector<uint8_t>> outgoing_;
        std::deque<packet_reader> messages_;
        std::string endpoint_;
        bool open_ = true;
        bool writing_ = false;
    };
}
