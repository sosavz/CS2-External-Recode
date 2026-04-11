#include <stdafx.hpp>

#include <fstream>
#include <cstdarg>
#include <ctime>

void menu::draw( )
{
	g::config.tick( );

	if ( GetAsyncKeyState( VK_INSERT ) & 1 )
	{
		this->m_open = !this->m_open;
	}

	if ( !this->m_open )
	{
		return;
	}

	{
		auto& style = zui::get_style( );
		style.window_padding_x = 12.0f;
		style.window_padding_y = 12.0f;
		style.item_spacing_y = 8.0f;
		style.frame_padding_x = 8.0f;
		style.frame_padding_y = 4.0f;
		style.checkbox_size = 14.0f;
		style.slider_height = 8.0f;

		style.window_bg = { 8, 8, 10, 252 };
		style.window_border = { 0, 0, 0, 0 };
		style.nested_bg = { 10, 10, 12, 245 };
		style.nested_border = { 0, 0, 0, 0 };

		style.group_box_bg = { 12, 11, 16, 245 };
		style.group_box_border = { 0, 0, 0, 0 };
		style.group_box_title_text = { 214, 194, 225, 255 };

		style.checkbox_bg = { 16, 14, 21, 255 };
		style.checkbox_border = { 34, 26, 42, 255 };
		style.checkbox_check = { 192, 136, 214, 255 };

		style.slider_bg = { 15, 13, 20, 255 };
		style.slider_border = { 34, 26, 42, 255 };
		style.slider_fill = { 177, 112, 194, 255 };
		style.slider_grab = { 201, 145, 219, 255 };
		style.slider_grab_active = { 216, 167, 230, 255 };

		style.button_bg = { 14, 13, 18, 255 };
		style.button_border = { 0, 0, 0, 0 };
		style.button_hovered = { 22, 18, 28, 255 };
		style.button_active = { 12, 11, 16, 255 };

		style.keybind_bg = { 15, 13, 20, 255 };
		style.keybind_border = { 34, 26, 42, 255 };
		style.keybind_waiting = { 202, 149, 220, 255 };

		style.combo_bg = { 15, 13, 20, 255 };
		style.combo_border = { 26, 20, 33, 180 };
		style.combo_arrow = { 196, 145, 216, 255 };
		style.combo_hovered = { 23, 19, 30, 255 };
		style.combo_popup_bg = { 12, 11, 16, 252 };
		style.combo_popup_border = { 26, 20, 33, 255 };
		style.combo_item_hovered = { 25, 20, 31, 255 };
		style.combo_item_selected = { 170, 108, 190, 45 };

		style.popup_bg = { 12, 11, 16, 252 };
		style.popup_border = { 20, 16, 27, 200 };

		style.text = { 220, 212, 226, 255 };
		style.accent = { 182, 118, 203, 255 };
	}

	zui::begin( );

	if ( zui::begin_window( "velora##main", this->m_x, this->m_y, this->m_w, this->m_h, true, 820.0f, 461.0f ) )
	{
		const auto [avail_w, avail_h] = zui::get_content_region_avail( );

		if ( zui::begin_nested_window( "##inner", avail_w, avail_h ) )
		{
			constexpr auto header_h{ 44.0f };
			constexpr auto padding{ 10.0f };

			zui::set_cursor_pos( padding, padding );
			this->draw_header( avail_w - padding * 2.0f, header_h );

			zui::set_cursor_pos( padding, padding + header_h + padding );
			this->draw_content( avail_w - padding * 2.0f, avail_h - header_h - padding * 3.0f );

			if ( const auto win = zui::detail::get_current_window( ) )
			{
				this->draw_accent_lines( win->bounds );
			}

			zui::end_nested_window( );
		}

		zui::end_window( );
	}

	zui::end( );
}

