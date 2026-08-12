#include "editor_profiles.hpp"

#include <cmath>
#include <rlgl.h>

namespace {
    constexpr Color body_color{123, 165, 239, 255};
    constexpr Color canvas_color{32, 32, 40, 255};
    constexpr Color panel_color{61, 103, 177, 255};
    constexpr Color label_color{191, 223, 255, 255};
    constexpr Color field_color{41, 82, 156, 255};
    constexpr Color selection_color{96, 144, 208, 255};

    auto make_profile(const auto &configure) -> std::shared_ptr<gui_control_profile> {
        auto profile = std::make_shared<gui_control_profile>();
        configure(*profile);

        return profile;
    }
}

namespace editor_profiles {
    auto body() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.fill_color = body_color;
            p.fill_color_hl = body_color;
            p.fill_color_na = body_color;
        });

        return profile;
    }

    auto toolbar() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.fill_color = Color{28, 40, 72, 255};
            p.fill_color_hl = Color{28, 40, 72, 255};
            p.fill_color_na = Color{28, 40, 72, 255};
        });

        return profile;
    }

    auto divider() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.fill_color = Color{12, 18, 36, 255};
            p.fill_color_hl = Color{12, 18, 36, 255};
            p.fill_color_na = Color{12, 18, 36, 255};
        });

        return profile;
    }

    auto canvas() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.fill_color = BLACK;
            p.fill_color_hl = BLACK;
            p.fill_color_na = BLACK;
        });

        return profile;
    }

    auto panel() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.fill_color = panel_color;
            p.fill_color_hl = panel_color;
            p.fill_color_na = panel_color;
        });

        return profile;
    }

    auto palette() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.border_style = gui_border_style::lowered;
            p.border_color_na = BLACK;
            p.border_thickness = 1;
            p.fill_color = canvas_color;
            p.fill_color_hl = canvas_color;
            p.fill_color_na = canvas_color;
        });

        return profile;
    }

    auto button() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_button.png");
            p.set_font("LiberationSans-Regular.ttf");
        });

        return profile;
    }

    auto tab() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_tab.png");
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = WHITE;
        });

        return profile;
    }

    auto window() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_window.png");
            p.set_font("LiberationSans-Regular.ttf");
            p.fill_color = body_color;
            p.fill_color_hl = body_color;
            p.fill_color_na = body_color;
        });

        return profile;
    }

    auto label() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = label_color;
        });

        return profile;
    }

    auto dialog_label() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = Color{50, 50, 50, 255};
        });

        return profile;
    }

    auto list() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_textedit.png");
            p.set_font("LiberationSans-Regular.ttf");
            p.fill_color = field_color;
            p.fill_color_hl = field_color;
            p.fill_color_na = field_color;
        });

        return profile;
    }

    auto list_text() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = label_color;
            p.font_color_hl = WHITE;
            p.fill_color_hl = selection_color;
        });

        return profile;
    }

    auto status() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = Color{16, 32, 72, 255};
        });

        return profile;
    }

    auto scrollbar() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_scroll.png");
            p.fill_color = field_color;
            p.fill_color_hl = Color{160, 190, 240, 255};
            p.border_color = body_color;
            p.border_color_na = BLACK;
        });

        return profile;
    }

    auto check() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_check.png");
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = label_color;
        });

        return profile;
    }

    auto radio() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_radio.png");
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = label_color;
        });

        return profile;
    }

    auto edit() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_textedit.png");
            p.set_font("LiberationSans-Regular.ttf");
            p.fill_color = field_color;
            p.fill_color_hl = selection_color;
            p.font_color = label_color;
            p.font_color_sel = WHITE;
            p.border_color = BLACK;
        });

        return profile;
    }

    auto error_line() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_font("LiberationSans-Regular.ttf");
            p.font_color = BLACK;
            p.font_color_sel = BLACK;
            p.caret_color = BLACK;
            p.border_style = gui_border_style::single;
            p.border_color = BLACK;
            p.border_color_hl = BLACK;
            p.border_color_na = BLACK;
            p.border_thickness = 1;
            p.fill_color = Color{255, 255, 128, 255};
            p.fill_color_hl = Color{222, 206, 80, 255};
            p.fill_color_na = Color{255, 255, 128, 255};
        });

        return profile;
    }

    auto overlay() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.fill_color = BLACK;
            p.fill_color_hl = BLACK;
            p.fill_color_na = BLACK;
            p.set_transparency(0.35f);
        });

        return profile;
    }

    auto code() -> const std::shared_ptr<gui_control_profile> & {
        static auto profile = make_profile([](gui_control_profile &p) {
            p.set_texture("guiblue_textedit.png");
            p.set_font("LiberationMono-Regular.ttf");
            p.fill_color = field_color;
            p.fill_color_hl = selection_color;
            p.font_color = label_color;
            p.font_color_sel = WHITE;
            p.border_color = BLACK;
        });

        return profile;
    }

    auto make_label(const std::string &text, const Vector2 position, const float width) -> std::shared_ptr<gui_text_ctrl> {
        auto control = std::make_shared<gui_text_ctrl>();
        control->set_profile(dialog_label());
        control->set_position(position);
        control->set_size({width, 20});
        control->set_text(text);

        return control;
    }

    auto make_edit(const Vector2 position, const Vector2 size) -> std::shared_ptr<gui_text_edit_ctrl> {
        auto control = std::make_shared<gui_text_edit_ctrl>();
        control->set_profile(edit());
        control->set_position(position);
        control->set_size(size);

        return control;
    }

    auto make_button(const std::string &text, const float width) -> std::shared_ptr<gui_button_ctrl> {
        auto control = std::make_shared<gui_button_ctrl>();
        control->set_profile(button());
        control->set_size({width, 22});
        control->set_text(text);

        return control;
    }

    void centre_window(gui_control &window, const Vector2 size) {
        window.set_position({
            std::round((static_cast<float>(GetScreenWidth()) - size.x) / 2.0f),
            std::round((static_cast<float>(GetScreenHeight()) - size.y) / 2.0f),
        });
    }

    void begin_invert() {
        rlDrawRenderBatchActive();
        rlSetBlendFactors(RL_ONE_MINUS_DST_COLOR, RL_ZERO, RL_FUNC_ADD);
        rlSetBlendMode(RL_BLEND_CUSTOM);
    }

    void end_invert() {
        rlDrawRenderBatchActive();
        rlSetBlendMode(RL_BLEND_ALPHA);
    }
}
