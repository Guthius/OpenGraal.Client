#include "random.hpp"

#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>

#include <bcrypt.h>
#else
#include <sys/random.h>
#endif

namespace og::shared {
    auto random_bytes(const std::size_t count) -> std::vector<std::uint8_t> {
        std::vector<std::uint8_t> bytes(count);
        if (count == 0) {
            return bytes;
        }

#if defined(_WIN32)
        if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(count), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
            throw std::runtime_error("no system entropy available");
        }
#else
        for (std::size_t filled = 0; filled < count;) {
            const auto read = getrandom(bytes.data() + filled, count - filled, 0);
            if (read <= 0) {
                throw std::runtime_error("no system entropy available");
            }

            filled += static_cast<std::size_t>(read);
        }
#endif

        return bytes;
    }

    auto equals_constant_time(const std::string_view left, const std::string_view right) -> bool {
        if (left.size() != right.size()) {
            return false;
        }

        unsigned char difference = 0;
        for (std::size_t i = 0; i < left.size(); ++i) {
            difference |= static_cast<unsigned char>(left[i]) ^ static_cast<unsigned char>(right[i]);
        }

        return difference == 0;
    }
}