void menu::draw_header( float width, float height )
{
	if ( !zui::begin_nested_window( "##header", width, height ) )
	{
		return;
	}

	const auto current = zui::detail::get_current_window( );
	if ( !current )
	{
		zui::end_nested_window( );
		return;
	}

	const auto& style = zui::get_style( );
	const auto bx = current->bounds.x;
	const auto by = current->bounds.y;
	const auto bw = current->bounds.w;
	const auto bh = current->bounds.h;

	zdraw::get_draw_list( ).add_rect_filled( bx, by, bw, bh, zdraw::rgba{ 12, 10, 15, 255 } );
	zdraw::get_draw_list( ).add_rect( bx, by, bw, bh, zdraw::rgba{ 0, 0, 0, 0 } );

	{
		constexpr auto title{ "VELORA" };
		auto [tw, th] = zdraw::measure_text( title );
		zdraw::get_draw_list( ).add_text( bx + 14.0f, by + ( bh - th ) * 0.5f, title, nullptr, zdraw::rgba{ 214, 194, 225, 255 } );
	}
	{
		constexpr auto build_text{ "alpha build" };
		auto [tw, th] = zdraw::measure_text( build_text );
		zdraw::get_draw_list( ).add_text( bx + bw - tw - 14.0f, by + ( bh - th ) * 0.5f, build_text, nullptr, zdraw::rgba{ 183, 147, 200, 230 } );
	}

	zui::end_nested_window( );
}

void menu::draw_content( float width, float height )
{
	zui::push_style_var( zui::style_var::window_padding_x, 12.0f );
	zui::push_style_var( zui::style_var::window_padding_y, 12.0f );

	if ( !zui::begin_nested_window( "##content", width, height ) )
	{
		zui::pop_style_var( 2 );
		return;
	}

	if ( const auto win = zui::detail::get_current_window( ) )
	{
		zdraw::get_draw_list( ).add_rect_filled( win->bounds.x, win->bounds.y, win->bounds.w, win->bounds.h, zdraw::rgba{ 8, 8, 10, 242 } );
		this->draw_accent_lines( win->bounds );
	}

	const auto [cw, ch] = zui::get_content_region_avail( );
	constexpr auto nav_w{ 176.0f };

	if ( zui::begin_nested_window( "##left_nav", nav_w, ch ) )
	{
		const auto selected = zdraw::rgba{ 182, 118, 203, 255 };
		const auto idle = zdraw::rgba{ 178, 161, 190, 255 };

		if ( this->m_tab == tab::combat )
		{
			zui::push_style_color( zui::style_color::button_border, selected );
		}
		if ( zui::button( "legit", nav_w - 12.0f, 34.0f ) )
		{
			this->m_tab = tab::combat;
		}
		if ( this->m_tab == tab::combat )
		{
			zui::pop_style_color( );
		}

		if ( this->m_tab == tab::esp )
		{
			zui::push_style_color( zui::style_color::button_border, selected );
		}
		if ( zui::button( "visuals", nav_w - 12.0f, 34.0f ) )
		{
			this->m_tab = tab::esp;
		}
		if ( this->m_tab == tab::esp )
		{
			zui::pop_style_color( );
		}

		if ( this->m_tab == tab::misc )
		{
			zui::push_style_color( zui::style_color::button_border, selected );
		}
		if ( zui::button( "misc", nav_w - 12.0f, 34.0f ) )
		{
			this->m_tab = tab::misc;
		}
		if ( this->m_tab == tab::misc )
		{
			zui::pop_style_color( );
		}

		if ( this->m_tab == tab::config )
		{
			zui::push_style_color( zui::style_color::button_border, selected );
		}
		if ( zui::button( "config", nav_w - 12.0f, 34.0f ) )
		{
			this->m_tab = tab::config;
		}
		if ( this->m_tab == tab::config )
		{
			zui::pop_style_color( );
		}

		zui::spacing( 2.0f );

		if ( this->m_tab == tab::dev )
		{
			zui::push_style_color( zui::style_color::button_border, selected );
		}
		if ( zui::button( "dev", nav_w - 12.0f, 34.0f ) )
		{
			this->m_tab = tab::dev;
		}
		if ( this->m_tab == tab::dev )
		{
			zui::pop_style_color( );
		}

		zui::spacing( 10.0f );
		zui::end_nested_window( );
	}

	zui::same_line( );

	if ( zui::begin_nested_window( "##main_panel", cw - nav_w - 8.0f, ch ) )
	{
		switch ( this->m_tab )
		{
		case tab::combat: this->draw_combat( ); break;
		case tab::esp:    this->draw_esp( );    break;
		case tab::misc:   this->draw_misc( );   break;
		case tab::config: this->draw_config( ); break;
		case tab::dev:    this->draw_dev( );    break;
		default: break;
		}

		zui::end_nested_window( );
	}

	zui::pop_style_var( 2 );
	zui::end_nested_window( );
}

