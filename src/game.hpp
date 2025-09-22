#pragma once

#include "actor.hpp"
#include "level.hpp"
#include "player.hpp"
#include "sign.hpp"
#include "tileset.hpp"
#include "thrown_item.hpp"

#include <vector>

#include "leaps.hpp"

class game
{
	struct LevelInfo
	{
		LevelInfo(level *level, tileset *tileset, const int x, const int y)
		{
			Level = level;
			Tileset = tileset;
			X = x;
			Y = y;
		}

		level *Level;
		tileset *Tileset;
		int X, Y;
	};

public:
	game();

	void Run() const;

	void ChangeLevel(const std::string &level_name) const;
	auto GetCurrentLevel() const -> level * { return level_->Level; }
	auto OnWall(Rectangle rect) const -> bool;
	auto OnWall(Vector2 pt) const -> bool;
	auto GetTileType(int x, int y) const -> int;
	void ShowSign(const std::string &str) const;
	void SpawnThrownItem(actor::CarriedItem type, Vector2 origin, Direction dir) const;
	void SpawnLeaps(LeapType type, Vector2 origin) const;

private:
	void Update() const;
	void UpdateThrownItems(float dt) const;
	void UpdateLeaps(float dt) const;

	void Draw() const;
	void DrawPlayer() const;
	void DrawHud() const;
	void DrawHudResource(Rectangle rect, Vector2 pos, const std::string &text) const;
	void DrawDiagnostics() const;

	void DrawThrownItems() const;
	void DrawLeaps() const;


	sign *sign_;
	LevelInfo *level_;
	player *player_;
	Texture2D state_{};
	Font font20_{};
	Font font14_{};
	Font font_pixel_{};
	mutable std::vector<thrown_item> thrown_items_{};
	mutable std::vector<Leap> leaps_{};
	mutable Texture2D sprites_{};
};
