#include <stdafx.hpp>

#include <cctype>
#include <fstream>
#include <sstream>

namespace {

	constexpr int k_schema_version{ 1 };

	static std::string trim_copy( std::string s )
	{
		auto is_ws = [ ]( const unsigned char c ) { return std::isspace( c ) != 0; };

		while ( !s.empty( ) && is_ws( static_cast< unsigned char >( s.front( ) ) ) )
		{
			s.erase( s.begin( ) );
		}

		while ( !s.empty( ) && is_ws( static_cast< unsigned char >( s.back( ) ) ) )
		{
			s.pop_back( );
		}

		return s;
	}

	static void skip_ws( std::string_view text, std::size_t& pos )
	{
		while ( pos < text.size( ) && std::isspace( static_cast<unsigned char>( text[ pos ] ) ) )
		{
			++pos;
		}
	}

	static bool parse_json_string( std::string_view text, std::size_t& pos, std::string& out )
	{
		if ( pos >= text.size( ) || text[ pos ] != '"' )
		{
			return false;
		}

		++pos;
		out.clear( );

		while ( pos < text.size( ) )
		{
			const auto ch = text[ pos++ ];
			if ( ch == '"' )
			{
				return true;
			}

			if ( ch == '\\' )
			{
				if ( pos >= text.size( ) )
				{
					return false;
				}

				const auto escaped = text[ pos++ ];
				switch ( escaped )
				{
				case '"': out.push_back( '"' ); break;
				case '\\': out.push_back( '\\' ); break;
				case '/': out.push_back( '/' ); break;
				case 'b': out.push_back( '\b' ); break;
				case 'f': out.push_back( '\f' ); break;
				case 'n': out.push_back( '\n' ); break;
				case 'r': out.push_back( '\r' ); break;
				case 't': out.push_back( '\t' ); break;
				default: return false;
				}
				continue;
			}

			out.push_back( ch );
		}

		return false;
	}

	static bool parse_json_value( std::string_view text, std::size_t& pos, std::string& out )
	{
		skip_ws( text, pos );
		if ( pos >= text.size( ) )
		{
			return false;
		}

		const auto start = pos;
		if ( text[ pos ] == '[' )
		{
			int depth = 0;
			do
			{
				const auto ch = text[ pos++ ];
				if ( ch == '[' )
				{
					++depth;
				}
				else if ( ch == ']' )
				{
					--depth;
				}
			} while ( pos < text.size( ) && depth > 0 );

			if ( depth != 0 )
			{
				return false;
			}
		}
		else if ( text[ pos ] == '"' )
		{
			std::string ignored{};
			if ( !parse_json_string( text, pos, ignored ) )
			{
				return false;
			}
		}
		else
		{
			while ( pos < text.size( ) && text[ pos ] != ',' && text[ pos ] != '}' )
			{
				++pos;
			}
		}

		out = trim_copy( std::string( text.substr( start, pos - start ) ) );
		return !out.empty( );
	}