void menu::draw_accent_lines( const zui::rect& bounds, float fade_ratio )
{
	( void )bounds;
	( void )fade_ratio;
}

void menu::draw_combat( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;
	auto& cfg = settings::g_combat.groups[ this->m_weapon_group ];

	if ( zui::begin_group_box( "weapon profile", col_w ) )
	{
		zui::combo( "group##combat_group", this->m_weapon_group, k_weapon_groups, 6 );
		zui::text_colored( "profile-specific legit settings", zdraw::rgba{ 170, 150, 182, 255 } );

		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "aimbot", col_w ) )
	{
		zui::checkbox( "enabled##aim", cfg.aimbot.enabled );
		if ( cfg.aimbot.enabled )
		{
			zui::keybind( "key##aim", cfg.aimbot.key );
			zui::slider_int( "fov##aim", cfg.aimbot.fov, 1, 45 );
			zui::slider_int( "smoothing##aim", cfg.aimbot.smoothing, 0, 50 );
			zui::checkbox( "head only##aim", cfg.aimbot.head_only );
			zui::checkbox( "visible only##aim", cfg.aimbot.visible_only );
			zui::checkbox( "predictive##aim", cfg.aimbot.predictive );
			zui::checkbox( "draw fov##aim", cfg.aimbot.draw_fov );
			if ( cfg.aimbot.draw_fov )
			{
				zui::color_picker( "fov color##aim", cfg.aimbot.fov_color );
			}
		}

		zui::end_group_box( );
	}

	if ( zui::begin_group_box( "triggerbot", col_w ) )
	{
		zui::checkbox( "enabled##trg", cfg.triggerbot.enabled );
		if ( cfg.triggerbot.enabled )
		{
			zui::keybind( "key##trg", cfg.triggerbot.key );
			zui::slider_float( "hitchance##trg", cfg.triggerbot.hitchance, 0.0f, 100.0f, "%.0f%%" );
			zui::slider_int( "delay ms##trg", cfg.triggerbot.delay, 0, 500 );
			zui::checkbox( "predictive##trg", cfg.triggerbot.predictive );
		}

		zui::end_group_box( );
	}

	( void )avail_h;
}

class dev_logger {
public:
	static dev_logger& get( ) {
		static dev_logger instance;
		return instance;
	}

	void init( ) {
		if ( m_file.is_open( ) )
			return;

		const auto now = std::chrono::system_clock::now( );
		const auto time_t = std::chrono::system_clock::to_time_t( now );
		std::tm tm{};
		localtime_s( &tm, &time_t );

		char buf[ 64 ];
		strftime( buf, sizeof( buf ), "%Y%m%d_%H%M%S", &tm );
		m_filename = std::format( "velora_dev_{}.log", buf );

		m_file.open( m_filename, std::ios::out | std::ios::trunc );
		if ( m_file.is_open( ) ) {
			log( "Dev logger initialized" );
		}
	}

