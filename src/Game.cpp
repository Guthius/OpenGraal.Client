#include "Game.h"

#include <raylib.h>
#include <rlgl.h>

#include "Animation.h"
#include "FileManager.h"
#include "LevelManager.h"
#include "SoundManager.h"
#include "TextureManager.h"
#include "TilesetManager.h"

#define SIGN_WIDTH 382
#define SIGN_HEIGHT 142

Game::Game()
{
	FileManager::BuildFileTable("levels");

	constexpr Vector2 pos{512, 512};

	player_ = new Player(this);
	player_->SetPosition(pos);

	level_ = new LevelInfo(
		LevelManager::Get("onlinestartlocal.graal"),
		TilesetManager::Get("pics1.png"),
		0, 0);

	state_ = TextureManager::Get("state.png");
	font20_ = LoadFontEx("Fonts/LiberationSans-Bold.ttf", 24, nullptr, 250);
	font14_ = LoadFontEx("Fonts/LiberationSans-Regular.ttf", 18, nullptr, 250);

	font_pixel_ = LoadFontEx("Fonts/Kenney Pixel.ttf", 12, nullptr, 0);

	sign_ = new Sign();

	sprites_ = TextureManager::Get("sprites.png");
}

void Game::Run() const
{
	Animation ani{};

	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BLACK);

		Draw();
		DrawHud();

		sign_->Draw(SIGN_WIDTH, SIGN_HEIGHT);

		DrawDiagnostics();

		Update();

		EndDrawing();
	}
}

void Game::ChangeLevel(const std::string &level_name) const
{
	const auto level = LevelManager::Get(level_name);

	if (level == nullptr)
	{
		return;
	}

	level_->Level = level;
}

auto Game::OnWall(const Rectangle rect) const -> bool
{
	if (level_->Level == nullptr)
	{
		return true;
	}

	return level_->Level->OnWall(level_->Tileset, rect);
}

auto Game::OnWall(const Vector2 pt) const -> bool
{
	if (level_->Level == nullptr)
	{
		return true;
	}

	return level_->Level->OnWall(level_->Tileset, pt);
}

auto Game::GetTileType(const int x, const int y) const -> int
{
	if (level_->Level == nullptr)
	{
		return TileType::Passable;
	}

	return level_->Level->GetTileType(level_->Tileset, x, y);
}

void Game::ShowSign(const std::string &str) const
{
	sign_->Show(str);
}

void Game::Update() const
{
	const auto dt = GetFrameTime();

	UpdateThrownItems(dt);
	UpdateLeaps(dt);

	if (sign_->IsOpen())
	{
		sign_->Update();

		return;
	}

	// Handle left mouse click: print tile ID at clicked position
	if (level_ != nullptr && level_->Level != nullptr && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		// Compute camera translation (same as in Draw)
		const auto csx = static_cast<float>(GetScreenWidth()) / 2.0f;
		const auto csy = static_cast<float>(GetScreenHeight()) / 2.0f;
		const auto pos = player_->GetPosition();
		const auto cx = static_cast<int>(csx - 16 - pos.x);
		const auto cy = static_cast<int>(csy - 16 - pos.y);

		const auto mouse = GetMousePosition();
		// Convert screen coordinates to world coordinates by reversing the translation
		const int worldX = static_cast<int>(mouse.x - static_cast<float>(cx));
		const int worldY = static_cast<int>(mouse.y - static_cast<float>(cy));

		const int tx = worldX / 16;
		const int ty = worldY / 16;

		const int tileId = level_->Level->GetTileId(worldX, worldY);
		TraceLog(LOG_INFO, "Clicked tile (%d,%d) world(%d,%d) id=%d", tx, ty, worldX, worldY, tileId);
	}

	player_->Update(GetFrameTime());
}

void Game::UpdateThrownItems(const float dt) const
{
	for (auto &item: thrown_items_)
	{
		item.Update(dt);

		if (!item.IsAlive())
		{
			SpawnLeaps(LeapType::Leaves, item.GetPosition());
		}
	}

	std::erase_if(thrown_items_, [](const ThrownItem &item) {
		return !item.IsAlive();
	});
}

