#pragma once

#include <stdafx.hpp>

class input
{
public:
	bool initialize( );
	void inject_mouse( int x, int y, std::uint8_t buttons );
	void inject_keyboard( std::uint16_t key, bool pressed );
	bool is_key_held( std::uint16_t key ) const;

	enum mouse_buttons : std::uint8_t
	{
		none = 0,
		left_down = 1 << 0,
		left_up = 1 << 1,
		right_down = 1 << 2,
		right_up = 1 << 3,
		move = 1 << 4
	};

private:
	std::unordered_set<std::uint16_t> m_held_keys;
	mutable std::mutex m_key_mutex;
};