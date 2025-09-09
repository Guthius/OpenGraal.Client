#include "LevelSign.h"

#include <sstream>

static constexpr int SignWidth = 32;
static constexpr int SignHeight = 16;

auto Text =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
		"0123456789!?-.,#>()#####\"####':/~&### <####;\n";

LevelSign::LevelSign(float x, float y, const std::string &text)
	: rect_{x, y, SignWidth, SignHeight}, text_(text)
{
}

auto LevelSign::Decode(const std::string &str) -> std::string
{
	const auto max = TextLength(Text);

	std::stringstream ss;

	for (const auto &c: str)
	{
		const auto index = static_cast<unsigned char>(c) - 32;
		if (index < 0 || index >= max)
		{
			continue;
		}

		ss << Text[index];
	}

	return ss.str();
}
