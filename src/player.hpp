#pragma once

#include "actor.hpp"



class game;

enum class player_state
{
	idle,
	walk,
	grab,
	push,
	pull,
	swim,
	sit,
	jump,
	attack,

	lift,
	carrystill,
	carry
};

class player final : public actor
{
public:
	explicit player(game *game);

	void update(float dt) override;
	void update_new(float dt);

private:
	void ReturnIdle();
	auto CheckForLevelLinkAt(const Vector2 &position) -> bool;
	auto check_for_sign_at(const Vector2 &position) const -> bool;
	void CheckAttack(const Vector2 &position);
	void TryDestroyObjectFacing(const Vector2 &position) const;
	auto TryMoveFromWall(Vector2 position) -> void;
	void CheckPushAndPull();
	auto try_pickup_item() -> bool;
	void CheckThrow();
	void UpdateAnimation();
	auto GetTileFacing() const -> int;

	auto drop_carried_object() -> bool;

	auto CheckJump(float dt, Vector2 &position) -> bool;
	auto CanJump(const Vector2 &position) const -> bool;
	void Jump();
	auto JumpUpdate(float dt, Vector2 &position) -> bool;


	void state_enter(player_state state);
	auto state_update(player_state state, float dt) -> player_state;
	void state_exit(player_state state);
	auto attack() -> player_state;

	bool get_carried_destination_override(Vector2 &dest) const override;

	game *game_;
	player_state state_ = player_state::idle;
	float walk_speed_ = 245.0f;
	Texture2D sprites_{};
	float push_timer_ = 0.0f;
	float jump_timer_ = 0.0f;
	int jump_step_ = 0;
	Vector2 jump_origin_{};
	Vector2 jump_from_{};
	Vector2 jump_to_{};
	Sound jump_sound_{};
	float lift_show_timer_ = 0.0f;
};
