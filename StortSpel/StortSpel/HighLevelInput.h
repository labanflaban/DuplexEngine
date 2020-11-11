#pragma once



enum Action
{
	JUMP,
	DASH,
	ROLL,
	USE,
	PLAYSOUND,
	SWAP_SCENES,
	CLOSEINTROGUI,
	PAUSE
};

enum State
{
	WALK_LEFT,
	WALK_RIGHT,
	WALK_FORWARD,
	WALK_BACKWARD,
};

enum Range
{
	RAW,
	REL,
};

