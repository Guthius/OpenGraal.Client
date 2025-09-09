#include "Game.h"

#include <raylib.h>
#include <rlgl.h>

#include "Animation.h"
#include "FileManager.h"
#include "LevelManager.h"
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
	font20_ = LoadFontEx("levels/pixantiqua.ttf", 20, nullptr, 250);
	font14_ = LoadFontEx("levels/pixantiqua.ttf", 16, nullptr, 250);

	sign_ = new Sign();
}

void Game::Run() const
{
	Animation ani{};

	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BLACK);

		Draw();
		DrawUI();

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
	if (sign_->IsOpen())
	{
		sign_->Update();

		return;
	}

	const auto csx = static_cast<float>(GetScreenWidth()) / 2.0f;
	const auto csy = static_cast<float>(GetScreenHeight()) / 2.0f;

	const auto pos = player_->GetPosition();
	const auto cx = static_cast<int>(csx - 16 - pos.x);
	const auto cy = static_cast<int>(csy - 16 - pos.y);

	rlPushMatrix();
	rlTranslatef(
		static_cast<float>(cx),
		static_cast<float>(cy),
		0);


	player_->Update(GetFrameTime());

	rlPopMatrix();
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

	rlPopMatrix();
}

void Game::DrawPlayer() const
{
	player_->Draw();
}

void Game::DrawUI() const
{
	DrawTextureRec(state_, {202, 0, 22, 30}, {15, 30}, WHITE);
	DrawTextureRec(state_, {202, 0, 22, 30}, {80, 30}, WHITE);
	DrawTextureRec(state_, {202, 0, 22, 30}, {145, 30}, WHITE);

	DrawTextEx(font20_, "A", {15 + 4, 30 + 7}, font20_.baseSize, 0.0f, BLACK);
	DrawTextEx(font20_, "S", {80 + 5, 30 + 7}, font20_.baseSize, 0.0f, BLACK);
	DrawTextEx(font20_, "D", {145 + 5, 30 + 7}, font20_.baseSize, 0.0f, BLACK);
	DrawTextEx(font20_, "A", {15 + 3, 30 + 6}, font20_.baseSize, 0.0f, WHITE);
	DrawTextEx(font20_, "S", {80 + 4, 30 + 6}, font20_.baseSize, 0.0f, WHITE);
	DrawTextEx(font20_, "D", {145 + 4, 30 + 6}, font20_.baseSize, 0.0f, WHITE);

	/* Alignment */
	DrawTextureRec(state_, {0, 97, 130, 22}, {15, 65}, WHITE);
	DrawTextureRec(state_, {0, 119, 100, 10}, {37, 71}, WHITE);

	DrawUI_Resource({80, 33, 16, 16}, {274, 30}, "754");
	DrawUI_Resource({136, 33, 16, 16}, {274 + 56, 30}, "5");
	DrawUI_Resource({184, 33, 16, 16}, {274 + 104, 30}, "0");
}

void Game::DrawUI_Resource(const Rectangle rect, const Vector2 pos, const std::string &text) const
{
	DrawTextureRec(state_, rect, pos, WHITE);

	const auto textStr = text.c_str();
	const auto textSize = MeasureTextEx(font14_, textStr, font14_.baseSize, 1);

	const auto tx = pos.x + 8 - textSize.x / 2;
	const auto ty = pos.y + rect.height;

	DrawTextEx(font14_, textStr, {tx + 2, ty + 2}, font14_.baseSize, 1, BLACK);
	DrawTextEx(font14_, textStr, {tx, ty}, font14_.baseSize, 1, WHITE);
}

static void DrawDiagnosticsText(const char *str, const int y)
{
	DrawText(str, 11, y + 1, 20, BLACK);
	DrawText(str, 10, y, 20, WHITE);
}

void Game::DrawDiagnostics()
{
	DrawDiagnosticsText(TextFormat("FPS: %d", GetFPS()), 10);
}
