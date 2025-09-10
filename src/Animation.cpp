#include "Animation.h"

#include <fstream>
#include <rlgl.h>
#include <boost/algorithm/string.hpp>

#include "Constants.h"
#include "SoundManager.h"
#include "TextureManager.h"
#include "Utils.h"

void AnimationState::Reset(size_t frame, const Animation *animation)
{
	if (animation == nullptr)
	{
		return;
	}

	if (const auto max_frame = animation->GetFrameCount() - 1; frame > max_frame)
	{
		frame = max_frame;
	}

	Frame = frame;
	NextFrame = animation->GetFrameDuration(frame);
	Ended = false;

	animation->PlaySound(frame);
}

Animation::SpriteSource Animation::ParseSpriteSource(const std::string &str)
{
	if (str == "SPRITES") return SpriteSource::Sprites;
	if (str == "SHIELD") return SpriteSource::Shield;
	if (str == "SWORD") return SpriteSource::Sword;
	if (str == "HEAD") return SpriteSource::Head;
	if (str == "BODY") return SpriteSource::Body;
	if (str == "ATTR1") return SpriteSource::Attr1;

	return SpriteSource::File;
}

void Animation::ParseSprite(const std::vector<std::string> &tokens)
{
	if (tokens.size() < 7)
	{
		return;
	}

	Sprite sprite;

	sprite.Id = std::stoi(tokens[1]);
	sprite.Source = ParseSpriteSource(tokens[2]);
	sprite.X = std::stoi(tokens[3]);
	sprite.Y = std::stoi(tokens[4]);
	sprite.W = std::stoi(tokens[5]);
	sprite.H = std::stoi(tokens[6]);

	if (sprite.Source == SpriteSource::File)
	{
		sprite.Texture = tokens[2];
	}

	sprites_[sprite.Id] = sprite;
}

void Animation::ParseAni(std::ifstream &stream)
{
	std::string line;
	while (std::getline(stream, line))
	{
		boost::trim(line);

		if (line == "ANIEND")
		{
			return;
		}

		Frame frame{};

		frame.Duration = 0.06f;

		if (single_direction_)
		{
			ParseSprites(line, frame.Sprites[0]);
		}
		else
		{
			ParseSprites(line, frame.Sprites[0]);

			if (!std::getline(stream, line)) break;
			ParseSprites(line, frame.Sprites[1]);

			if (!std::getline(stream, line)) break;
			ParseSprites(line, frame.Sprites[2]);

			if (!std::getline(stream, line)) break;
			ParseSprites(line, frame.Sprites[3]);
		}

		while (std::getline(stream, line))
		{
			boost::trim(line);

			if (line == "ANIEND")
			{
				frames_.push_back(frame);
				return;
			}

			if (line.empty())
			{
				break;
			}

			auto tokens = Split(line);
			if (tokens.empty())
			{
				break;
			}

			if (tokens[0] == "WAIT" && tokens.size() == 2)
			{
				frame.Duration = std::stof(tokens[1]) / 10;
			}
			else if (tokens[0] == "PLAYSOUND" && tokens.size() == 4)
			{
				frame.PlaySound = tokens[1];
				frame.PlaySoundAt =
				{
					std::stof(tokens[2]),
					std::stof(tokens[3])
				};
			}
		}

		frames_.push_back(frame);
	}
}

void Animation::ParseSprites(std::string &line, std::vector<SpriteRef> &frame)
{
	std::vector<std::string> sprite_infos;
	std::vector<std::string> tokens;

	boost::trim(line);

	if (line.empty())
	{
		return;
	}

	boost::split(sprite_infos, line, boost::is_any_of(","));

	for (auto &sprite_info: sprite_infos)
	{
		boost::trim(sprite_info);

		if (sprite_info.empty())
		{
			continue;
		}

		boost::split(tokens, sprite_info, boost::is_any_of(" "), boost::token_compress_on);
		if (tokens.size() != 3)
		{
			continue;
		}

		auto id = std::stoi(tokens[0]);
		auto sprite = sprites_.find(id);

		if (sprite == sprites_.end())
		{
			continue;
		}

		SpriteRef spriteRef{};

		spriteRef.Sprite = &sprite->second;
		spriteRef.X = std::stoi(tokens[1]);
		spriteRef.Y = std::stoi(tokens[2]);

		frame.push_back(spriteRef);
	}
}

