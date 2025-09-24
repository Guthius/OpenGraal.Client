#include "utils.hpp"

#include <sstream>

auto split_string(const std::string &str) -> std::vector<std::string>
{
	std::vector<std::string> tokens;

	std::string buf;
	std::stringstream ss(str);

	while (std::getline(ss, buf, ' '))
	{
		if (buf.empty())
		{
			continue;
		}

		tokens.push_back(buf);
	}

	return tokens;
}