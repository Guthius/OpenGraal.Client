#pragma once

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

	void Reset(size_t frame, const Animation *animation);
};

class Animation
{
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
	Animation() = default;

private:
	static auto ParseSpriteSource(const std::string &str) -> SpriteSource;

	void ParseSprite(const std::vector<std::string> &tokens);
	void ParseAni(std::ifstream &stream);
	void ParseSprites(std::string &line, std::vector<SpriteRef> &frame);

public:
	void Load(const std::filesystem::path &path);
	void Update(float dt, AnimationState &state) const;
	void Draw(float x, float y, int direction, const AnimationState &state) const;

	[[nodiscard]]
	auto GetFrameCount() const -> size_t { return frames_.size(); }

	[[nodiscard]]
	auto GetFrameDuration(const size_t frame) const -> float { return frames_[frame].Duration; }

	void PlaySound(size_t frame) const;

private:
	static void PlaySound(const std::string &filename, const Vector2 &position);

	void DrawSprites(const AnimationState &state, const std::vector<SpriteRef> &sprite_refs) const;

	[[nodiscard]] auto GetTextureName(const AnimationState &state, const SpriteRef &sprite_ref) const -> std::string;

	std::string set_back_to_;
	std::string default_attr1_{"hat0.png"};
	std::string default_head_{"head19.png"};
	std::string default_body_{"body.png"};
	bool single_direction_ = false;
	bool continuous_ = false;
	std::map<int, Sprite> sprites_{};
	std::vector<Frame> frames_{};
};
