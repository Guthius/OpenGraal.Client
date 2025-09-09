#pragma once

#include <raylib.h>
#include <string>
#include <vector>

class Sign
{
	struct Letter
	{
		char ch;
		float width;
		Rectangle rect;
	};

	static Letter Glyphs[96];

public:
	Sign();

	void Show(const std::string &str);
	void Draw(float width, float height) const;
	void Update();

private:
	void DrawFrame(float width, float height) const;
	void DrawLetters(const std::string &str) const;
	void DrawLetter(char c, Vector2 &pos) const;

public:
	[[nodiscard]] auto IsOpen() const -> bool { return _open; }

private:
	static auto IsNextKeyPressed() -> bool;

	Texture2D _texture{};
	bool _open = false;
	std::vector<std::string> _pages;
	int _page = 0;
	Sound _nextPageSound{};
};