void Animation::Load(const std::filesystem::path &path)
{
	if (!is_regular_file(path))
	{
		return;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		boost::trim(line);

		if (line.empty())
		{
			continue;
		}

		auto tokens = Split(line);

		if (tokens.empty())
		{
			continue;
		}

		if (tokens[0] == "SPRITE")
		{
			ParseSprite(tokens);
		}
		else if (tokens[0] == "SINGLEDIRECTION")
		{
			single_direction_ = true;
		}
		else if (tokens[0] == "CONTINUOUS")
		{
			continuous_ = true;
		}
		else if (tokens[0] == "SETBACKTO")
		{
			set_back_to_ = line.substr(10);
		}
		else if (tokens[0] == "DEFAULTATTR1")
		{
			default_attr1_ = line.substr(13);
		}
		else if (tokens[0] == "DEFAULTHEAD")
		{
			default_head_ = line.substr(12);
		}
		else if (tokens[0] == "DEFAULTBODY")
		{
			default_body_ = line.substr(12);
		}
		else if (tokens[0] == "ANI")
		{
			ParseAni(file);
		}
	}
}

void Animation::Update(const float dt, AnimationState &state) const
{
	if (state.Frame < 0 || state.Frame >= frames_.size())
	{
		state.Frame = 0;
		state.NextFrame = frames_[0].Duration;
	}

	state.NextFrame -= dt;

	if (state.NextFrame > 0)
	{
		return;
	}

	if (state.Frame < frames_.size() - 1)
	{
		state.Frame++;
		state.NextFrame = frames_[state.Frame].Duration;

		auto &sound = frames_[state.Frame].PlaySound;
		if (!sound.empty())
		{
			PlaySound(sound, {0, 0});
		}

		return;
	}

	if (continuous_)
	{
		state.Frame = 0;
		state.NextFrame = frames_[0].Duration;

		auto &sound = frames_[state.Frame].PlaySound;
		if (!sound.empty())
		{
			PlaySound(sound, {0, 0});
		}

		return;
	}

	state.Ended = true;
}

void Animation::Draw(float x, float y, Direction direction, const AnimationState &state) const
{
	if (frames_.empty())
	{
		return;
	}

	if (single_direction_)
	{
		direction = Direction::DIR_UP;
	}

	auto frame_index = state.Frame;
	if (frame_index > frames_.size() - 1)
	{
		frame_index = frames_.size() - 1;
	}

	auto &frame = frames_[frame_index];
	auto &sprites = frame.Sprites[static_cast<int>(direction)];

	if (sprites.empty())
	{
		return;
	}

	rlPushMatrix();
	rlTranslatef(x, y, 0);

	DrawSprites(state, sprites);

	rlPopMatrix();
}

void Animation::PlaySound(const size_t frame) const
{
	if (auto &sound = frames_[frame].PlaySound; !sound.empty())
	{
		PlaySound(sound, {0, 0});
	}
}

void Animation::PlaySound(const std::string &filename, const Vector2 &position)
{
	if (const auto sound = SoundManager::Get(filename); IsSoundValid(sound))
	{
		::PlaySound(sound);
	}
}

void Animation::DrawSprites(const AnimationState &state, const std::vector<SpriteRef> &sprite_refs) const
{
	for (const auto &sprite_ref: sprite_refs)
	{
		if (sprite_ref.Sprite == nullptr)
		{
			continue;
		}

		auto texture_name = GetTextureName(state, sprite_ref);
		if (texture_name.empty())
		{
			continue;
		}

		const auto texture = TextureManager::Get(texture_name);

		if (!IsTextureValid(texture))
		{
			continue;
		}

		const auto sx = static_cast<float>(sprite_ref.Sprite->X);
		const auto sy = static_cast<float>(sprite_ref.Sprite->Y);
		const auto sw = static_cast<float>(sprite_ref.Sprite->W);
		const auto sh = static_cast<float>(sprite_ref.Sprite->H);

		const auto dx = static_cast<float>(sprite_ref.X);
		const auto dy = static_cast<float>(sprite_ref.Y);

		DrawTextureRec(
			texture,
			{sx, sy, sw, sh},
			{dx, dy},
			WHITE);
	}
}

std::string Animation::GetTextureName(const AnimationState &state, const SpriteRef &sprite_ref) const
{
	switch (sprite_ref.Sprite->Source)
	{
		case SpriteSource::File:
			return sprite_ref.Sprite->Texture;

		case SpriteSource::Sprites:
			return "sprites.png";

		case SpriteSource::Shield:
			return state.Shield;

		case SpriteSource::Sword:
			return state.Sword;

		case SpriteSource::Head:
			{
				if (state.Head.empty())
				{
					return default_head_;
				}

				return state.Head;
			}

		case SpriteSource::Body:
			{
				if (state.Body.empty())
				{
					return default_body_;
				}

				return state.Body;
			}

		case SpriteSource::Attr1:
			return state.Attr1;
	}

	return {};
}
