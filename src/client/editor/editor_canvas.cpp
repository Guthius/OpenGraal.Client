#include "editor_canvas.hpp"

#include "../constants.hpp"
#include "../dev_input.hpp"
#include "../texture_manager.hpp"
#include "../tileset_manager.hpp"
#include "editor_profiles.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <raylib.h>
#include <rlgl.h>
#include <vector>

namespace {
    constexpr std::array zoom_steps{0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};

    constexpr Color link_color{240, 220, 40, 255};
    constexpr Color grid_color{255, 255, 255, 40};

    auto npc_texture(const og::shared::level_npc &npc) -> Texture2D {
        if (!npc.image.empty()) {
            if (const auto texture = load_texture(npc.image); IsTextureValid(texture)) {
                return texture;
            }
        }

        return load_texture("editor_npc.png");
    }
}

auto canvas_view::zoom_step(const int direction, const Vector2 anchor_screen) -> void {
    auto index = 0;
    auto best = std::numeric_limits<float>::max();

    for (size_t i = 0; i < zoom_steps.size(); ++i) {
        if (const auto distance = std::fabs(zoom_steps[i] - zoom); distance < best) {
            best = distance;
            index = static_cast<int>(i);
        }
    }

    const auto next = std::clamp(index + direction, 0, static_cast<int>(zoom_steps.size()) - 1);
    if (zoom_steps[next] == zoom) {
        return;
    }

    const auto anchor_level = screen_to_level(anchor_screen);

    zoom = zoom_steps[next];
    pan_x = anchor_level.x - (anchor_screen.x / zoom);
    pan_y = anchor_level.y - (anchor_screen.y / zoom);
}

auto editor_canvas::level_tile_at(const Vector2 pt) const -> Vector2 {
    const auto level = tab_->view.screen_to_level(pt);

    return {level.x / tile_size, level.y / tile_size};
}

void editor_canvas::update(const float dt) {
    gui_control::update(dt);

    if (tab_ == nullptr || !is_visible()) {
        return;
    }

    const auto mouse = dev_input::mouse_position();
    const auto local = global_to_local_coord(mouse.x, mouse.y);

    if (is_mouse_over()) {
        if (const auto wheel = dev_input::mouse_wheel(); wheel != 0.0f) {
            tab_->view.zoom_step(wheel > 0.0f ? 1 : -1, local);

            if (view_changed) {
                view_changed();
            }
        }

        const auto tile = level_tile_at(local);
        const auto tile_x = static_cast<int>(std::floor(tile.x));
        const auto tile_y = static_cast<int>(std::floor(tile.y));

        hover_tile_ = tile;

        if (hovered && tile_x >= 0 && tile_x < og::shared::level_width && tile_y >= 0 && tile_y < og::shared::level_height) {
            const auto *tiles = tab_->level.tiles();

            hovered(tile_x, tile_y, tiles != nullptr ? (*tiles)[(tile_y * og::shared::level_width) + tile_x] : 0);
        }
    }

    const auto over_npc = is_mouse_over() && mode_ == editor_mode::none && npc_at(hover_tile_) >= 0;
    if (over_npc != move_cursor_) {
        move_cursor_ = over_npc;

        SetMouseCursor(over_npc ? MOUSE_CURSOR_RESIZE_ALL : MOUSE_CURSOR_DEFAULT);
    }
}

auto editor_canvas::npc_at(const Vector2 level_tile) const -> int {
    if (tab_ == nullptr || hide_npcs_) {
        return -1;
    }

    for (auto i = static_cast<int>(tab_->level.npcs.size()) - 1; i >= 0; --i) {
        const auto &npc = tab_->level.npcs[static_cast<size_t>(i)];
        const auto texture = npc_texture(npc);
        const auto width = IsTextureValid(texture) ? static_cast<float>(texture.width) / tile_size : 1.0f;
        const auto height = IsTextureValid(texture) ? static_cast<float>(texture.height) / tile_size : 1.0f;

        if (level_tile.x >= npc.x && level_tile.x < npc.x + width &&
            level_tile.y >= npc.y && level_tile.y < npc.y + height) {
            return i;
        }
    }

    return -1;
}

void editor_canvas::on_mouse_pressed(const int button, const Vector2 pt) {
    if (tab_ == nullptr) {
        return;
    }

    if (button == MOUSE_BUTTON_MIDDLE) {
        panning_ = true;
        pan_start_ = pt;
        pan_origin_ = {tab_->view.pan_x, tab_->view.pan_y};

        capture_mouse();

        return;
    }

    capture_mouse();

    if (canvas_pressed) {
        canvas_pressed(button, level_tile_at(pt));
    }
}

