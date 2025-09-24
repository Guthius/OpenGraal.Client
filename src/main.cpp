#include "animation_manager.hpp"
#include "game.hpp"

constexpr auto Title = "OpenGraal";

int main()
{
	InitAudioDevice();

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	load_animations_from_directory("levels/ganis");

	InitWindow(1280, 720, Title);

	game game{};

	game.run();

	CloseWindow();

	return 0;
}