	template <typename F>
	void visit_settings( F&& f )
	{
		for ( std::size_t i = 0; i < settings::combat::k_group_count; ++i )
		{
			auto& g = settings::g_combat.groups[ i ];
			const auto base = std::format( "combat.groups.{}.", i );

			f( base + "aimbot.enabled", g.aimbot.enabled );
			f( base + "aimbot.key", g.aimbot.key );
			f( base + "aimbot.fov", g.aimbot.fov );
			f( base + "aimbot.smoothing", g.aimbot.smoothing );
			f( base + "aimbot.head_only", g.aimbot.head_only );
			f( base + "aimbot.visible_only", g.aimbot.visible_only );
			f( base + "aimbot.draw_fov", g.aimbot.draw_fov );
			f( base + "aimbot.fov_color", g.aimbot.fov_color );
			f( base + "aimbot.predictive", g.aimbot.predictive );

			f( base + "triggerbot.enabled", g.triggerbot.enabled );
			f( base + "triggerbot.key", g.triggerbot.key );
			f( base + "triggerbot.hitchance", g.triggerbot.hitchance );
			f( base + "triggerbot.delay", g.triggerbot.delay );
			f( base + "triggerbot.predictive", g.triggerbot.predictive );
		}

		auto& p = settings::g_esp.m_player;
		f( "esp.player.enabled", p.enabled );
		f( "esp.player.box.enabled", p.m_box.enabled );
		f( "esp.player.box.style", p.m_box.style );
		f( "esp.player.box.fill", p.m_box.fill );
		f( "esp.player.box.outline", p.m_box.outline );
		f( "esp.player.box.corner_length", p.m_box.corner_length );
		f( "esp.player.box.visible_color", p.m_box.visible_color );
		f( "esp.player.box.occluded_color", p.m_box.occluded_color );

		f( "esp.player.skeleton.enabled", p.m_skeleton.enabled );
		f( "esp.player.skeleton.thickness", p.m_skeleton.thickness );
		f( "esp.player.skeleton.visible_color", p.m_skeleton.visible_color );
		f( "esp.player.skeleton.occluded_color", p.m_skeleton.occluded_color );

		f( "esp.player.hitboxes.enabled", p.m_hitboxes.enabled );
		f( "esp.player.hitboxes.fill", p.m_hitboxes.fill );
		f( "esp.player.hitboxes.outline", p.m_hitboxes.outline );
		f( "esp.player.hitboxes.visible_color", p.m_hitboxes.visible_color );
		f( "esp.player.hitboxes.occluded_color", p.m_hitboxes.occluded_color );

		f( "esp.player.health.enabled", p.m_health_bar.enabled );
		f( "esp.player.health.position", p.m_health_bar.position );
		f( "esp.player.health.outline", p.m_health_bar.outline );
		f( "esp.player.health.gradient", p.m_health_bar.gradient );
		f( "esp.player.health.show_value", p.m_health_bar.show_value );
		f( "esp.player.health.full_color", p.m_health_bar.full_color );
		f( "esp.player.health.low_color", p.m_health_bar.low_color );
		f( "esp.player.health.background_color", p.m_health_bar.background_color );
		f( "esp.player.health.outline_color", p.m_health_bar.outline_color );
		f( "esp.player.health.text_color", p.m_health_bar.text_color );

		f( "esp.player.ammo.enabled", p.m_ammo_bar.enabled );
		f( "esp.player.ammo.position", p.m_ammo_bar.position );
		f( "esp.player.ammo.outline", p.m_ammo_bar.outline );
		f( "esp.player.ammo.gradient", p.m_ammo_bar.gradient );
		f( "esp.player.ammo.show_value", p.m_ammo_bar.show_value );
		f( "esp.player.ammo.full_color", p.m_ammo_bar.full_color );
		f( "esp.player.ammo.low_color", p.m_ammo_bar.low_color );
		f( "esp.player.ammo.background_color", p.m_ammo_bar.background_color );
		f( "esp.player.ammo.outline_color", p.m_ammo_bar.outline_color );
		f( "esp.player.ammo.text_color", p.m_ammo_bar.text_color );

		f( "esp.player.info.enabled", p.m_info_flags.enabled );
		f( "esp.player.info.flags", p.m_info_flags.flags );
		f( "esp.player.info.money_color", p.m_info_flags.money_color );
		f( "esp.player.info.armor_color", p.m_info_flags.armor_color );
		f( "esp.player.info.kit_color", p.m_info_flags.kit_color );
		f( "esp.player.info.scoped_color", p.m_info_flags.scoped_color );
		f( "esp.player.info.defusing_color", p.m_info_flags.defusing_color );
		f( "esp.player.info.flashed_color", p.m_info_flags.flashed_color );
		f( "esp.player.info.distance_color", p.m_info_flags.distance_color );

		f( "esp.player.name.enabled", p.m_name.enabled );
		f( "esp.player.name.color", p.m_name.color );
		f( "esp.player.weapon.enabled", p.m_weapon.enabled );
		f( "esp.player.weapon.display", p.m_weapon.display );
		f( "esp.player.weapon.text_color", p.m_weapon.text_color );
		f( "esp.player.weapon.icon_color", p.m_weapon.icon_color );

		auto& it = settings::g_esp.m_item;
		f( "esp.item.enabled", it.enabled );
		f( "esp.item.max_distance", it.max_distance );
		f( "esp.item.icon.enabled", it.m_icon.enabled );
		f( "esp.item.icon.color", it.m_icon.color );
		f( "esp.item.name.enabled", it.m_name.enabled );
		f( "esp.item.name.color", it.m_name.color );
		f( "esp.item.ammo.enabled", it.m_ammo.enabled );
		f( "esp.item.ammo.color", it.m_ammo.color );
		f( "esp.item.ammo.empty_color", it.m_ammo.empty_color );
		f( "esp.item.filters.rifles", it.m_filters.rifles );
		f( "esp.item.filters.smgs", it.m_filters.smgs );
		f( "esp.item.filters.shotguns", it.m_filters.shotguns );
		f( "esp.item.filters.snipers", it.m_filters.snipers );
		f( "esp.item.filters.pistols", it.m_filters.pistols );
		f( "esp.item.filters.heavy", it.m_filters.heavy );
		f( "esp.item.filters.grenades", it.m_filters.grenades );
		f( "esp.item.filters.utility", it.m_filters.utility );

		auto& pr = settings::g_esp.m_projectile;
		f( "esp.projectile.enabled", pr.enabled );
		f( "esp.projectile.show_icon", pr.show_icon );
		f( "esp.projectile.show_name", pr.show_name );
		f( "esp.projectile.show_timer_bar", pr.show_timer_bar );
		f( "esp.projectile.show_inferno_bounds", pr.show_inferno_bounds );
		f( "esp.projectile.default_color", pr.default_color );
		f( "esp.projectile.color_he", pr.color_he );
		f( "esp.projectile.color_flash", pr.color_flash );
		f( "esp.projectile.color_smoke", pr.color_smoke );
		f( "esp.projectile.color_molotov", pr.color_molotov );
		f( "esp.projectile.color_decoy", pr.color_decoy );
		f( "esp.projectile.timer_high_color", pr.timer_high_color );
		f( "esp.projectile.timer_low_color", pr.timer_low_color );
		f( "esp.projectile.bar_background", pr.bar_background );

		auto& m = settings::g_misc;
		f( "misc.grenades.enabled", m.m_grenades.enabled );
		f( "misc.grenades.local_only", m.m_grenades.local_only );
		f( "misc.grenades.color", m.m_grenades.color );
		f( "misc.spectator_warning", m.spectator_warning );
		f( "misc.watermark", m.watermark );
		f( "misc.limit_fps", m.limit_fps );
		f( "misc.fps_limit", m.fps_limit );
	}

