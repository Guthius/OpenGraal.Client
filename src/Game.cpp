#include "Game.h"
#include <raylib.h>
#include <rlgl.h>

#include "Animation.h"
#include "FileManager.h"
#include "LevelManager.h"

Game::Game() : _player(this)
{
	FileManager::BuildFileTable("levels");

	_level = new LevelInfo(
			LevelManager::Get("onlinestartlocal.graal"),
			TilesetManager::Get("pics1.png"),
			0, 0);
}

void Game::Run()
{
	InitAudioDevice();

	SetTargetFPS(60);

	Animation ani{};

	while (!WindowShouldClose())
	{
		Update();

		BeginDrawing();

		ClearBackground(BLACK);

		Draw();
		DrawDiagnostics();

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

void Game::Update()
{
	_player.Update(GetFrameTime());
}

void Game::Draw() const
{
	auto csx = static_cast<float>(GetScreenWidth()) / 2.0f;
	auto csy = static_cast<float>(GetScreenHeight()) / 2.0f;

	auto pos = _player.GetPosition();
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
	_player.Draw();
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