void Game::UpdateLeaps(const float dt) const
{
	for (auto &leap: leaps_)
	{
		leap.Update(dt);
	}

	std::erase_if(leaps_, [](const Leap &item) {
		return !item.IsAlive();
	});
}

void Game::Draw() const
{
	const auto csx = static_cast<float>(GetScreenWidth()) / 2.0f;
	const auto csy = static_cast<float>(GetScreenHeight()) / 2.0f;

	const auto pos = player_->GetPosition();
	const auto cx = static_cast<int>(csx - 16 - pos.x);
	const auto cy = static_cast<int>(csy - 16 - pos.y);

	rlSetTexture(level_->Tileset->GetTexture().id);

	rlPushMatrix();
	rlTranslatef(
		static_cast<float>(cx),
		static_cast<float>(cy),
		0);

	level_->Level->Draw(level_->Tileset);

	rlSetTexture(0);

	DrawPlayer();

	DrawThrownItems();
	DrawLeaps();

	rlPopMatrix();
}

void Game::DrawPlayer() const
{
	player_->Draw();
}

void DrawHudKey(const Texture &texture, const Font &font, const Vector2 position, const char *key)
{
	DrawTextureRec(texture, {202, 0, 22, 30}, position, WHITE);

	DrawTextEx(font, key, {position.x + 3, position.y + 4}, 24, 0.0f, BLACK);
	DrawTextEx(font, key, {position.x + 2, position.y + 3}, 24, 0.0f, WHITE);
}

void Game::SpawnThrownItem(const Actor::CarriedItem type, const Vector2 origin, const Direction dir) const
{
	if (type == Actor::CarriedItem::None)
	{
		return;
	}

	thrown_items_.emplace_back(type, origin, dir);
}

void Game::SpawnLeaps(LeapType type, Vector2 origin) const
{
	if (const auto sound = SoundManager::Get("crush.wav"); IsSoundValid(sound))
	{
		PlaySound(sound);
	}

	leaps_.emplace_back(type, origin);
}

void Game::DrawHud() const
{
	/* Buttons */
	DrawHudKey(state_, font20_, {15, 30}, "A");
	DrawHudKey(state_, font20_, {80, 30}, "S");
	DrawHudKey(state_, font20_, {145, 30}, "D");

	/* Alignment */
	DrawTextureRec(state_, {0, 97, 130, 22}, {15, 65}, WHITE);
	DrawTextureRec(state_, {0, 119, 100, 10}, {37, 71}, WHITE);

	DrawHudResource({80, 33, 16, 16}, {274, 30}, "754");
	DrawHudResource({136, 33, 16, 16}, {274 + 56, 30}, "5");
	DrawHudResource({184, 33, 16, 16}, {274 + 104, 30}, "0");
}

void Game::DrawHudResource(const Rectangle rect, const Vector2 pos, const std::string &text) const
{
	DrawTextureRec(state_, rect, pos, WHITE);

	const auto textStr = text.c_str();
	const auto textSize = MeasureTextEx(font14_, textStr, font14_.baseSize, 1);

	const auto tx = pos.x + 8 - textSize.x / 2;
	const auto ty = pos.y + rect.height;

	DrawTextEx(font14_, textStr, {tx + 2, ty + 2}, font14_.baseSize, 1, BLACK);
	DrawTextEx(font14_, textStr, {tx, ty}, font14_.baseSize, 1, WHITE);
}

void Game::DrawDiagnostics() const
{
	const auto str = TextFormat("FPS: %d", GetFPS());

	DrawTextEx(font_pixel_, str, {6, 6}, 12, 1, BLACK);
	DrawTextEx(font_pixel_, str, {5, 5}, 12, 1, WHITE);
}

void Game::DrawThrownItems() const
{
	for (const auto &item: thrown_items_)
	{
		item.Draw();
	}
}

void Game::DrawLeaps() const
{
	for (const auto &leap: leaps_)
	{
		leap.Draw();
	}
}