	template <typename T>
	std::string encode_value( const T& value )
	{
		if constexpr ( std::is_same_v<T, bool> )
		{
			return value ? "true" : "false";
		}
		else if constexpr ( std::is_integral_v<T> || std::is_floating_point_v<T> )
		{
			return std::format( "{}", value );
		}
		else if constexpr ( std::is_enum_v<T> )
		{
			return std::format( "{}", static_cast<int>( value ) );
		}
		else
		{
			return std::format( "[{},{},{},{}]", value.r, value.g, value.b, value.a );
		}
	}

	template <typename T>
	bool decode_value( const std::string& text, T& out )
	{
		try
		{
			if constexpr ( std::is_same_v<T, bool> )
			{
				if ( text == "true" ) { out = true; return true; }
				if ( text == "false" ) { out = false; return true; }
				return false;
			}
			else if constexpr ( std::is_integral_v<T> && !std::is_same_v<T, bool> )
			{
				out = static_cast<T>( std::stoi( text ) );
				return true;
			}
			else if constexpr ( std::is_floating_point_v<T> )
			{
				out = static_cast<T>( std::stof( text ) );
				return true;
			}
			else if constexpr ( std::is_enum_v<T> )
			{
				out = static_cast<T>( std::stoi( text ) );
				return true;
			}
			else
			{
				int r{}, g{}, b{}, a{};
				if ( std::sscanf( text.c_str( ), "[%d,%d,%d,%d]", &r, &g, &b, &a ) != 4 )
				{
					return false;
				}

				out = zdraw::rgba
				{
					static_cast<std::uint8_t>( std::clamp( r, 0, 255 ) ),
					static_cast<std::uint8_t>( std::clamp( g, 0, 255 ) ),
					static_cast<std::uint8_t>( std::clamp( b, 0, 255 ) ),
					static_cast<std::uint8_t>( std::clamp( a, 0, 255 ) )
				};
				return true;
			}
		}
		catch ( ... )
		{
			return false;
		}
	}

