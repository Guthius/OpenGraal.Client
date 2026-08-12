#include <net/connection.hpp>

#include <format>

using namespace std;
using asio::ip::tcp;

namespace og::net {
    namespace {
        constexpr size_t length_prefix = sizeof(uint32_t);
        constexpr size_t id_size = sizeof(uint16_t);

        auto read_length(const vector<uint8_t> &buffer) -> uint32_t {
            uint32_t length = 0;
            for (size_t i = 0; i < length_prefix; ++i) {
                length |= static_cast<uint32_t>(buffer[i]) << (i * 8);
            }

            return length;
        }

        auto describe(const tcp::socket &socket) -> string {
            asio::error_code code;
            const auto remote = socket.remote_endpoint(code);

            if (code) {
                return "unknown";
            }

            return format("{}:{}", remote.address().to_string(), remote.port());
        }
    }

    connection::connection(tcp::socket socket)
        : socket_(std::move(socket)),
          endpoint_(describe(socket_)) {
        asio::error_code code;
        socket_.set_option(tcp::no_delay(true), code);
    }

    auto connection::create(tcp::socket socket) -> connection_ptr {
        auto link = make_shared<connection>(std::move(socket));
        link->start();

        return link;
    }

    auto connection::connect(asio::io_context &io, const string_view host, const uint16_t port, const std::chrono::milliseconds timeout) -> connection_ptr {
        tcp::resolver resolver(io);
        tcp::socket socket(io);

        asio::error_code code = asio::error::would_block;
        tcp::resolver::results_type endpoints;

        io.restart();

        resolver.async_resolve(host, std::to_string(port), [&](const asio::error_code &error, const tcp::resolver::results_type &found) {
            code = error;
            endpoints = found;
        });

        io.run_for(timeout);

        if (code || endpoints.empty()) {
            resolver.cancel();

            return nullptr;
        }

        code = asio::error::would_block;

        io.restart();

        asio::async_connect(socket, endpoints, [&](const asio::error_code &error, const tcp::endpoint &) {
            code = error;
        });

        io.run_for(timeout);

        if (code) {
            asio::error_code ignored;
            socket.close(ignored);

            return nullptr;
        }

        return create(std::move(socket));
    }

    void connection::start() {
        read_some();
    }

    void connection::close() {
        if (!open_) {
            return;
        }

        open_ = false;

        asio::error_code code;
        socket_.shutdown(tcp::socket::shutdown_both, code);
        socket_.close(code);
    }

    void connection::fail() {
        open_ = false;

        asio::error_code code;
        socket_.close(code);
    }

    void connection::send(const packet_writer &writer) {
        if (!open_) {
            return;
        }

        outgoing_.push_back(writer.frame());

        if (!writing_) {
            write_next();
        }
    }

    void connection::write_next() {
        if (outgoing_.empty() || !open_) {
            writing_ = false;

            return;
        }

        writing_ = true;

        asio::async_write(
            socket_,
            asio::buffer(outgoing_.front()),
            [self = shared_from_this()](const asio::error_code &code, size_t) {
                if (code) {
                    self->fail();

                    return;
                }

                self->outgoing_.pop_front();
                self->write_next();
            });
    }

    void connection::read_some() {
        socket_.async_read_some(
            asio::buffer(buffer_),
            [self = shared_from_this()](const asio::error_code &code, const size_t read) {
                if (code) {
                    self->fail();

                    return;
                }

                self->incoming_.insert(self->incoming_.end(), self->buffer_.begin(), self->buffer_.begin() + static_cast<ptrdiff_t>(read));

                if (self->incoming_.size() > max_message_size) {
                    self->fail();

                    return;
                }

                self->decode();

                if (self->open_) {
                    self->read_some();
                }
            });
    }

    void connection::decode() {
        while (incoming_.size() >= length_prefix) {
            const auto length = read_length(incoming_);
            if (length > max_message_size || length < id_size) {
                fail();

                return;
            }

            if (incoming_.size() < length_prefix + length) {
                return;
            }

            const auto *start = incoming_.data() + length_prefix;
            const auto id = static_cast<message_id>(static_cast<uint16_t>(start[0]) | static_cast<uint16_t>(start[1]) << 8);

            messages_.emplace_back(id, vector(start + id_size, start + length));
            incoming_.erase(incoming_.begin(), incoming_.begin() + static_cast<ptrdiff_t>(length_prefix + length));
        }
    }

    auto connection::poll() -> optional<packet_reader> {
        if (messages_.empty()) {
            return nullopt;
        }

        auto message = std::move(messages_.front());
        messages_.pop_front();

        return message;
    }
}
