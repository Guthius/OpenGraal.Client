#include "LevelSign.h"

static constexpr int SignWidth = 32;
static constexpr int SignHeight = 16;

LevelSign::LevelSign(float x, float y, const std::string &text)
		: _rect{x, y, SignWidth, SignHeight}, _text(text)
{
}

std::string LevelSign::Decode(const std::string &str)
{
	return str;
}