	template <typename T>
	void hash_mix( std::uint64_t& h, const T& value )
	{
		const auto* bytes = reinterpret_cast<const std::uint8_t*>( &value );
		for ( std::size_t i = 0; i < sizeof( T ); ++i )
		{
			h ^= bytes[ i ];
			h *= 1099511628211ull;
		}
	}

} // namespace

void config_system::initialize( )
{
	if ( this->m_initialized )
	{
		return;
	}

	this->m_dir = std::filesystem::current_path( ) / "configs";

	std::error_code ec{};
	std::filesystem::create_directories( this->m_dir, ec );
	if ( ec )
	{
		this->m_last_error = std::format( "failed to create config dir: {}", ec.message( ) );
	}

	this->m_initialized = true;

	this->refresh( );
	if ( !this->m_names.empty( ) )
	{
		this->m_active = this->m_names.front( );
	}

	this->m_last_hash = this->settings_hash( );
	this->m_last_auto_save = std::chrono::steady_clock::now( );
}

void config_system::tick( )
{
	this->initialize( );

	if ( this->m_active.empty( ) )
	{
		return;
	}

	const auto now = std::chrono::steady_clock::now( );
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( now - this->m_last_auto_save ).count( );
	if ( elapsed < 700 )
	{
		return;
	}

	const auto hash = this->settings_hash( );
	if ( hash != this->m_last_hash )
	{
		if ( this->save( this->m_active ) )
		{
			this->m_last_hash = hash;
		}
	}

	this->m_last_auto_save = now;
}

bool config_system::create( std::string_view requested_name )
{
	this->initialize( );
	const auto name = this->unique_name( this->sanitize_name( requested_name ) );
	if ( name.empty( ) )
	{
		this->m_last_error = "invalid config name";
		return false;
	}

	if ( !this->save( name ) )
	{
		return false;
	}

	this->set_active( name );
	this->refresh( );
	return true;
}

bool config_system::save( std::string_view name )
{
	this->initialize( );

	const auto clean = this->sanitize_name( name );
	if ( clean.empty( ) )
	{
		this->m_last_error = "invalid config name";
		return false;
	}

	if ( !this->save_atomic( this->path_for( clean ), this->serialize_json( ) ) )
	{
		this->m_last_error = "failed to write config";
		return false;
	}

	this->m_active = clean;
	this->refresh( );
	this->m_last_error.clear( );
	return true;
}

bool config_system::load( std::string_view name )
{
	this->initialize( );
	const auto clean = this->sanitize_name( name );
	if ( clean.empty( ) )
	{
		this->m_last_error = "invalid config name";
		return false;
	}

	const auto path = this->path_for( clean );
	if ( !std::filesystem::exists( path ) )
	{
		this->m_last_error = "config file is missing";
		return false;
	}

	std::unordered_map<std::string, std::string> map{};
	if ( !this->load_with_backup( path, map ) )
	{
		this->m_last_error = "config is corrupted";
		return false;
	}

	if ( const auto it = map.find( "_schema" ); it != map.end( ) )
	{
		try
		{
			const auto schema = std::stoi( it->second );
			if ( schema > k_schema_version )
			{
				this->m_last_error = "config schema is newer than this build";
				return false;
			}
		}
		catch ( ... )
		{
			this->m_last_error = "config schema is invalid";
			return false;
		}
	}

	const auto old_combat = settings::g_combat;
	const auto old_esp = settings::g_esp;
	const auto old_misc = settings::g_misc;

	this->reset_defaults( );
	if ( !this->apply_map( map ) )
	{
		settings::g_combat = old_combat;
		settings::g_esp = old_esp;
		settings::g_misc = old_misc;
		this->m_last_error = "config has invalid values";
		return false;
	}

	this->enforce_ranges( );

	this->m_active = clean;
	this->m_last_hash = this->settings_hash( );
	this->m_last_error.clear( );
	return true;
}

