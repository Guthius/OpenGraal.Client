#pragma once

#include <string>
#include <raylib.h>

class LevelSign
{
public:
	LevelSign(float x, float y, const std::string &text);

public:
	[[nodiscard]] const Rectangle &GetRectangle() const { return _rect; }
	[[nodiscard]] const std::string &GetText() const { return _text; }

public:
	static std::string Decode(const std::string &str);

private:
	Rectangle _rect;
	std::string _text{};
};