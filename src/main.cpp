#include "animation_manager.hpp"
#include "game.hpp"

constexpr auto Title = "OpenGraal";

int main()
{
	InitAudioDevice();

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	load_animations_from_directory("levels/ganis");

	InitWindow(1280, 720, Title);

	run_game();

	CloseWindow();

	return 0;
}
