#pragma once

#include <string>
#include <vector>

class client_prefs {
  public:
    struct entry {
        std::string account;
        std::string password;
        std::string nickname;
    };

    void load();
    void save() const;

    [[nodiscard]] auto accounts() const -> std::vector<std::string>;
    [[nodiscard]] auto last() const -> entry;
    [[nodiscard]] auto find(const std::string &account) const -> entry;

    void remember(const std::string &account, const std::string &password, const std::string &nickname, bool save_password);

  private:
    std::vector<entry> entries_;
    std::string last_account_;
};

auto get_client_prefs() -> client_prefs &;
