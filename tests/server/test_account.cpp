#include <catch2/catch_test_macros.hpp>

#include <server/account.hpp>

#include <filesystem>
#include <fstream>

using namespace std;
using namespace og;

TEST_CASE("an account round-trips through its file, weapons included", "[server]") {
    const auto path = filesystem::temp_directory_path() / "opengraal-account-test" / "carol.txt";

    remove_all(path.parent_path());

    server::account stored;
    stored.name = "carol@example.com";
    stored.password_hash = server::hash_password("secret");
    stored.nickname = "Carol";
    stored.level = "testworld.nw";
    stored.x = 12.5f;
    stored.y = 34.0f;
    stored.head = "head1.png";
    stored.developer = true;
    stored.weapons = {"Systems/Initialisation", "+Fists"};

    REQUIRE(server::save_account(path, stored));

    const auto loaded = server::load_account(path);

    REQUIRE(loaded);
    REQUIRE(loaded->name == "carol@example.com");
    REQUIRE(loaded->nickname == "Carol");
    REQUIRE(loaded->level == "testworld.nw");
    REQUIRE(loaded->x == 12.5f);
    REQUIRE(loaded->head == "head1.png");
    REQUIRE(loaded->developer);
    REQUIRE(loaded->weapons == vector<string>{"Systems/Initialisation", "+Fists"});
    REQUIRE(loaded->verify("secret") == server::verify_result::ok);

    remove_all(path.parent_path());
}

TEST_CASE("an account file without a developer line is not a developer", "[server]") {
    const auto path = filesystem::temp_directory_path() / "opengraal-developer-test" / "erin@example.com.txt";

    remove_all(path.parent_path());
    create_directories(path.parent_path());

    {
        ofstream ofs(path);
        ofs << "OGACC001\nNAME erin@example.com\n";
    }

    const auto loaded = server::load_account(path);

    REQUIRE(loaded);
    REQUIRE_FALSE(loaded->developer);

    remove_all(path.parent_path());
}

TEST_CASE("a password is stored as argon2id, salted per call", "[server]") {
    const auto first = server::hash_password("secret");
    const auto second = server::hash_password("secret");

    REQUIRE(first.starts_with("$argon2id$"));
    REQUIRE(first != second);

    server::account stored;
    stored.password_hash = first;

    REQUIRE(stored.verify("secret") == server::verify_result::ok);
    REQUIRE(stored.verify("wrong") == server::verify_result::failed);
    REQUIRE(stored.verify("") == server::verify_result::failed);
}

TEST_CASE("a legacy checksum still verifies, and asks to be rehashed", "[server]") {
    const auto path = filesystem::temp_directory_path() / "opengraal-legacy-test" / "dave@example.com.txt";

    remove_all(path.parent_path());
    create_directories(path.parent_path());

    {
        ofstream ofs(path);
        ofs << "OGACC001\nNAME dave@example.com\nPASSWORD f20ba9995e8a8783\n";
    }

    auto loaded = server::load_account(path);

    REQUIRE(loaded);
    REQUIRE(loaded->verify("secret") == server::verify_result::needs_rehash);
    REQUIRE(loaded->verify("wrong") == server::verify_result::failed);

    loaded->password_hash = server::hash_password("secret");

    REQUIRE(server::save_account(path, *loaded));

    const auto again = server::load_account(path);

    REQUIRE(again);
    REQUIRE(again->verify("secret") == server::verify_result::ok);

    remove_all(path.parent_path());
}

TEST_CASE("an account name has to be an e-mail address", "[server]") {
    REQUIRE(server::is_valid_email("a@b.co"));
    REQUIRE(server::is_valid_email("first.last+tag@sub.example.co.uk"));
    REQUIRE(server::is_valid_email("dhummel@gmail.com"));

    REQUIRE_FALSE(server::is_valid_email(""));
    REQUIRE_FALSE(server::is_valid_email("alice"));
    REQUIRE_FALSE(server::is_valid_email("a@"));
    REQUIRE_FALSE(server::is_valid_email("@b.com"));
    REQUIRE_FALSE(server::is_valid_email("a@b"));
    REQUIRE_FALSE(server::is_valid_email("a b@c.com"));
    REQUIRE_FALSE(server::is_valid_email("a@@b.com"));
    REQUIRE_FALSE(server::is_valid_email("a@b@c.com"));
    REQUIRE_FALSE(server::is_valid_email(".a@b.com"));
    REQUIRE_FALSE(server::is_valid_email("a.@b.com"));
    REQUIRE_FALSE(server::is_valid_email("a..b@c.com"));
    REQUIRE_FALSE(server::is_valid_email("a@b..com"));
    REQUIRE_FALSE(server::is_valid_email("a@-b.com"));
    REQUIRE_FALSE(server::is_valid_email(string(250, 'a') + "@example.com"));
    REQUIRE_FALSE(server::is_valid_email("../../etc/passwd@b.com"));
    REQUIRE_FALSE(server::is_valid_email("a/b@c.com"));
}
