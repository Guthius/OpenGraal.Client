#include "LevelSign.h"
#include <sstream>

static constexpr int SignWidth = 32;
static constexpr int SignHeight = 16;

const char * Text=
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
		"0123456789!?-.,#>()#####\"####':/~&### <####;\n";

LevelSign::LevelSign(float x, float y, const std::string &text)
		: _rect{x, y, SignWidth, SignHeight}, _text(text)
{
}

std::string LevelSign::Decode(const std::string &str)
{
	auto max = TextLength(Text);

	std::stringstream ss;

	for (const auto &c : str)
	{
		auto index = ((unsigned char)c - 32);
		if (index < 0 || index >= max)
		{
			continue;
		}

		ss << Text[index];
	}

	return ss.str();
}