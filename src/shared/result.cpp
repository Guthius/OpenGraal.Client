#include <shared/result.hpp>

using namespace std;

namespace og::shared {
    auto make_error(string message) -> unexpected<string> {
        return unexpected(std::move(message));
    }
}