bool config_system::remove( std::string_view name )
{
	this->initialize( );
	const auto clean = this->sanitize_name( name );
	if ( clean.empty( ) )
	{
		this->m_last_error = "invalid config name";
		return false;
	}

	std::error_code ec{};
	if ( !std::filesystem::remove( this->path_for( clean ), ec ) || ec )
	{
		this->m_last_error = "failed to delete config";
		return false;
	}

	if ( this->m_active == clean )
	{
		this->m_active.clear( );
	}

	this->refresh( );
	this->m_last_error.clear( );
	return true;
}

void config_system::refresh( )
{
	this->initialize( );

	this->m_names.clear( );
	std::error_code ec{};
	for ( const auto& entry : std::filesystem::directory_iterator( this->m_dir, ec ) )
	{
		if ( ec )
		{
			break;
		}

		if ( !entry.is_regular_file( ) )
		{
			continue;
		}

		if ( entry.path( ).extension( ) == ".json" )
		{
			this->m_names.push_back( entry.path( ).stem( ).string( ) );
		}
	}

	std::sort( this->m_names.begin( ), this->m_names.end( ) );
}

void config_system::set_active( std::string name )
{
	this->m_active = this->sanitize_name( name );
}

std::filesystem::path config_system::path_for( std::string_view name ) const
{
	return this->m_dir / std::format( "{}.json", name );
}

std::string config_system::sanitize_name( std::string_view raw ) const
{
	std::string out{};
	out.reserve( raw.size( ) );

	for ( const auto c : raw )
	{
		if ( std::isalnum( static_cast<unsigned char>( c ) ) || c == '_' || c == '-' )
		{
			out.push_back( static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) ) );
		}
		else if ( c == ' ' )
		{
			out.push_back( '_' );
		}
	}

	if ( out.empty( ) )
	{
		out = "default";
	}

	return out;
}

std::string config_system::unique_name( std::string base ) const
{
	if ( base.empty( ) )
	{
		base = "default";
	}

	if ( !std::filesystem::exists( this->path_for( base ) ) )
	{
		return base;
	}

	for ( int i = 1; i < 10000; ++i )
	{
		const auto candidate = std::format( "{}_{}", base, i );
		if ( !std::filesystem::exists( this->path_for( candidate ) ) )
		{
			return candidate;
		}
	}

	return {};
}

void config_system::reset_defaults( ) const
{
	settings::g_combat = settings::combat{};
	settings::g_esp = settings::esp{};
	settings::g_misc = settings::misc{};
}

std::uint64_t config_system::settings_hash( ) const
{
	const auto text = this->serialize_json( );
	std::uint64_t h{ 1469598103934665603ull };
	for ( const auto c : text )
	{
		h ^= static_cast< std::uint8_t >( c );
		h *= 1099511628211ull;
	}
	return h;
}

bool config_system::parse_json_map( const std::string& text, std::unordered_map<std::string, std::string>& out_map ) const
{
	out_map.clear( );
	std::string_view view{ text };
	std::size_t pos = 0;

	skip_ws( view, pos );
	if ( pos >= view.size( ) || view[ pos ] != '{' )
	{
		return false;
	}
	++pos;

	while ( true )
	{
		skip_ws( view, pos );
		if ( pos >= view.size( ) )
		{
			return false;
		}

		if ( view[ pos ] == '}' )
		{
			++pos;
			break;
		}

		std::string key{};
		if ( !parse_json_string( view, pos, key ) )
		{
			return false;
		}

		skip_ws( view, pos );
		if ( pos >= view.size( ) || view[ pos ] != ':' )
		{
			return false;
		}
		++pos;

		std::string value{};
		if ( !parse_json_value( view, pos, value ) || key.empty( ) )
		{
			return false;
		}

		out_map[ std::move( key ) ] = std::move( value );

		skip_ws( view, pos );
		if ( pos >= view.size( ) )
		{
			return false;
		}

		if ( view[ pos ] == ',' )
		{
			++pos;
			continue;
		}

		if ( view[ pos ] == '}' )
		{
			++pos;
			break;
		}

		return false;
	}

	skip_ws( view, pos );
	return !out_map.empty( ) && pos == view.size( );
}