	void log( const char* msg ) {
		if ( !m_file.is_open( ) )
			return;

		const auto now = std::chrono::system_clock::now( );
		const auto time_t = std::chrono::system_clock::to_time_t( now );
		std::tm tm{};
		localtime_s( &tm, &time_t );

		char buf[ 64 ];
		strftime( buf, sizeof( buf ), "%H:%M:%S", &tm );

		m_file << "[" << buf << "] " << msg << "\n";
		m_file.flush( );
	}

	void log_format( const char* fmt, ... ) {
		if ( !m_file.is_open( ) )
			return;

		char buffer[ 1024 ];
		va_list args;
		va_start( args, fmt );
		vsnprintf( buffer, sizeof( buffer ), fmt, args );
		va_end( args );

		log( buffer );
	}

	[[nodiscard]] bool is_enabled( ) const { return m_enabled; }
	void set_enabled( bool enabled ) { m_enabled = enabled; }
	[[nodiscard]] const std::string& filename( ) const { return m_filename; }

private:
	dev_logger( ) = default;
	~dev_logger( ) {
		if ( m_file.is_open( ) )
			m_file.close( );
	}

	bool m_enabled{ false };
	std::ofstream m_file;
	std::string m_filename;
};

void menu::draw_dev( ) {
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = avail_w - 8.0f;

	if ( zui::begin_group_box( "performance", col_w ) ) {
		zui::checkbox( "perf overlay##dev", settings::g_misc.perf_overlay );
		zui::checkbox( "log to file##dev", settings::g_misc.dev_logging );
		zui::text_colored( std::format( "log file: {}", dev_logger::get( ).filename( ).empty( ) ? "not initialized" : dev_logger::get( ).filename( ) ), zdraw::rgba{ 170, 150, 182, 255 } );

		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "logging", col_w ) ) {
		if ( zui::button( "init logger", col_w - 16.0f, 28.0f ) ) {
			dev_logger::get( ).init( );
		}
		if ( zui::button( "test log", col_w - 16.0f, 28.0f ) ) {
			dev_logger::get( ).log( "Test log entry" );
			dev_logger::get( ).log_format( "FPS: %d, FrameTime: %.2fms", perf::profiler::get( ).current_fps( ), perf::profiler::get( ).current_frame_ms( ) );
		}

		zui::end_group_box( );
	}
}

