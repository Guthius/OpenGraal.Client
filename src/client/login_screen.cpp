#include "login_screen.hpp"

#include "dev_input.hpp"
#include "gui/gui_control_profile.hpp"

#include <raylib.h>

#include <cmath>

namespace {
    constexpr Vector2 window_size{568, 456};
    constexpr Vector2 create_size{420, 372};
    constexpr float title_height = 24.0f;

    constexpr float content_x = 84.0f;
    constexpr float content_width = 400.0f;
    constexpr float label_x = 18.0f;
    constexpr float field_x = 170.0f;
    constexpr float field_width = 212.0f;
    constexpr float field_height = 24.0f;

    constexpr Color body_color{123, 165, 239, 255};
    constexpr Color panel_color{61, 103, 177, 255};
    constexpr Color field_color{41, 82, 156, 255};
    constexpr Color selection_color{96, 144, 208, 255};
    constexpr Color panel_label_color{191, 223, 255, 255};
    constexpr Color body_label_color{16, 32, 72, 255};

    login_screen instance;

    auto window_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_texture("guiblue_window.png");
            profile->set_font("LiberationSans-Regular.ttf");
            profile->fill_color = body_color;
            profile->fill_color_hl = body_color;
        }

        return profile;
    }

    auto panel_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_font("LiberationSans-Regular.ttf");
            profile->border_style = gui_border_style::lowered;
            profile->border_color_na = BLACK;
            profile->border_thickness = 1;
            profile->fill_color = panel_color;
            profile->fill_color_hl = panel_color;
        }

        return profile;
    }

    auto label_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_font("LiberationSans-Regular.ttf");
            profile->font_color = panel_label_color;
        }

        return profile;
    }

    auto body_label_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_font("LiberationSans-Regular.ttf");
            profile->font_color = body_label_color;
        }

        return profile;
    }

    auto edit_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_texture("guiblue_textedit.png");
            profile->set_font("LiberationSans-Regular.ttf");
            profile->fill_color = field_color;
            profile->fill_color_hl = selection_color;
            profile->font_color = panel_label_color;
            profile->font_color_sel = WHITE;
            profile->border_color = BLACK;
        }

        return profile;
    }

    auto button_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_texture("guiblue_button.png");
            profile->set_font("LiberationSans-Regular.ttf");
        }

        return profile;
    }

    auto logo_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_font("LiberationSans-Bold.ttf");
            profile->set_font_size(34);
            profile->font_color = {50, 50, 50, 255};
        }

        return profile;
    }

    auto check_profile() -> const std::shared_ptr<gui_control_profile> & {
        static std::shared_ptr<gui_control_profile> profile;
        if (!profile) {
            profile = std::make_shared<gui_control_profile>();
            profile->set_texture("guiblue_check.png");
            profile->set_font("LiberationSans-Regular.ttf");
            profile->font_color = panel_label_color;
        }

        return profile;
    }

    auto make_label(const std::string &text, const Vector2 position) -> std::shared_ptr<gui_text_ctrl> {
        auto label = std::make_shared<gui_text_ctrl>();
        label->set_profile(label_profile());
        label->set_position(position);
        label->set_size({140, 20});
        label->set_text(text);

        return label;
    }

    auto make_edit(const Vector2 position, const Vector2 size, const size_t max_length, const bool password) -> std::shared_ptr<gui_text_edit_ctrl> {
        auto edit = std::make_shared<gui_text_edit_ctrl>();
        edit->set_profile(edit_profile());
        edit->set_position(position);
        edit->set_size(size);
        edit->set_max_length(max_length);
        edit->set_password(password);

        return edit;
    }

    auto make_button(const std::string &text, const Vector2 position, const Vector2 size) -> std::shared_ptr<gui_button_ctrl> {
        auto button = std::make_shared<gui_button_ctrl>();
        button->set_profile(button_profile());
        button->set_position(position);
        button->set_size(size);
        button->set_text(text);

        return button;
    }

    auto make_panel(const Vector2 position, const Vector2 size) -> std::shared_ptr<gui_control> {
        auto panel = std::make_shared<gui_control>();
        panel->set_profile(panel_profile());
        panel->set_position(position);
        panel->set_size(size);

        return panel;
    }
}

auto get_login_screen() -> login_screen & {
    return instance;
}

void login_screen::attach(const std::shared_ptr<gui_control> &root) {
    build_login_window();
    build_create_window();

    root->add_child(window_);
    root->add_child(create_window_);

    layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
}

