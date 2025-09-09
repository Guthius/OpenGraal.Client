#include "Sign.h"

#include <rlgl.h>
#include <sstream>

#include "SoundManager.h"
#include "TextureManager.h"

#define SIGN_BACKGROUND CLITERAL(Color){ 255, 247, 204, 255 }
#define SIGN_VERTICAL_OFFSET 80

Sign::Letter Sign::Glyphs[96] =
{
	{'A', 11, {0, 0, 16, 32}},
	{'B', 11, {16, 0, 16, 32}},
	{'C', 11, {32, 0, 16, 32}},
	{'D', 11, {48, 0, 16, 32}},
	{'E', 11, {64, 0, 16, 32}},
	{'F', 11, {80, 0, 16, 32}},
	{'G', 11, {96, 0, 16, 32}},
	{'H', 11, {112, 0, 16, 32}},
	{'I', 6, {128, 0, 16, 32}},
	{'J', 11, {144, 0, 16, 32}},
	{'K', 11, {160, 0, 16, 32}},
	{'L', 11, {176, 0, 16, 32}},
	{'M', 13, {192, 0, 16, 32}},
	{'N', 11, {208, 0, 16, 32}},
	{'O', 11, {224, 0, 16, 32}},
	{'P', 11, {240, 0, 16, 32}},
	{'Q', 11, {0, 32, 16, 32}},
	{'R', 11, {16, 32, 16, 32}},
	{'S', 11, {32, 32, 16, 32}},
	{'T', 13, {48, 32, 16, 32}},
	{'U', 11, {64, 32, 16, 32}},
	{'V', 13, {80, 32, 16, 32}},
	{'W', 13, {96, 32, 16, 32}},
	{'X', 13, {112, 32, 16, 32}},
	{'Y', 13, {128, 32, 16, 32}},
	{'Z', 11, {144, 32, 16, 32}},
	{'a', 11, {160, 32, 16, 32}},
	{'b', 11, {176, 32, 16, 32}},
	{'c', 11, {192, 32, 16, 32}},
	{'d', 11, {208, 32, 16, 32}},
	{'e', 11, {224, 32, 16, 32}},
	{'f', 11, {240, 32, 16, 32}},
	{'g', 11, {0, 64, 16, 32}},
	{'h', 11, {16, 64, 16, 32}},
	{'i', 5, {32, 64, 16, 32}},
	{'j', 9, {48, 64, 16, 32}},
	{'k', 11, {64, 64, 16, 32}},
	{'l', 5, {80, 64, 16, 32}},
	{'m', 13, {96, 64, 16, 32}},
	{'n', 11, {112, 64, 16, 32}},
	{'o', 11, {128, 64, 16, 32}},
	{'p', 11, {144, 64, 16, 32}},
	{'q', 11, {160, 64, 16, 32}},
	{'r', 9, {176, 64, 16, 32}},
	{'s', 11, {192, 64, 16, 32}},
	{'t', 11, {208, 64, 16, 32}},
	{'u', 11, {224, 64, 16, 32}},
	{'v', 13, {240, 64, 16, 32}},
	{'w', 13, {0, 96, 16, 32}},
	{'x', 13, {16, 96, 16, 32}},
	{'y', 13, {32, 96, 16, 32}},
	{'z', 11, {48, 96, 16, 32}},
	{'0', 11, {64, 96, 16, 32}},
	{'1', 7, {80, 96, 16, 32}},
	{'2', 11, {96, 96, 16, 32}},
	{'3', 11, {112, 96, 16, 32}},
	{'4', 11, {128, 96, 16, 32}},
	{'5', 11, {144, 96, 16, 32}},
	{'6', 11, {160, 96, 16, 32}},
	{'7', 11, {176, 96, 16, 32}},
	{'8', 11, {192, 96, 16, 32}},
	{'9', 11, {208, 96, 16, 32}},
	{'!', 5, {224, 96, 16, 32}},
	{'?', 13, {240, 96, 16, 32}},
	{'-', 11, {0, 128, 16, 32}},
	{'.', 7, {16, 128, 16, 32}},
	{',', 7, {32, 128, 16, 32}},
	{'\0', 16, {48, 128, 16, 32}},
	{'>', 11, {64, 128, 16, 32}},
	{'(', 11, {80, 128, 16, 32}},
	{')', 11, {96, 128, 16, 32}},
	{'\0', 16, {112, 128, 16, 32}},
	{'\0', 16, {128, 128, 16, 32}},
	{'\0', 16, {144, 128, 16, 32}},
	{'\0', 16, {160, 128, 16, 32}},
	{'\0', 16, {176, 128, 16, 32}},
	{'"', 9, {192, 128, 16, 32}},
	{'\0', 16, {208, 128, 16, 32}},
	{'\0', 16, {224, 128, 16, 32}},
	{'\0', 16, {240, 128, 16, 32}},
	{'\0', 16, {0, 160, 16, 32}},
	{'\'', 7, {16, 160, 16, 32}},
	{':', 5, {32, 160, 16, 32}},
	{'/', 13, {48, 160, 16, 32}},
	{'~', 13, {64, 160, 16, 32}},
	{'&', 15, {80, 160, 16, 32}},
	{'#', 13, {96, 160, 16, 32}},
	{'\0', 16, {112, 160, 16, 32}},
	{'\0', 16, {128, 160, 16, 32}},
	{' ', 8, {144, 160, 16, 32}},
	{'<', 11, {160, 160, 16, 32}},
	{'\0', 16, {176, 160, 16, 32}},
	{'\0', 16, {192, 160, 16, 32}},
	{'\0', 16, {208, 160, 16, 32}},
	{'\0', 16, {224, 160, 16, 32}},
	{';', 7, {240, 160, 16, 32}}
};

