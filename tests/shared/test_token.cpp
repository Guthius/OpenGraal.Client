#include <catch2/catch_test_macros.hpp>

#include <shared/random.hpp>
#include <shared/token.hpp>

using namespace std;
using namespace og::shared;

namespace {
    constexpr string_view secret = "a-test-secret-of-at-least-32-chars";
    constexpr int64_t now = 1'700'000'000;

    auto make_token(const string &account = "alice@example.com", const int64_t issued = now, const int64_t expires = now + 120) -> string {
        return issue_token({.account = account, .issued_at = issued, .expires_at = expires}, secret);
    }
}

TEST_CASE("base64url round trips and stays url safe", "[token]") {
    for (const auto &data : {""s, "a"s, "ab"s, "abc"s, "abcd"s, "abcde"s, "\x01\xff\xfe"s, "sure."s}) {
        const auto encoded = base64url_encode(data);

        REQUIRE(encoded.find('=') == string::npos);
        REQUIRE(encoded.find('+') == string::npos);
        REQUIRE(encoded.find('/') == string::npos);
        REQUIRE(base64url_decode(encoded) == data);
    }
}

TEST_CASE("base64url rejects malformed input", "[token]") {
    REQUIRE_FALSE(base64url_decode("a").has_value());
    REQUIRE_FALSE(base64url_decode("ab*d").has_value());
    REQUIRE_FALSE(base64url_decode("ab=d").has_value());
}

TEST_CASE("a token round trips", "[token]") {
    const auto token = make_token();

    REQUIRE_FALSE(token.empty());

    const auto claims = verify_token(token, secret, now);

    REQUIRE(claims.has_value());
    REQUIRE(claims->account == "alice@example.com");
    REQUIRE(claims->issued_at == now);
    REQUIRE(claims->expires_at == now + 120);
}

TEST_CASE("a token is refused by a different secret", "[token]") {
    REQUIRE_FALSE(verify_token(make_token(), "a-different-secret-also-32-chars-long", now).has_value());
}

TEST_CASE("a tampered token is refused", "[token]") {
    auto token = make_token();

    token.back() = token.back() == 'A' ? 'B' : 'A';

    REQUIRE_FALSE(verify_token(token, secret, now).has_value());
}

TEST_CASE("a token with a rewritten payload is refused", "[token]") {
    const auto token = make_token();
    const auto first = token.find('.');
    const auto second = token.find('.', first + 1);

    const auto forged = base64url_encode(R"({"sub":"root@example.com","iat":1700000000,"exp":1700000120})");
    const auto rebuilt = token.substr(0, first + 1) + forged + token.substr(second);

    REQUIRE_FALSE(verify_token(rebuilt, secret, now).has_value());
}

TEST_CASE("an unsigned token is refused", "[token]") {
    const auto header = base64url_encode(R"({"alg":"none","typ":"JWT"})");
    const auto payload = base64url_encode(R"({"sub":"alice@example.com","iat":1700000000,"exp":1700000120})");

    REQUIRE_FALSE(verify_token(header + "." + payload + ".", secret, now).has_value());
    REQUIRE_FALSE(verify_token(header + "." + payload, secret, now).has_value());
}

TEST_CASE("an expired token is refused", "[token]") {
    const auto token = make_token();

    REQUIRE(verify_token(token, secret, now + 119).has_value());
    REQUIRE_FALSE(verify_token(token, secret, now + 120).has_value());
    REQUIRE_FALSE(verify_token(token, secret, now + 6000).has_value());
}

TEST_CASE("a token issued in the future is refused", "[token]") {
    REQUIRE_FALSE(verify_token(make_token("alice@example.com", now + 3600, now + 7200), secret, now).has_value());
}

TEST_CASE("a malformed token is refused", "[token]") {
    REQUIRE_FALSE(verify_token("", secret, now).has_value());
    REQUIRE_FALSE(verify_token("onepart", secret, now).has_value());
    REQUIRE_FALSE(verify_token("two.parts", secret, now).has_value());
    REQUIRE_FALSE(verify_token(make_token() + ".extra", secret, now).has_value());
}

TEST_CASE("a short secret issues and verifies nothing", "[token]") {
    const auto token = issue_token({.account = "alice@example.com", .issued_at = now, .expires_at = now + 120}, "too-short");

    REQUIRE(token.empty());
    REQUIRE_FALSE(verify_token(make_token(), "too-short", now).has_value());
}

TEST_CASE("random bytes are drawn and compared safely", "[token]") {
    REQUIRE(random_bytes(0).empty());
    REQUIRE(random_bytes(32).size() == 32);
    REQUIRE(random_bytes(32) != random_bytes(32));

    REQUIRE(equals_constant_time("abc", "abc"));
    REQUIRE_FALSE(equals_constant_time("abc", "abd"));
    REQUIRE_FALSE(equals_constant_time("abc", "ab"));
    REQUIRE(equals_constant_time("", ""));
}