void login_screen::build_login_window() {
    window_ = std::make_shared<gui_window_ctrl>();
    window_->set_profile(window_profile());
    window_->set_size(window_size);
    window_->set_text("Login");

    logo_ = std::make_shared<gui_text_ctrl>();
    logo_->set_profile(logo_profile());
    logo_->set_text("OpenGraal");
    logo_->set_position({std::round((window_size.x - logo_profile()->measure_text(logo_->get_text()).x) / 2.0f), title_height + 40});

    nick_group_ = make_panel({content_x, title_height + 110}, {content_width, 46});
    nick_group_->add_child(make_label("Nickname:", {label_x, 13}));

    nick_edit_ = make_edit({field_x, 11}, {field_width, field_height}, 32, false);
    nick_group_->add_child(nick_edit_);

    credentials_group_ = make_panel({content_x, title_height + 172}, {content_width, 110});
    credentials_group_->add_child(make_label("E-mail or Account:", {label_x, 14}));
    credentials_group_->add_child(make_label("Password:", {label_x, 48}));

    account_edit_ = make_edit({field_x, 12}, {field_width, field_height}, 254, false);
    password_edit_ = make_edit({field_x, 46}, {field_width, field_height}, 64, true);

    credentials_group_->add_child(account_edit_);
    credentials_group_->add_child(password_edit_);

    save_password_ = std::make_shared<gui_check_box_ctrl>();
    save_password_->set_profile(check_profile());
    save_password_->set_position({field_x, 80});
    save_password_->set_size({180, 18});
    save_password_->set_text("Save password");
    credentials_group_->add_child(save_password_);

    status_ = std::make_shared<gui_text_ctrl>();
    status_->set_profile(body_label_profile());
    status_->set_position({content_x, title_height + 294});
    status_->set_size({content_width, 20});

    start_ = make_button("Start", {content_x + ((content_width - 120) / 2), title_height + 324}, {120, 30});
    start_->clicked = [this] { request_login(); };

    auto member = make_button("Become a Member", {content_x, title_height + 380}, {190, 28});
    member->clicked = [this] { show_create_account(); };

    auto options = make_button("Options", {content_x + 210, title_height + 380}, {190, 28});
    options->set_disabled(true);

    nick_edit_->set_next_focus(account_edit_);
    account_edit_->set_next_focus(password_edit_);
    password_edit_->set_next_focus(nick_edit_);

    account_edit_->submitted = [this] { password_edit_->focus(); };
    password_edit_->submitted = [this] { request_login(); };
    nick_edit_->submitted = [this] { account_edit_->focus(); };

    window_->add_child(logo_);
    window_->add_child(nick_group_);
    window_->add_child(credentials_group_);
    window_->add_child(status_);
    window_->add_child(start_);
    window_->add_child(member);
    window_->add_child(options);
}

void login_screen::build_create_window() {
    constexpr float margin = 16.0f;
    constexpr float inner_width = create_size.x - (margin * 2.0f);
    constexpr float create_label_x = 16.0f;
    constexpr float create_field_x = 152.0f;
    constexpr float create_field_width = 220.0f;

    create_window_ = std::make_shared<gui_window_ctrl>();
    create_window_->set_profile(window_profile());
    create_window_->set_size(create_size);
    create_window_->set_text("Create Account");
    create_window_->set_visible(false);

    auto blurb_panel = make_panel({margin, title_height + margin}, {inner_width, 76});

    create_blurb_ = std::make_shared<gui_ml_text_ctrl>();
    create_blurb_->set_profile(label_profile());
    create_blurb_->set_position({10, 8});
    create_blurb_->set_size({inner_width - 20, 60});
    create_blurb_->set_text(
        "To keep items and buy items you need an account. Type your e-mail address and "
        "choose a password, then click 'Continue'. The account is created straight away.");
    blurb_panel->add_child(create_blurb_);

    auto fields = make_panel({margin, title_height + 108}, {inner_width, 150});
    fields->add_child(make_label("E-mail:", {create_label_x, 16}));
    fields->add_child(make_label("Password:", {create_label_x, 50}));
    fields->add_child(make_label("Verify:", {create_label_x, 84}));

    create_account_edit_ = make_edit({create_field_x, 14}, {create_field_width, field_height}, 254, false);
    create_password_edit_ = make_edit({create_field_x, 48}, {create_field_width, field_height}, 64, true);
    create_verify_edit_ = make_edit({create_field_x, 82}, {create_field_width, field_height}, 64, true);

    fields->add_child(create_account_edit_);
    fields->add_child(create_password_edit_);
    fields->add_child(create_verify_edit_);

    create_agree_ = std::make_shared<gui_check_box_ctrl>();
    create_agree_->set_profile(check_profile());
    create_agree_->set_position({create_label_x, 116});
    create_agree_->set_size({inner_width - (create_label_x * 2.0f), 18});
    create_agree_->set_text("I agree to play nicely with other players");
    fields->add_child(create_agree_);

    create_status_ = std::make_shared<gui_text_ctrl>();
    create_status_->set_profile(body_label_profile());
    create_status_->set_position({margin, title_height + 268});
    create_status_->set_size({inner_width, 20});

    constexpr float button_width = 120.0f;
    constexpr float button_gap = 16.0f;
    constexpr float buttons_x = (create_size.x - ((button_width * 2.0f) + button_gap)) / 2.0f;

    auto continue_button = make_button("Continue", {buttons_x, title_height + 298}, {button_width, 28});
    continue_button->clicked = [this] { request_create(); };

    auto cancel = make_button("Cancel", {buttons_x + button_width + button_gap, title_height + 298}, {button_width, 28});
    cancel->clicked = [this] { close_create_account(); };

    create_account_edit_->set_next_focus(create_password_edit_);
    create_password_edit_->set_next_focus(create_verify_edit_);
    create_verify_edit_->set_next_focus(create_account_edit_);

    create_verify_edit_->submitted = [this] { request_create(); };

    create_window_->add_child(blurb_panel);
    create_window_->add_child(fields);
    create_window_->add_child(create_status_);
    create_window_->add_child(continue_button);
    create_window_->add_child(cancel);
}

