#include "Game.h"
#include <raylib.h>
#include <rlgl.h>

#include "Animation.h"
#include "FileManager.h"
#include "LevelManager.h"
#include "TilesetManager.h"

Game::Game()
{
	FileManager::BuildFileTable("levels");

	Vector2 pos{512, 512};

	_player = new Player(this);
	_player->SetPosition(pos);

	_level = new LevelInfo(
			LevelManager::Get("onlinestartlocal.graal"),
			TilesetManager::Get("pics1.png"),
			0, 0);

	_state = TextureManager::Get("state.png");
	_font20 = LoadFontEx("levels/pixantiqua.ttf", 20, 0, 250);
	_font14 = LoadFontEx("levels/pixantiqua.ttf", 16, 0, 250);
}

void Game::Run()
{
	SetTargetFPS(60);

	Animation ani{};

	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BLACK);

		Draw();
		DrawUI();
		DrawDiagnostics();

		Update();

		EndDrawing();
	}
}

void Game::ChangeLevel(const std::string &levelName)
{
	auto level = LevelManager::Get(levelName);

	if (level == nullptr)
	{
		return;
	}

	_level->Level = level;
}

bool Game::OnWall(Rectangle rect) const
{
	if (_level->Level == nullptr)
	{
		return true;
	}

	return _level->Level->OnWall(_level->Tileset, rect);
}

TileType Game::GetTileType(int x, int y) const
{
	if (_level->Level == nullptr)
	{
		return TileType::Passable;
	}

	return _level->Level->GetTileType(_level->Tileset, x, y);
}

void Game::Update()
{
	auto csx = static_cast<float>(GetScreenWidth()) / 2.0f;
	auto csy = static_cast<float>(GetScreenHeight()) / 2.0f;

	auto pos = _player->GetPosition();
	auto cx = static_cast<int>(csx - 16 - pos.x);
	auto cy = static_cast<int>(csy - 16 - pos.y);

	rlPushMatrix();
	rlTranslatef(
			static_cast<float>(cx),
			static_cast<float>(cy),
			0);


	_player->Update(GetFrameTime());

	rlPopMatrix();
}

void Game::Draw() const
{
	auto csx = static_cast<float>(GetScreenWidth()) / 2.0f;
	auto csy = static_cast<float>(GetScreenHeight()) / 2.0f;

	auto pos = _player->GetPosition();
	auto cx = static_cast<int>(csx - 16 - pos.x);
	auto cy = static_cast<int>(csy - 16 - pos.y);

	rlSetTexture(_level->Tileset->GetTexture().id);

	rlPushMatrix();
	rlTranslatef(
			static_cast<float>(cx),
			static_cast<float>(cy),
			0);

	_level->Level->Draw(_level->Tileset);

	rlSetTexture(0);

	DrawPlayer();

	rlPopMatrix();
}

void Game::DrawPlayer() const
{
	_player->Draw();
}

void Game::DrawUI() const
{
	DrawTextureRec(_state, {202, 0, 22, 30}, {15, 30}, WHITE);
	DrawTextureRec(_state, {202, 0, 22, 30}, {80, 30}, WHITE);
	DrawTextureRec(_state, {202, 0, 22, 30}, {145, 30}, WHITE);

	DrawTextEx(_font20, "A", {15+4, 30+7}, _font20.baseSize, 0.0f, BLACK);
	DrawTextEx(_font20, "S", {80+5, 30+7}, _font20.baseSize, 0.0f, BLACK);
	DrawTextEx(_font20, "D", {145+5, 30+7}, _font20.baseSize, 0.0f, BLACK);
	DrawTextEx(_font20, "A", {15+3, 30+6}, _font20.baseSize, 0.0f, WHITE);
	DrawTextEx(_font20, "S", {80+4, 30+6}, _font20.baseSize, 0.0f, WHITE);
	DrawTextEx(_font20, "D", {145+4, 30+6}, _font20.baseSize, 0.0f, WHITE);


	/* Alignment */
	DrawTextureRec(_state, {0, 97, 130, 22}, {15, 65}, WHITE);
	DrawTextureRec(_state, {0, 119, 100, 10}, {37, 71}, WHITE);


	DrawUI_Resource({80, 33, 16, 16}, {274, 30}, "754");
	DrawUI_Resource({136, 33, 16, 16}, {274+56, 30}, "5");
	DrawUI_Resource({184, 33, 16, 16}, {274+104, 30}, "0");
}

void Game::DrawUI_Resource(Rectangle rect, Vector2 pos, const std::string& text) const
{
	DrawTextureRec(_state, rect, pos, WHITE);

	auto textStr = text.c_str();
	auto textSize = MeasureTextEx(_font14, textStr, _font14.baseSize, 1);

	auto tx = pos.x + 8 - (textSize.x / 2);
	auto ty = pos.y + rect.height;

	DrawTextEx(_font14, textStr, {tx + 2, ty + 2}, _font14.baseSize, 1, BLACK);
	DrawTextEx(_font14, textStr, {tx, ty}, _font14.baseSize, 1, WHITE);
}

static void DrawDiagnosticsText(const char *str, int y)
{
	DrawText(str, 11, y + 1, 20, BLACK);
	DrawText(str, 10, y, 20, WHITE);
}

void Game::DrawDiagnostics()
{
	DrawDiagnosticsText(TextFormat("FPS: %d", GetFPS()), 10);
}