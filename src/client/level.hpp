#pragma once

#include "carry_object.hpp"
#include "leap_effect.hpp"
#include "level_link.hpp"
#include "level_sign.hpp"
#include "tileset.hpp"

#include <array>
#include <filesystem>
#include <vector>

enum class tile_pattern_type {
    none,
    bush,
    swamp,
    vase,
    sign
};

struct tile_pattern_match {
    tile_pattern_type type;
    int x, y;
    std::array<short, 4> replacement;
    leap_effect_type leap_type;
};

class level {
  public:
    explicit level(const og::shared::level_data &data);

    void draw(const tileset *tileset) const;

    [[nodiscard]] auto get_name() const -> const std::string & { return name_; }
    [[nodiscard]] auto get_npcs() const -> const std::vector<og::shared::level_npc> & { return npcs_; }
    [[nodiscard]] auto get_chests() const -> const std::vector<og::shared::level_chest> & { return chests_; }
    [[nodiscard]] auto get_baddies() const -> const std::vector<og::shared::level_baddy> & { return baddies_; }

    [[nodiscard]] auto get_link_at(float x, float y) const -> const level_link *;
    [[nodiscard]] auto get_sign_at(float x, float y) const -> const level_sign *;
    [[nodiscard]] auto get_tile_type(const tileset *tileset, int x, int y) const -> int;
    [[nodiscard]] auto get_tile_id(int x, int y) const -> int;

    [[nodiscard]] auto has_tiles() const -> bool { return has_tiles_; }

    void set_board(const std::vector<uint16_t> &tiles);

    [[nodiscard]] auto on_wall(const tileset *tileset, Rectangle rect) const -> bool;
    [[nodiscard]] auto on_wall(const tileset *tileset, Vector2 pt) const -> bool;

    [[nodiscard]] auto find_tile_pattern_at(float x, float y) const -> tile_pattern_match;

    auto try_destroy_object_at(float x, float y) -> std::tuple<leap_effect_type, int, int>;
    auto try_lift_object_at(float x, float y) -> carry_object_type;

    static auto load(const std::filesystem::path &path) -> std::shared_ptr<level>;
    static auto load(const std::string &name, const std::string &source) -> std::shared_ptr<level>;

  private:
    std::string name_;
    std::vector<short> board_;
    bool has_tiles_ = false;
    std::vector<level_link> links_;
    std::vector<level_sign> signs_;
    std::vector<og::shared::level_npc> npcs_;
    std::vector<og::shared::level_chest> chests_;
    std::vector<og::shared::level_baddy> baddies_;
};
