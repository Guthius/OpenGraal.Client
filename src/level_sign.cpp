#include "level_sign.hpp"

#include <sstream>
#include <utility>

namespace
{
	constexpr int sign_width = 32;
	constexpr int sign_height = 16;

	auto sign_characters =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
			"0123456789!?-.,#>()#####\"####':/~&### <####;\n";
}

level_sign::level_sign(const float x, const float y, std::string text)
	: rectangle_{x, y, sign_width, sign_height},
	  text_(std::move(text))
{
}

auto decode_sign_text(const std::string &str) -> std::string
{
	const auto max = TextLength(sign_characters);

	std::stringstream ss;

	for (const auto &c: str)
	{
		const auto index = static_cast<unsigned char>(c) - 32;
		if (index < 0 || index >= max)
		{
			continue;
		}

		ss << sign_characters[index];
	}

	return ss.str();
}
