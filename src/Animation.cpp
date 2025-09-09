#include "Animation.h"
#include "TextureManager.h"
#include "Utils.h"
#include "SoundManager.h"

#include <fstream>
#include <rlgl.h>
#include <boost/algorithm/string.hpp>

Animation::Animation()
		: _defaultAttr1("hat0.png"),
		  _defaultHead("head19.png"),
		  _defaultBody("body.png")
{
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

	_sprites[sprite.Id] = sprite;
}

Animation::SpriteSource Animation::ParseSpriteSource(const std::string &str)
{
	if (str == "SPRITES") return Animation::SpriteSource::Sprites;
	if (str == "SHIELD") return Animation::SpriteSource::Shield;
	if (str == "SWORD") return Animation::SpriteSource::Sword;
	if (str == "HEAD") return Animation::SpriteSource::Head;
	if (str == "BODY") return Animation::SpriteSource::Body;
	if (str == "ATTR1") return Animation::SpriteSource::Attr1;

	return Animation::SpriteSource::File;
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

		if (_singleDirection)
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
				_frames.push_back(frame);
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

		_frames.push_back(frame);
	}
}

void Animation::ParseSprites(std::string &line, std::vector<SpriteRef> &frame)
{
	std::vector<std::string> spriteInfos;
	std::vector<std::string> tokens;

	boost::trim(line);

	if (line.empty())
	{
		return;
	}

	boost::split(spriteInfos, line, boost::is_any_of(","));

	for (auto &spriteInfo: spriteInfos)
	{
		boost::trim(spriteInfo);

		if (spriteInfo.empty())
		{
			continue;
		}

		boost::split(tokens, spriteInfo, boost::is_any_of(" "), boost::token_compress_on);
		if (tokens.size() != 3)
		{
			continue;
		}

		auto id = std::stoi(tokens[0]);
		auto sprite = _sprites.find(id);

		if (sprite == _sprites.end())
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
			_singleDirection = true;
		}
		else if (tokens[0] == "CONTINUOUS")
		{
			_continuous = true;
		}
		else if (tokens[0] == "SETBACKTO")
		{
			_setBackTo = line.substr(10);
		}
		else if (tokens[0] == "DEFAULTATTR1")
		{
			_defaultAttr1 = line.substr(13);
		}
		else if (tokens[0] == "DEFAULTHEAD")
		{
			_defaultHead = line.substr(12);
		}
		else if (tokens[0] == "DEFAULTBODY")
		{
			_defaultBody = line.substr(12);
		}
		else if (tokens[0] == "ANI")
		{
			ParseAni(file);
		}
	}
}

void AnimationState::Reset(size_t frame, Animation *animation)
{
	if (animation == nullptr)
	{
		return;
	}

	auto maxFrame = animation->GetFrameCount() - 1;

	if (frame > maxFrame)
	{
		frame = maxFrame;
	}

	Frame = frame;
	NextFrame = animation->GetFrameDuration(frame);
	Ended = false;

	animation->PlaySound(frame);
}

void Animation::PlaySound(size_t frame) const
{
	auto &sound = _frames[frame].PlaySound;

	if (!sound.empty())
	{
		PlaySound(sound, {0, 0});
	}
}

void Animation::PlaySound(const std::string &fileName, const Vector2 &position)
{
	auto sound = SoundManager::Get(fileName);

	if (IsSoundValid(sound))
	{
		::PlaySound(sound);
	}
}

void Animation::Update(float dt, AnimationState &state)
{
	if (state.Frame < 0 || state.Frame >= _frames.size())
	{
		state.Frame = 0;
		state.NextFrame = _frames[0].Duration;
	}

	state.NextFrame -= dt;

	if (state.NextFrame > 0)
	{
		return;
	}

	if (state.Frame < _frames.size() - 1)
	{
		state.Frame++;
		state.NextFrame = _frames[state.Frame].Duration;

		auto &sound = _frames[state.Frame].PlaySound;
		if (!sound.empty())
		{
			PlaySound(sound, {0, 0});
		}

		return;
	}

	if (_continuous)
	{
		state.Frame = 0;
		state.NextFrame = _frames[0].Duration;

		auto &sound = _frames[state.Frame].PlaySound;
		if (!sound.empty())
		{
			PlaySound(sound, {0, 0});
		}

		return;
	}

	state.Ended = true;
}

void Animation::Draw(float x, float y, int direction, const AnimationState &state) const
{
	if (_frames.empty())
	{
		return;
	}

	if (_singleDirection)
	{
		direction = DIR_UP;
	}

	auto frameIndex = state.Frame;

	if (frameIndex > _frames.size() - 1)
	{
		frameIndex = _frames.size() - 1;
	}

	auto &frame = _frames[frameIndex];
	auto &sprites = frame.Sprites[direction];

	if (sprites.empty())
	{
		return;
	}

	rlPushMatrix();
	rlTranslatef(x, y, 0);
	DrawSprites(state, sprites);

	rlPopMatrix();
}

void Animation::DrawSprites(const AnimationState &state, const std::vector<SpriteRef> &sprites) const
{
	for (const auto &spriteRef: sprites)
	{
		if (spriteRef.Sprite == nullptr)
		{
			continue;
		}

		auto textureName = GetTextureName(state, spriteRef);

		if (textureName.empty())
		{
			continue;
		}

		auto texture = TextureManager::Get(textureName);

		if (!IsTextureValid(texture))
		{
			continue;
		}

		auto sx = static_cast<float>(spriteRef.Sprite->X);
		auto sy = static_cast<float>(spriteRef.Sprite->Y);
		auto sw = static_cast<float>(spriteRef.Sprite->W);
		auto sh = static_cast<float>(spriteRef.Sprite->H);

		auto dx = static_cast<float>(spriteRef.X);
		auto dy = static_cast<float>(spriteRef.Y);

		DrawTextureRec(
				texture,
				{sx, sy, sw, sh},
				{dx, dy},
				WHITE);
	}
}

std::string Animation::GetTextureName(const AnimationState &state, const SpriteRef &spriteRef) const
{
	switch (spriteRef.Sprite->Source)
	{
		case SpriteSource::File:
			return spriteRef.Sprite->Texture;

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
				return _defaultHead;
			}

			return state.Head;
		}

		case SpriteSource::Body:
		{
			if (state.Body.empty())
			{
				return _defaultBody;
			}

			return state.Body;
		}

		case SpriteSource::Attr1:
			return state.Attr1;
	}

	return "";
}