void editor_canvas::on_mouse_move(const Vector2 pt) {
    if (tab_ == nullptr) {
        return;
    }

    if (panning_) {
        tab_->view.pan_x = pan_origin_.x + ((pan_start_.x - pt.x) / tab_->view.zoom);
        tab_->view.pan_y = pan_origin_.y + ((pan_start_.y - pt.y) / tab_->view.zoom);

        if (view_changed) {
            view_changed();
        }

        return;
    }

    if (canvas_moved) {
        canvas_moved(level_tile_at(pt));
    }
}

void editor_canvas::on_mouse_released(const int button, const Vector2 pt) {
    release_mouse();

    if (panning_ && button == MOUSE_BUTTON_MIDDLE) {
        panning_ = false;

        return;
    }

    if (tab_ != nullptr && canvas_released) {
        canvas_released(button, level_tile_at(pt));
    }
}

void editor_canvas::draw() const {
    const auto bounds = get_bounds();
    const auto profile = get_profile();

    if (profile) {
        profile->draw_frame(bounds, false, false);
    }

    if (tab_ == nullptr) {
        draw_children();

        return;
    }

    const auto clip = local_to_global_coord({0, 0});
    BeginScissorMode(
        static_cast<int>(clip.x), static_cast<int>(clip.y),
        std::max(0, static_cast<int>(bounds.width)), std::max(0, static_cast<int>(bounds.height)));

    const auto &view = tab_->view;
    const auto scale = tile_size * view.zoom;
    const auto origin = view.level_to_screen({0, 0});

    const auto first_x = std::max(0, static_cast<int>(view.pan_x / tile_size));
    const auto first_y = std::max(0, static_cast<int>(view.pan_y / tile_size));
    const auto last_x = std::min(og::shared::level_width - 1, static_cast<int>((view.pan_x + (bounds.width / view.zoom)) / tile_size) + 1);
    const auto last_y = std::min(og::shared::level_height - 1, static_cast<int>((view.pan_y + (bounds.height / view.zoom)) / tile_size) + 1);

    auto indices = std::vector<int>{};
    for (const auto &layer : tab_->level.layers) {
        indices.push_back(layer.index);
    }
    std::ranges::sort(indices);

    const auto *tiles_texture = tileset_for_level(tab_->level.name);

    if (tiles_texture != nullptr) {
        const auto tile_width = tiles_texture->get_tile_width();
        const auto tile_height = tiles_texture->get_tile_height();

        rlSetTexture(tiles_texture->get_texture().id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);

        for (const auto index : indices) {
            const auto &tiles = tab_->level.find_layer(index)->tiles;

            for (int yy = first_y; yy <= last_y; ++yy) {
                for (int xx = first_x; xx <= last_x; ++xx) {
                    const auto tile = tiles[(yy * og::shared::level_width) + xx];
                    if (index != 0 && tile == 0) {
                        continue;
                    }

                    const auto sx1 = bounds.x + origin.x + (static_cast<float>(xx) * scale);
                    const auto sy1 = bounds.y + origin.y + (static_cast<float>(yy) * scale);
                    const auto sx2 = sx1 + scale;
                    const auto sy2 = sy1 + scale;

                    const auto tilex = (((tile / 512) * 16) + (tile % 16));
                    const auto tiley = (tile % 512) / 16;

                    const auto tx1 = static_cast<float>(tilex) * tile_width;
                    const auto tx2 = tx1 + tile_width;
                    const auto ty1 = static_cast<float>(tiley) * tile_height;
                    const auto ty2 = ty1 + tile_height;

                    rlTexCoord2f(tx1, ty1);
                    rlVertex2f(sx1, sy1);
                    rlTexCoord2f(tx1, ty2);
                    rlVertex2f(sx1, sy2);
                    rlTexCoord2f(tx2, ty2);
                    rlVertex2f(sx2, sy2);
                    rlTexCoord2f(tx2, ty1);
                    rlVertex2f(sx2, sy1);
                }
            }
        }

        rlEnd();
        rlSetTexture(0);
    }

    const auto tile_rect = [&](const float x, const float y, const float w, const float h) {
        const auto top_left = view.level_to_screen({x * tile_size, y * tile_size});

        return Rectangle{bounds.x + top_left.x, bounds.y + top_left.y, w * scale, h * scale};
    };

    if (show_grid_) {
        const auto level_left = bounds.x + origin.x;
        const auto level_top = bounds.y + origin.y;
        const auto level_right = level_left + (static_cast<float>(og::shared::level_width) * scale);
        const auto level_bottom = level_top + (static_cast<float>(og::shared::level_height) * scale);

        for (int xx = first_x; xx <= last_x + 1; ++xx) {
            const auto x = level_left + (static_cast<float>(xx) * scale);

            DrawLineV({x, level_top}, {x, level_bottom}, grid_color);
        }

        for (int yy = first_y; yy <= last_y + 1; ++yy) {
            const auto y = level_top + (static_cast<float>(yy) * scale);

            DrawLineV({level_left, y}, {level_right, y}, grid_color);
        }
    }

    for (const auto &link : tab_->level.links) {
        DrawRectangleLinesEx(
            tile_rect(static_cast<float>(link.x), static_cast<float>(link.y),
                static_cast<float>(link.width), static_cast<float>(link.height)),
            2, link_color);
    }

    if (!hide_npcs_) {
        for (size_t i = 0; i < tab_->level.npcs.size(); ++i) {
            const auto &npc = tab_->level.npcs[i];
            const auto texture = npc_texture(npc);
            const auto anchor = view.level_to_screen({npc.x * tile_size, npc.y * tile_size});
            const auto position = Vector2{bounds.x + anchor.x, bounds.y + anchor.y};

            if (IsTextureValid(texture)) {
                DrawTextureEx(texture, position, 0.0f, view.zoom, WHITE);
            }

            if (static_cast<int>(i) == tab_->selected_npc) {
                const auto width = IsTextureValid(texture) ? static_cast<float>(texture.width) : tile_size;
                const auto height = IsTextureValid(texture) ? static_cast<float>(texture.height) : tile_size;

                DrawRectangleLinesEx(
                    {position.x - 2, position.y - 2, (width * view.zoom) + 4, (height * view.zoom) + 4},
                    2, WHITE);
            }
        }
    }

    const auto draw_block = [&](const tile_clipboard &block, const int at_x, const int at_y) {
        if (tiles_texture == nullptr) {
            return;
        }

        const auto texture = tiles_texture->get_texture();

        for (int row = 0; row < block.height; ++row) {
            for (int column = 0; column < block.width; ++column) {
                const auto tile = block.tiles[(row * block.width) + column];
                const int atlas_x = (((tile / 512) * 16) + (tile % 16)) * tile_size;
                const int atlas_y = ((tile % 512) / 16) * tile_size;
                const auto source = Rectangle{
                    static_cast<float>(atlas_x),
                    static_cast<float>(atlas_y),
                    tile_size,
                    tile_size,
                };

                DrawTexturePro(texture, source,
                    tile_rect(static_cast<float>(at_x + column), static_cast<float>(at_y + row), 1, 1),
                    {0, 0}, 0.0f, WHITE);
            }
        }
    };

    if (floating_ != nullptr) {
        draw_block(*floating_, floating_x_, floating_y_);
    }

    if (is_mouse_over()) {
        const auto hover_x = static_cast<int>(std::floor(hover_tile_.x));
        const auto hover_y = static_cast<int>(std::floor(hover_tile_.y));

        if (mode_ == editor_mode::tile_placement && brush_ != nullptr && floating_ == nullptr) {
            draw_block(*brush_, hover_x, hover_y);
        }

        if (mode_ == editor_mode::npc_placement) {
            if (const auto icon = load_texture("editor_npc.png"); IsTextureValid(icon)) {
                const auto anchor = view.level_to_screen({static_cast<float>(hover_x) * tile_size, static_cast<float>(hover_y) * tile_size});

                DrawTextureEx(icon, {bounds.x + anchor.x, bounds.y + anchor.y}, 0.0f, view.zoom, WHITE);
            }
        }
    }

    if (tab_->selection.active) {
        const auto &selection = tab_->selection;

        editor_profiles::begin_invert();

        if (selection.width == 0 || selection.height == 0) {
            const auto centre = view.level_to_screen({
                static_cast<float>(selection.x) * tile_size,
                static_cast<float>(selection.y) * tile_size,
            });
            const auto x = bounds.x + centre.x;
            const auto y = bounds.y + centre.y;

            DrawLineEx({x - 8, y}, {x + 8, y}, 2, WHITE);
            DrawLineEx({x, y - 8}, {x, y + 8}, 2, WHITE);
        } else {
            DrawRectangleLinesEx(
                tile_rect(static_cast<float>(selection.x), static_cast<float>(selection.y),
                    static_cast<float>(selection.width), static_cast<float>(selection.height)),
                2, WHITE);
        }

        editor_profiles::end_invert();
    }

    EndScissorMode();

    draw_children();
}