void menu::draw_esp( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	auto& p = settings::g_esp.m_player;
	auto& it = settings::g_esp.m_item;
	auto& pr = settings::g_esp.m_projectile;

	const auto nav_w = ( avail_w - 16.0f ) / 3.0f;
	if ( this->m_esp_panel == esp_panel::player )
	{
		zui::push_style_color( zui::style_color::button_border, zdraw::rgba{ 182, 118, 203, 255 } );
	}
	if ( zui::button( "player", nav_w, 30.0f ) )
	{
		this->m_esp_panel = esp_panel::player;
	}
	if ( this->m_esp_panel == esp_panel::player )
	{
		zui::pop_style_color( );
	}

	zui::same_line( );

	if ( this->m_esp_panel == esp_panel::items )
	{
		zui::push_style_color( zui::style_color::button_border, zdraw::rgba{ 182, 118, 203, 255 } );
	}
	if ( zui::button( "items", nav_w, 30.0f ) )
	{
		this->m_esp_panel = esp_panel::items;
	}
	if ( this->m_esp_panel == esp_panel::items )
	{
		zui::pop_style_color( );
	}

	zui::same_line( );

	if ( this->m_esp_panel == esp_panel::projectiles )
	{
		zui::push_style_color( zui::style_color::button_border, zdraw::rgba{ 182, 118, 203, 255 } );
	}
	if ( zui::button( "projectiles", nav_w, 30.0f ) )
	{
		this->m_esp_panel = esp_panel::projectiles;
	}
	if ( this->m_esp_panel == esp_panel::projectiles )
	{
		zui::pop_style_color( );
	}

	zui::new_line( );
	zui::spacing( 6.0f );

	const auto [body_w, body_h] = zui::get_content_region_avail( );
	( void )body_h;

	auto draw_player_core = [ & ]( float width )
	{
		if ( !zui::begin_group_box( "player core", width ) )
		{
			return;
		}

		zui::checkbox( "enabled##pl", p.enabled );

		zui::checkbox( "box##bx", p.m_box.enabled );
		if ( p.m_box.enabled )
		{
			auto cornered_style = p.m_box.style == settings::esp::player::box::style_type::cornered;
			if ( zui::checkbox( "cornered style##bx", cornered_style ) )
			{
				p.m_box.style = cornered_style ? settings::esp::player::box::style_type::cornered : settings::esp::player::box::style_type::full;
			}

			zui::checkbox( "fill##bx", p.m_box.fill );
			zui::checkbox( "outline##bx", p.m_box.outline );
			if ( p.m_box.style == settings::esp::player::box::style_type::cornered )
			{
				zui::slider_float( "corner len##bx", p.m_box.corner_length, 4.0f, 30.0f, "%.0f" );
			}

			zui::color_picker( "visible##bx", p.m_box.visible_color );
			zui::color_picker( "occluded##bx", p.m_box.occluded_color );
		}

		zui::checkbox( "skeleton##sk", p.m_skeleton.enabled );
		if ( p.m_skeleton.enabled )
		{
			zui::slider_float( "thickness##sk", p.m_skeleton.thickness, 0.5f, 4.0f, "%.1f" );
			zui::color_picker( "visible##sk", p.m_skeleton.visible_color );
			zui::color_picker( "occluded##sk", p.m_skeleton.occluded_color );
		}

		zui::checkbox( "hitboxes##hbxs", p.m_hitboxes.enabled );
		if ( p.m_hitboxes.enabled )
		{
			zui::checkbox( "fill##hbxs", p.m_hitboxes.fill );
			zui::checkbox( "outline##hbxs", p.m_hitboxes.outline );
			zui::color_picker( "visible##hbxs", p.m_hitboxes.visible_color );
			zui::color_picker( "occluded##hbxs", p.m_hitboxes.occluded_color );
		}

		zui::checkbox( "name##nm", p.m_name.enabled );
		if ( p.m_name.enabled )
		{
			zui::color_picker( "color##nm", p.m_name.color );
		}

		zui::checkbox( "weapon##wp", p.m_weapon.enabled );
		if ( p.m_weapon.enabled )
		{
			constexpr const char* disp_types[ ]{ "text", "icon", "text + icon" };
			auto dt = static_cast< int >( p.m_weapon.display );
			if ( zui::combo( "display##wp", dt, disp_types, 3 ) )
			{
				p.m_weapon.display = static_cast< settings::esp::player::weapon::display_type >( dt );
			}
			zui::color_picker( "text color##wp", p.m_weapon.text_color );
			zui::text_colored( "weapon icons are fixed to white", zdraw::rgba{ 255, 179, 236, 235 } );
		}

		zui::end_group_box( );
	};

	auto draw_player_bars = [ & ]( float width )
	{
		if ( !zui::begin_group_box( "bars", width ) )
		{
			return;
		}

		zui::checkbox( "health bar##hb", p.m_health_bar.enabled );
		if ( p.m_health_bar.enabled )
		{
			constexpr const char* hb_positions[ ]{ "left", "top", "bottom" };
			auto hbp = static_cast< int >( p.m_health_bar.position );
			if ( zui::combo( "position##hb", hbp, hb_positions, 3 ) )
			{
				p.m_health_bar.position = static_cast< settings::esp::player::health_bar::position_type >( hbp );
			}
			zui::checkbox( "outline##hb", p.m_health_bar.outline );
			zui::checkbox( "gradient##hb", p.m_health_bar.gradient );
			zui::checkbox( "show value##hb", p.m_health_bar.show_value );
			zui::color_picker( "full##hb", p.m_health_bar.full_color );
			zui::color_picker( "low##hb", p.m_health_bar.low_color );
			zui::color_picker( "background##hb", p.m_health_bar.background_color );
			zui::color_picker( "outline color##hb", p.m_health_bar.outline_color );
			zui::color_picker( "text color##hb", p.m_health_bar.text_color );
		}

		zui::checkbox( "ammo bar##amb", p.m_ammo_bar.enabled );
		if ( p.m_ammo_bar.enabled )
		{
			constexpr const char* amb_positions[ ]{ "left", "top", "bottom" };
			auto abp = static_cast< int >( p.m_ammo_bar.position );
			if ( zui::combo( "position##amb", abp, amb_positions, 3 ) )
			{
				p.m_ammo_bar.position = static_cast< settings::esp::player::ammo_bar::position_type >( abp );
			}
			zui::checkbox( "outline##amb", p.m_ammo_bar.outline );
			zui::checkbox( "gradient##amb", p.m_ammo_bar.gradient );
			zui::checkbox( "show value##amb", p.m_ammo_bar.show_value );
			zui::color_picker( "full##amb", p.m_ammo_bar.full_color );
			zui::color_picker( "low##amb", p.m_ammo_bar.low_color );
			zui::color_picker( "background##amb", p.m_ammo_bar.background_color );
			zui::color_picker( "outline color##amb", p.m_ammo_bar.outline_color );
			zui::color_picker( "text color##amb", p.m_ammo_bar.text_color );
		}

		zui::end_group_box( );
	};

	auto draw_player_flags = [ & ]( float width )
	{
		if ( !zui::begin_group_box( "info flags", width ) )
		{
			return;
		}

		zui::checkbox( "enabled##if", p.m_info_flags.enabled );
		if ( p.m_info_flags.enabled )
		{
			constexpr const char* flag_names[ ]{ "money", "armor", "kit", "scoped", "defusing", "flashed", "ping", "distance" };
			constexpr settings::esp::player::info_flags::flag flag_values[ ]
			{
				settings::esp::player::info_flags::money,
				settings::esp::player::info_flags::armor,
				settings::esp::player::info_flags::kit,
				settings::esp::player::info_flags::scoped,
				settings::esp::player::info_flags::defusing,
				settings::esp::player::info_flags::flashed,
				settings::esp::player::info_flags::ping,
				settings::esp::player::info_flags::distance
			};

			bool selected[ 8 ]
			{
				p.m_info_flags.has( flag_values[ 0 ] ), p.m_info_flags.has( flag_values[ 1 ] ),
				p.m_info_flags.has( flag_values[ 2 ] ), p.m_info_flags.has( flag_values[ 3 ] ),
				p.m_info_flags.has( flag_values[ 4 ] ), p.m_info_flags.has( flag_values[ 5 ] ),
				p.m_info_flags.has( flag_values[ 6 ] ), p.m_info_flags.has( flag_values[ 7 ] )
			};

			zui::multicombo( "flags##if", selected, flag_names, 8 );

			p.m_info_flags.flags = settings::esp::player::info_flags::none;
			for ( int i = 0; i < 8; ++i )
			{
				if ( selected[ i ] )
				{
					p.m_info_flags.flags |= flag_values[ i ];
				}
			}

			zui::color_picker( "money##if", p.m_info_flags.money_color );
			zui::color_picker( "armor##if", p.m_info_flags.armor_color );
			zui::color_picker( "kit##if", p.m_info_flags.kit_color );
			zui::color_picker( "scoped##if", p.m_info_flags.scoped_color );
			zui::color_picker( "defusing##if", p.m_info_flags.defusing_color );
			zui::color_picker( "flashed##if", p.m_info_flags.flashed_color );
			zui::color_picker( "distance##if", p.m_info_flags.distance_color );
		}

		zui::end_group_box( );
	};

	auto draw_items = [ & ]( float width )
	{
		if ( !zui::begin_group_box( "items", width ) )
		{
			return;
		}

		zui::checkbox( "enabled##it", it.enabled );
		if ( it.enabled )
		{
			zui::slider_float( "max dist##it", it.max_distance, 5.0f, 150.0f, "%.0fm" );

			zui::checkbox( "icon##it", it.m_icon.enabled );
			if ( it.m_icon.enabled )
			{
				zui::text_colored( "item icons are fixed to white", zdraw::rgba{ 205, 214, 245, 235 } );
			}

			zui::checkbox( "name##it", it.m_name.enabled );
			if ( it.m_name.enabled )
			{
				zui::color_picker( "color##it_name", it.m_name.color );
			}

			zui::checkbox( "ammo##it", it.m_ammo.enabled );
			if ( it.m_ammo.enabled )
			{
				zui::color_picker( "color##it_ammo", it.m_ammo.color );
				zui::color_picker( "empty##it_ammo", it.m_ammo.empty_color );
			}
		}

		auto& f = it.m_filters;
		constexpr const char* filter_items[ ]{ "rifles", "smgs", "shotguns", "snipers", "pistols", "heavy", "grenades", "utility" };
		bool filter_selected[ 8 ]{ f.rifles, f.smgs, f.shotguns, f.snipers, f.pistols, f.heavy, f.grenades, f.utility };

		zui::multicombo( "filters##f", filter_selected, filter_items, 8 );

		f.rifles = filter_selected[ 0 ];
		f.smgs = filter_selected[ 1 ];
		f.shotguns = filter_selected[ 2 ];
		f.snipers = filter_selected[ 3 ];
		f.pistols = filter_selected[ 4 ];
		f.heavy = filter_selected[ 5 ];
		f.grenades = filter_selected[ 6 ];
		f.utility = filter_selected[ 7 ];

		zui::end_group_box( );
	};

	auto draw_projectiles = [ & ]( float width )
	{
		if ( !zui::begin_group_box( "projectiles", width ) )
		{
			return;
		}

		zui::checkbox( "enabled##pr", pr.enabled );
		if ( pr.enabled )
		{
			zui::checkbox( "icon##pr", pr.show_icon );
			zui::checkbox( "name##pr", pr.show_name );
			zui::checkbox( "timer bar##pr", pr.show_timer_bar );
			zui::checkbox( "inferno bounds##pr", pr.show_inferno_bounds );

			zui::color_picker( "default##pr", pr.default_color );
			zui::color_picker( "he##pr", pr.color_he );
			zui::color_picker( "flash##pr", pr.color_flash );
			zui::color_picker( "smoke##pr", pr.color_smoke );
			zui::color_picker( "molotov##pr", pr.color_molotov );
			zui::color_picker( "decoy##pr", pr.color_decoy );
			zui::color_picker( "timer high##pr", pr.timer_high_color );
			zui::color_picker( "timer low##pr", pr.timer_low_color );
			zui::color_picker( "bar background##pr", pr.bar_background );
		}

		zui::end_group_box( );
	};

	if ( this->m_esp_panel == esp_panel::player )
	{
		const auto col_w = ( body_w - 8.0f ) * 0.5f;
		draw_player_core( col_w );
		zui::same_line( );
		draw_player_bars( col_w );
		zui::new_line( );
		draw_player_flags( body_w );
	}
	else if ( this->m_esp_panel == esp_panel::items )
	{
		draw_items( body_w );
	}
	else
	{
		draw_projectiles( body_w );
	}
}

