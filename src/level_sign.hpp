#pragma once

#include <raylib.h>
#include <string>

class level_sign
{
public:
	level_sign(float x, float y, std::string text);

	[[nodiscard]] auto get_rectangle() const -> const Rectangle & { return rectangle_; }
	[[nodiscard]] auto get_text() const -> const std::string & { return text_; }

private:
	Rectangle rectangle_;
	std::string text_{};
};

auto decode_sign_text(const std::string &str) -> std::string;
