#pragma once

#include "gui/gui_button_ctrl.hpp"
#include "gui/gui_check_box_ctrl.hpp"
#include "gui/gui_ml_text_ctrl.hpp"
#include "gui/gui_text_ctrl.hpp"
#include "gui/gui_text_edit_ctrl.hpp"
#include "gui/gui_window_ctrl.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class login_screen {
  public:
    using start_handler = std::function<void(const std::string &account, const std::string &password, const std::string &nickname, bool save_password)>;
    using create_handler = std::function<void(const std::string &account, const std::string &password)>;

    void attach(const std::shared_ptr<gui_control> &root);
    void update(float dt);

    void show();
    void hide();

    [[nodiscard]] auto is_visible() const -> bool;

    void set_status(const std::string &text);
    void set_busy(bool busy);
    void set_fields(const std::string &account, const std::string &password, const std::string &nickname);
    void set_accounts(const std::vector<std::string> &accounts);
    void set_save_password(bool save);

    void close_create_account();

    void on_start(start_handler handler) { start_handler_ = std::move(handler); }
    void on_create(create_handler handler) { create_handler_ = std::move(handler); }

  private:
    void build_login_window();
    void build_create_window();
    void layout(float width, float height);
    static void centre(const Vector2 &size, const std::shared_ptr<gui_control> &control);
    void request_login();
    void request_create();
    void show_create_account();

    Vector2 screen_{};
    std::shared_ptr<gui_window_ctrl> window_;
    std::shared_ptr<gui_text_ctrl> logo_;
    std::shared_ptr<gui_control> nick_group_;
    std::shared_ptr<gui_text_edit_ctrl> nick_edit_;
    std::shared_ptr<gui_control> credentials_group_;
    std::shared_ptr<gui_text_edit_ctrl> account_edit_;
    std::shared_ptr<gui_text_edit_ctrl> password_edit_;
    std::shared_ptr<gui_check_box_ctrl> save_password_;
    std::shared_ptr<gui_text_ctrl> status_;
    std::shared_ptr<gui_button_ctrl> start_;

    std::shared_ptr<gui_window_ctrl> create_window_;
    std::shared_ptr<gui_ml_text_ctrl> create_blurb_;
    std::shared_ptr<gui_text_edit_ctrl> create_account_edit_;
    std::shared_ptr<gui_text_edit_ctrl> create_password_edit_;
    std::shared_ptr<gui_text_edit_ctrl> create_verify_edit_;
    std::shared_ptr<gui_check_box_ctrl> create_agree_;
    std::shared_ptr<gui_text_ctrl> create_status_;

    start_handler start_handler_;
    create_handler create_handler_;
};

auto get_login_screen() -> login_screen &;
