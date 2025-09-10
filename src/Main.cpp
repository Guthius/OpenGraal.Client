#include "AnimationManager.h"
#include "Game.h"

constexpr auto Title = "OpenGraal";

int main()
{
	InitAudioDevice();

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	AnimationManager::LoadFrom("levels/ganis");

	InitWindow(640, 480, Title);

	auto game = Game();

	game.Run();

	CloseWindow();

	return 0;
}
