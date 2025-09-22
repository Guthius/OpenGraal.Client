#include "animation_manager.hpp"
#include "game.hpp"

constexpr auto Title = "OpenGraal";

int main()
{
	InitAudioDevice();

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	animation_manager::LoadFrom("levels/ganis");

	InitWindow(1280, 720, Title);

	game game{};

	game.Run();

	CloseWindow();

	return 0;
}