void menu::draw_misc( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;

	if ( zui::begin_group_box( "grenade prediction", col_w ) )
	{
		zui::checkbox( "enabled##gr", settings::g_misc.m_grenades.enabled );
		if ( settings::g_misc.m_grenades.enabled )
		{
			zui::checkbox( "local only##gr", settings::g_misc.m_grenades.local_only );
			zui::color_picker( "trajectory color##gr", settings::g_misc.m_grenades.color );
		}

		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "performance", col_w ) )
	{
		zui::checkbox( "spectator warning##perf", settings::g_misc.spectator_warning );
		zui::checkbox( "watermark##perf", settings::g_misc.watermark );
		zui::checkbox( "perf overlay##perf", settings::g_misc.perf_overlay );
		zui::checkbox( "limit fps##fps", settings::g_misc.limit_fps );
		zui::slider_int( "fps cap##fps", settings::g_misc.fps_limit, 60, 1000 );

		if ( zui::button( "unload", col_w - 16.0f, 28.0f ) )
		{
			threads::request_stop( );
			if ( g::render.hwnd( ) )
			{
				::PostMessageW( g::render.hwnd( ), WM_CLOSE, 0, 0 );
			}

			zui::end_group_box( );
			return;
		}

		zui::end_group_box( );
	}
}

