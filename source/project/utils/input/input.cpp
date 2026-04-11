#include <stdafx.hpp>

bool input::initialize( )
{
	return true;
}

void input::inject_mouse( int x, int y, std::uint8_t buttons )
{
	INPUT inputs[ 4 ]{};
	int count = 0;
	bool left_down_sent = false;
	bool left_up_sent = false;
	bool right_down_sent = false;
	bool right_up_sent = false;

	if ( buttons & mouse_buttons::move )
	{
		inputs[ count ].type = INPUT_MOUSE;
		inputs[ count ].mi.dx = x;
		inputs[ count ].mi.dy = y;
		inputs[ count ].mi.dwFlags = MOUSEEVENTF_MOVE;
		++count;
	}

	if ( buttons & mouse_buttons::left_down )
	{
		inputs[ count ].type = INPUT_MOUSE;
		inputs[ count ].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		++count;
		left_down_sent = true;
	}

	if ( buttons & mouse_buttons::left_up )
	{
		inputs[ count ].type = INPUT_MOUSE;
		inputs[ count ].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		++count;
		left_up_sent = true;
	}

	if ( buttons & mouse_buttons::right_down )
	{
		inputs[ count ].type = INPUT_MOUSE;
		inputs[ count ].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		++count;
		right_down_sent = true;
	}

	if ( buttons & mouse_buttons::right_up )
	{
		inputs[ count ].type = INPUT_MOUSE;
		inputs[ count ].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		++count;
		right_up_sent = true;
	}

	if ( count > 0 )
	{
		if ( ::SendInput( count, inputs, sizeof( INPUT ) ) == static_cast< UINT >( count ) )
		{
			std::lock_guard lock( m_key_mutex );
			if ( left_down_sent )
				m_held_keys.insert( 0x01 );
			if ( left_up_sent )
				m_held_keys.erase( 0x01 );
			if ( right_down_sent )
				m_held_keys.insert( 0x02 );
			if ( right_up_sent )
				m_held_keys.erase( 0x02 );
		}
	}
}

void input::inject_keyboard( std::uint16_t key, bool pressed )
{
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key;
	input.ki.wScan = static_cast< std::uint16_t >( MapVirtualKeyW( key, MAPVK_VK_TO_VSC ) );
	input.ki.dwFlags = pressed ? 0 : KEYEVENTF_KEYUP;

	if ( ::SendInput( 1, &input, sizeof( INPUT ) ) == 1 )
	{
		std::lock_guard lock( m_key_mutex );
		if ( pressed )
			m_held_keys.insert( key );
		else
			m_held_keys.erase( key );
	}
}

bool input::is_key_held( std::uint16_t key ) const
{
	if ( key >= 0x01 && key <= 0x06 )
	{
		std::lock_guard lock( m_key_mutex );
		return m_held_keys.contains( key ) || ( ::GetAsyncKeyState( key ) & 0x8000 ) != 0;
	}
	return ( ::GetAsyncKeyState( key ) & 0x8000 ) != 0;
}