void login_screen::centre(const Vector2 &size, const std::shared_ptr<gui_control> &control) {
    if (control) {
        control->set_position({
            std::round((size.x - control->get_size().x) / 2.0f),
            std::round((size.y - control->get_size().y) / 2.0f),
        });
    }
}

void login_screen::layout(const float width, const float height) {
    if (width == screen_.x && height == screen_.y) {
        return;
    }

    screen_ = {width, height};

    centre(screen_, window_);
    centre(screen_, create_window_);
}

void login_screen::update(float /*dt*/) {
    layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));

    if (!is_visible()) {
        return;
    }

    if (std::string account, password; dev_input::take_login(account, password)) {
        account_edit_->set_text(account);
        password_edit_->set_text(password);

        request_login();
    }
}

auto login_screen::is_visible() const -> bool {
    return window_ && window_->is_visible();
}

void login_screen::show() {
    if (!window_) {
        return;
    }

    window_->set_visible(true);
    window_->set_disabled(false);
    set_busy(false);

    if (account_edit_->get_text().empty()) {
        account_edit_->focus();
    } else {
        password_edit_->focus();
    }
}

void login_screen::hide() {
    if (!window_) {
        return;
    }

    close_create_account();

    window_->set_visible(false);

    for (const auto &field : {nick_edit_, account_edit_, password_edit_}) {
        field->blur();
    }
}

void login_screen::set_status(const std::string &text) {
    if (status_) {
        status_->set_text(text);
    }

    if (create_status_ && create_window_ && create_window_->is_visible()) {
        create_status_->set_text(text);
    }
}

void login_screen::set_busy(const bool busy) {
    if (start_) {
        start_->set_disabled(busy);
    }
}

void login_screen::set_fields(const std::string &account, const std::string &password, const std::string &nickname) {
    if (account_edit_) {
        account_edit_->set_text(account);
    }

    if (password_edit_) {
        password_edit_->set_text(password);
    }

    if (nick_edit_) {
        nick_edit_->set_text(nickname);
    }
}

void login_screen::set_accounts(const std::vector<std::string> &accounts) {
    if (account_edit_) {
        account_edit_->set_dropdown_rows(accounts);
    }
}

void login_screen::set_save_password(const bool save) {
    if (save_password_) {
        save_password_->set_checked(save);
    }
}

void login_screen::request_login() {
    if (!start_handler_ || !account_edit_ || start_->is_disabled()) {
        return;
    }

    const auto account = account_edit_->get_text();
    if (account.empty()) {
        set_status("Enter your e-mail or account name.");

        return;
    }

    set_status("Connecting...");
    set_busy(true);

    start_handler_(account, password_edit_->get_text(), nick_edit_->get_text(), save_password_->is_checked());
}

void login_screen::show_create_account() {
    if (!create_window_) {
        return;
    }

    create_account_edit_->set_text("");
    create_password_edit_->set_text("");
    create_verify_edit_->set_text("");
    create_agree_->set_checked(false);
    create_status_->set_text("");

    create_window_->set_visible(true);
    create_window_->bring_to_front();

    window_->set_disabled(true);

    create_account_edit_->focus();
}

void login_screen::close_create_account() {
    if (!create_window_ || !create_window_->is_visible()) {
        return;
    }

    create_window_->set_visible(false);
    window_->set_disabled(false);
    create_status_->set_text("");
}

void login_screen::request_create() {
    if (!create_handler_) {
        return;
    }

    const auto account = create_account_edit_->get_text();
    const auto password = create_password_edit_->get_text();

    if (account.empty()) {
        create_status_->set_text("Enter an e-mail address.");

        return;
    }

    if (password.size() < 6) {
        create_status_->set_text("Choose a password of at least 6 characters.");

        return;
    }

    if (password != create_verify_edit_->get_text()) {
        create_status_->set_text("The two passwords do not match.");

        return;
    }

    if (!create_agree_->is_checked()) {
        create_status_->set_text("You must agree before continuing.");

        return;
    }

    create_status_->set_text("Creating account...");

    create_handler_(account, password);
}
