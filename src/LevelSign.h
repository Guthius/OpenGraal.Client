#pragma once

#include <raylib.h>
#include <string>

class LevelSign
{
public:
	LevelSign(float x, float y, const std::string &text);

	[[nodiscard]] auto GetRectangle() const -> const Rectangle & { return rect_; }
	[[nodiscard]] auto GetText() const -> const std::string & { return text_; }

	static auto Decode(const std::string &str) -> std::string;

private:
	Rectangle rect_;
	std::string text_{};
};
