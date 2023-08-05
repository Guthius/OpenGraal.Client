#include "Utils.h"

#include <sstream>

std::vector<std::string> Split(const std::string &str)
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