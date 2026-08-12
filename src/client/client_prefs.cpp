#include "client_prefs.hpp"

#include <shared/config.hpp>
#include <shared/token.hpp>

#include <algorithm>
#include <fstream>
#include <ranges>

namespace {
    constexpr std::string_view prefs_path = "prefs.txt";
    constexpr std::string_view obfuscation_key = "opengraal-prefs";

    client_prefs instance;

    auto obfuscate(const std::string &text) -> std::string {
        std::string result;
        result.reserve(text.size());

        for (std::size_t i = 0; i < text.size(); ++i) {
            result += static_cast<char>(text[i] ^ obfuscation_key[i % obfuscation_key.size()]);
        }

        return result;
    }
}

auto get_client_prefs() -> client_prefs & {
    return instance;
}

void client_prefs::load() {
    entries_.clear();
    last_account_.clear();

    const auto settings = og::shared::load_settings(std::filesystem::path(prefs_path));
    if (!settings) {
        return;
    }

    last_account_ = settings->get("last");

    for (const auto &account : settings->get_list("account")) {
        entry next;
        next.account = account;
        next.nickname = settings->get("nick_" + account);

        if (const auto stored = settings->get("pass_" + account); !stored.empty()) {
            if (const auto decoded = og::shared::base64url_decode(stored)) {
                next.password = obfuscate(*decoded);
            }
        }

        entries_.push_back(std::move(next));
    }
}

void client_prefs::save() const {
    std::ofstream ofs(std::filesystem::path(prefs_path), std::ios::trunc);
    if (!ofs) {
        return;
    }

    ofs << "# Written by the client. A saved password is obfuscated, not encrypted.\n";

    if (!last_account_.empty()) {
        ofs << "last=" << last_account_ << '\n';
    }

    for (const auto &next : entries_) {
        ofs << "account=" << next.account << '\n';

        if (!next.nickname.empty()) {
            ofs << "nick_" << next.account << '=' << next.nickname << '\n';
        }

        if (!next.password.empty()) {
            ofs << "pass_" << next.account << '=' << og::shared::base64url_encode(obfuscate(next.password)) << '\n';
        }
    }
}

auto client_prefs::accounts() const -> std::vector<std::string> {
    std::vector<std::string> names;
    names.reserve(entries_.size());

    for (const auto &next : entries_) {
        names.push_back(next.account);
    }

    return names;
}

auto client_prefs::find(const std::string &account) const -> entry {
    const auto match = std::ranges::find(entries_, account, &entry::account);

    return match == entries_.end() ? entry{} : *match;
}

auto client_prefs::last() const -> entry {
    return last_account_.empty() ? entry{} : find(last_account_);
}

void client_prefs::remember(const std::string &account, const std::string &password, const std::string &nickname, const bool save_password) {
    if (account.empty()) {
        return;
    }

    last_account_ = account;

    auto match = std::ranges::find(entries_, account, &entry::account);
    if (match == entries_.end()) {
        entries_.push_back({.account = account});
        match = std::prev(entries_.end());
    }

    match->nickname = nickname;
    match->password = save_password ? password : std::string{};

    save();
}