std::string config_system::serialize_json( ) const
{
	std::vector<std::string> lines{};
	lines.reserve( 220 );
	lines.push_back( std::format( "  \"_schema\": {}", k_schema_version ) );

	visit_settings( [ & ]( const std::string& key, const auto& value )
		{
			lines.push_back( std::format( "  \"{}\": {}", key, encode_value( value ) ) );
		} );

	std::string out{ "{\n" };
	for ( std::size_t i = 0; i < lines.size( ); ++i )
	{
		out += lines[ i ];
		out += ( i + 1 < lines.size( ) ) ? ",\n" : "\n";
	}
	out += "}\n";

	return out;
}

bool config_system::apply_map( const std::unordered_map<std::string, std::string>& map )
{
	bool ok = true;

	visit_settings( [ & ]( const std::string& key, auto& value )
		{
			const auto it = map.find( key );
			if ( it == map.end( ) )
			{
				return;
			}

			if ( !decode_value( it->second, value ) )
			{
				ok = false;
			}
		} );

	return ok;
}

bool config_system::save_atomic( const std::filesystem::path& target, const std::string& contents )
{
	std::error_code ec{};
	const auto temp = target.string( ) + ".tmp";
	const auto backup = target.string( ) + ".bak";

	{
		std::ofstream out( temp, std::ios::trunc );
		if ( !out.is_open( ) )
		{
			return false;
		}

		out.write( contents.data( ), static_cast< std::streamsize >( contents.size( ) ) );
		if ( !out.good( ) )
		{
			return false;
		}
	}

	if ( std::filesystem::exists( target, ec ) )
	{
		ec.clear( );
		std::filesystem::copy_file( target, backup, std::filesystem::copy_options::overwrite_existing, ec );
	}

	if ( !::MoveFileExW( std::filesystem::path( temp ).c_str( ), target.c_str( ), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
	{
		std::error_code remove_ec{};
		std::filesystem::remove( temp, remove_ec );
		return false;
	}

	return true;
}

bool config_system::load_with_backup( const std::filesystem::path& target, std::unordered_map<std::string, std::string>& out_map )
{
	auto read_parse = [ & ]( const std::filesystem::path& path ) -> bool
		{
			std::ifstream in( path );
			if ( !in.is_open( ) )
			{
				return false;
			}

			std::stringstream buffer{};
			buffer << in.rdbuf( );
			if ( !this->parse_json_map( buffer.str( ), out_map ) )
			{
				return false;
			}

			return out_map.find( "misc.fps_limit" ) != out_map.end( );
		};

	if ( read_parse( target ) )
	{
		return true;
	}

	const auto backup = target.string( ) + ".bak";
	if ( !std::filesystem::exists( backup ) )
	{
		return false;
	}

	return read_parse( backup );
}

void config_system::enforce_ranges( ) const
{
	for ( auto& group : settings::g_combat.groups )
	{
		group.aimbot.fov = std::clamp( group.aimbot.fov, 1, 45 );
		group.aimbot.smoothing = std::clamp( group.aimbot.smoothing, 0, 50 );
		group.triggerbot.hitchance = std::clamp( group.triggerbot.hitchance, 0.0f, 100.0f );
		group.triggerbot.delay = std::clamp( group.triggerbot.delay, 0, 500 );
	}

	settings::g_esp.m_player.m_box.corner_length = std::clamp( settings::g_esp.m_player.m_box.corner_length, 4.0f, 30.0f );
	settings::g_esp.m_player.m_skeleton.thickness = std::clamp( settings::g_esp.m_player.m_skeleton.thickness, 0.5f, 4.0f );
	settings::g_esp.m_item.max_distance = std::clamp( settings::g_esp.m_item.max_distance, 5.0f, 150.0f );

	settings::g_misc.fps_limit = std::clamp( settings::g_misc.fps_limit, 30, 1000 );
}