void menu::draw_config( )
{
	const auto [avail_w, avail_h] = zui::get_content_region_avail( );
	const auto col_w = ( avail_w - 8.0f ) * 0.5f;

	auto& cfg = g::config;
	const auto& names = cfg.names( );

	if ( !names.empty( ) )
	{
		this->m_cfg_selected = std::clamp( this->m_cfg_selected, 0, static_cast<int>( names.size( ) - 1 ) );
	}
	else
	{
		this->m_cfg_selected = 0;
	}

	if ( zui::begin_group_box( "profiles", col_w ) )
	{
		zui::text_input( "name##cfg", this->m_cfg_name, 64, "profile name" );

		if ( names.empty( ) )
		{
			zui::text_colored( "no configs yet", zdraw::rgba{ 170, 150, 182, 255 } );
		}
		else
		{
			std::vector<const char*> items{};
			items.reserve( names.size( ) );
			for ( const auto& n : names )
			{
				items.push_back( n.c_str( ) );
			}

			zui::combo( "list##cfg", this->m_cfg_selected, items.data( ), static_cast<int>( items.size( ) ) );
		}

		if ( zui::button( "create", ( col_w - 24.0f ) * 0.5f, 28.0f ) )
		{
			cfg.create( this->m_cfg_name );
		}

		zui::same_line( );

		if ( zui::button( "refresh", ( col_w - 24.0f ) * 0.5f, 28.0f ) )
		{
			cfg.refresh( );
		}

		if ( !names.empty( ) )
		{
			const auto selected_name = names[ this->m_cfg_selected ];

			if ( zui::button( "load", ( col_w - 24.0f ) * 0.5f, 28.0f ) )
			{
				cfg.load( selected_name );
			}

			zui::same_line( );

			if ( zui::button( "delete", ( col_w - 24.0f ) * 0.5f, 28.0f ) )
			{
				cfg.remove( selected_name );
			}
		}

		zui::end_group_box( );
	}

	zui::same_line( );

	if ( zui::begin_group_box( "state", col_w ) )
	{
		const auto active = cfg.active( ).empty( ) ? std::string{ "none" } : cfg.active( );
		zui::text_colored( std::format( "active: {}", active ), zdraw::rgba{ 214, 194, 225, 255 } );
		zui::text_colored( "auto-save: enabled", zdraw::rgba{ 170, 150, 182, 255 } );

		if ( !cfg.active( ).empty( ) )
		{
			if ( zui::button( "save now", col_w - 16.0f, 28.0f ) )
			{
				cfg.save( cfg.active( ) );
			}
		}

		if ( cfg.has_error( ) )
		{
			zui::text_colored( std::format( "error: {}", cfg.last_error( ) ), zdraw::rgba{ 230, 130, 150, 255 } );
		}
		else
		{
			zui::text_colored( "status: ready", zdraw::rgba{ 151, 214, 168, 255 } );
		}

		zui::end_group_box( );
	}
}
