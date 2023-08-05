#include "Game.h"
#include "AnimationManager.h"

constexpr const char* Title = "OpenGraal";

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	AnimationManager::LoadFrom("levels/ganis");

	InitWindow(640, 480, Title);

	auto game = Game();

	game.Run();

	CloseWindow();

	return 0;
}
