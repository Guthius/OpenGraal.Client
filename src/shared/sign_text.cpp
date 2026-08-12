#include <shared/sign_text.hpp>

using namespace std;

namespace og::shared {
    namespace {
        constexpr string_view sign_characters =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
            "0123456789!?-.,#>()#####\"####':/~&### <####;\n";
    }

    auto decode_sign_text(const string_view str) -> string {
        string text;
        text.reserve(str.size());

        for (const auto ch : str) {
            const auto index = static_cast<size_t>(static_cast<unsigned char>(ch)) - 32;
            if (index >= sign_characters.size()) {
                continue;
            }

            text += sign_characters[index];
        }

        return text;
    }

    auto encode_sign_text(const string_view str) -> string {
        string encoded;
        encoded.reserve(str.size());

        for (const auto ch : str) {
            const auto index = sign_characters.find(ch);
            if (index == string_view::npos) {
                continue;
            }

            encoded += static_cast<char>(index + 32);
        }

        return encoded;
    }
}
