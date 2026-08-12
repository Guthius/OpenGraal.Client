#include "gui_drawing_panel.hpp"

#include "../texture_manager.hpp"

namespace {
    auto grayscale_shader() -> Shader {
        static const auto shader = LoadShaderFromMemory(nullptr, R"(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;
void main()
{
    vec4 texel = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    finalColor = vec4(vec3(gray), texel.a);
}
)");

        return shader;
    }
}

void gui_drawing_panel::draw_image(std::string image, const Vector2 position) {
    commands_.push_back({
        .image = std::move(image), .dest = {position.x, position.y, 0, 0},
             .source = {}
    });
}

void gui_drawing_panel::draw_image_stretched(std::string image, const Rectangle dest, const Rectangle source) {
    commands_.push_back({.image = std::move(image), .dest = dest, .source = source});
}

void gui_drawing_panel::draw_image_rectangle(std::string image, const Vector2 position, const Rectangle source) {
    commands_.push_back({
        .image = std::move(image),
        .dest = {position.x, position.y, source.width, source.height},
        .source = source,
    });
}

void gui_drawing_panel::draw() const {
    const auto bounds = get_bounds();

    if (const auto profile = get_profile(); profile) {
        DrawRectangleRec(bounds, profile->fill_color);
        DrawRectangleLinesEx(bounds, static_cast<float>(profile->border_thickness), profile->border_color);
    }

    const auto clip = local_to_global_coord({0, 0});

    BeginScissorMode(
        static_cast<int>(clip.x), static_cast<int>(clip.y),
        static_cast<int>(bounds.width), static_cast<int>(bounds.height));

    const auto disabled = is_disabled();
    const auto tint = disabled ? Color{255, 255, 255, 112} : WHITE;

    if (disabled) {
        BeginShaderMode(grayscale_shader());
    }

    for (const auto &[image, dest, source] : commands_) {
        const auto texture = load_texture(image);
        if (!IsTextureValid(texture)) {
            continue;
        }

        const auto whole = Rectangle{0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)};
        const auto from = source.width > 0 && source.height > 0 ? source : whole;
        const auto to = Rectangle{
            bounds.x + dest.x,
            bounds.y + dest.y,
            dest.width > 0 ? dest.width : from.width,
            dest.height > 0 ? dest.height : from.height};

        DrawTexturePro(texture, from, to, {0, 0}, 0.0f, tint);
    }

    if (disabled) {
        EndShaderMode();
    }

    draw_children();

    EndScissorMode();
}
