#pragma once

#include "Constants.h"

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <raylib.h>

class Animation;
struct AnimationState
{
	size_t Frame;
	float NextFrame;
	bool Ended;
	std::string Body{"body.png"};
	std::string Head{"head0.png"};
	std::string Sword{"sword1.png"};
	std::string Shield{"shield1.png"};
	std::string Attr1{"hat0.png"};

	void Reset(size_t frame, Animation *animation);
};

class Animation
{
private:
	enum class SpriteSource
	{
		File,
		Sprites,
		Shield,
		Sword,
		Head,
		Body,
		Attr1
	};

	struct Sprite
	{
		int Id;
		SpriteSource Source;
		std::string Texture;
		int X, Y, W, H;
	};

	struct SpriteRef
	{
		Sprite *Sprite;
		int X, Y;
	};

	struct Frame
	{
		std::vector<SpriteRef> Sprites[4]{};
		float Duration;
		std::string PlaySound;
		Vector2 PlaySoundAt;
	};

public:
	Animation();

private:
	void ParseSprite(const std::vector<std::string> &tokens);
	static SpriteSource ParseSpriteSource(const std::string &str);
	void ParseAni(std::ifstream &stream);
	void ParseSprites(std::string &line, std::vector<SpriteRef> &frame);

public:
	void Load(const std::filesystem::path &path);
	void Update(float dt, AnimationState &state);
	void Draw(float x, float y, int direction, const AnimationState &state) const;
	[[nodiscard]] size_t GetFrameCount() const { return _frames.size(); }
	[[nodiscard]] float GetFrameDuration(size_t frame) const { return _frames[frame].Duration; }
	void PlaySound(size_t frame) const;

private:
	void DrawSprites(const AnimationState &state, const std::vector<SpriteRef> &sprites) const;
	static void PlaySound(const std::string &fileName, const Vector2 &position);


private:
	[[nodiscard]] std::string GetTextureName(const AnimationState &state, const SpriteRef &spriteRef) const;

private:
	std::string _setBackTo;
	std::string _defaultAttr1;
	std::string _defaultHead;
	std::string _defaultBody;
	bool _singleDirection = false;
	bool _continuous = false;

	std::map<int, Sprite> _sprites{};
	std::vector<Frame> _frames{};
};