Sign::Sign()
{
	_texture = TextureManager::Get("letters.png");
	_nextPageSound = SoundManager::Get("nextpage.wav");
}

void Sign::Show(const std::string &str)
{
	_page = 0;
	_pages.clear();

	std::stringstream ssin(str);
	std::stringstream sspage;
	std::string line;
	int i = 0;

	while (std::getline(ssin, line))
	{
		sspage << line << '\n';

		++i;
		if (i == 3)
		{
			_pages.push_back(sspage.str());

			sspage.str(std::string());

			i = 0;
		}
	}

	if (auto page = sspage.str(); !page.empty())
	{
		_pages.push_back(page);
	}

	_open = !_pages.empty();
}

void Sign::Draw(const float width, const float height) const
{
	if (!_open)
	{
		return;
	}

	auto x = (static_cast<float>(GetScreenWidth()) - width) / 2;
	auto y = static_cast<float>(GetScreenHeight()) - height - SIGN_VERTICAL_OFFSET;

	rlPushMatrix();
	rlTranslatef(x, y, 0);

	DrawFrame(width, height);

	auto &str = _pages[_page];

	if (str.empty())
	{
		return;
	}

	DrawLetters(str);

	rlPopMatrix();
}

void Sign::Update()
{
	if (!_open)
	{
		return;
	}

	if (IsNextKeyPressed())
	{
		_page++;

		if (_page >= _pages.size())
		{
			_open = false;

			return;
		}

		PlaySound(_nextPageSound);
	}
}

void Sign::DrawFrame(const float width, const float height) const
{
	DrawTextureRec(_texture, {0, 192, 16, 32}, {0, 0}, WHITE);
	DrawTextureRec(_texture, {16, 192, 16, 32}, {width - 16, 0}, WHITE);
	DrawTextureRec(_texture, {32, 192, 16, 32}, {0, height - 32}, WHITE);
	DrawTextureRec(_texture, {48, 192, 16, 32}, {width - 16, height - 32}, WHITE);

	auto borderx = width - 32;
	if (borderx > 0)
	{
		DrawTexturePro(_texture, {64, 192, 16, 32}, {16, 0, borderx, 32}, {0, 0}, 0, WHITE);
		DrawTexturePro(_texture, {80, 192, 16, 32}, {16, height - 32, borderx, 32}, {0, 0}, 0, WHITE);
	}

	auto bordery = height - 64;
	if (bordery > 0)
	{
		DrawTexturePro(_texture, {96, 192, 16, 32}, {0, 32, 16, bordery}, {0, 0}, 0, WHITE);
		DrawTexturePro(_texture, {112, 192, 16, 32}, {width - 16, 32, 16, bordery}, {0, 0}, 0, WHITE);
	}

	if (borderx > 0 && bordery > 0)
	{
		DrawRectangle(
			16, 32,
			static_cast<int>(borderx),
			static_cast<int>(bordery),
			SIGN_BACKGROUND);
	}
}

void Sign::DrawLetters(const std::string &str) const
{
	rlPushMatrix();
	rlTranslatef(16, 16, 0);

	Vector2 p{0, 0};

	for (const auto &c: str)
	{
		if (c == '\n')
		{
			p.x = 0;
			p.y += 32;
			continue;
		}

		DrawLetter(c, p);
	}

	rlPopMatrix();
}

void Sign::DrawLetter(const char c, Vector2 &pos) const
{
	for (auto &[ch, width, rect]: Glyphs)
	{
		if (ch != c)
		{
			continue;
		}

		DrawTextureRec(_texture, rect, pos, WHITE);

		pos.x += width + 1;

		return;
	}
}

bool Sign::IsNextKeyPressed()
{
	return IsKeyPressed(KEY_SPACE) ||
	       IsKeyPressed(KEY_LEFT) ||
	       IsKeyPressed(KEY_DOWN) ||
	       IsKeyPressed(KEY_RIGHT);
}
