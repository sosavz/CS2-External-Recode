#pragma once

class menu
{
public:
	void draw( );
	[[nodiscard]] bool is_open( ) const noexcept { return this->m_open; }

private:
	enum class tab : int { combat = 0, esp, misc, config, count };

	void draw_header( float width, float height );
	void draw_content( float width, float height );
	void draw_accent_lines( const zui::rect& bounds, float fade_ratio = 0.15f );

	void draw_combat( );
	void draw_esp( );
	void draw_misc( );
	void draw_config( );

	tab m_tab{ tab::combat };
	bool m_open{ true };

	float m_x{ 200.0f };
	float m_y{ 100.0f };
	float m_w{ 960.0f };
	float m_h{ 540.0f };

	static constexpr const char* k_weapon_groups[ ]{ "pistol", "smg", "rifle", "shotgun", "sniper", "lmg" };
	int m_weapon_group{ 0 };
	std::string m_cfg_name{ "default" };
	int m_cfg_selected{ 0 };
};
