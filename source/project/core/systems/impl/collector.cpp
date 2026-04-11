#include <stdafx.hpp>
#include <utils/logger/logger.hpp>

namespace systems {

	namespace {
		[[nodiscard]] std::uint32_t read_player_pawn_handle( std::uintptr_t controller )
		{
			const auto primary = SCHEMA( "CCSPlayerController", "m_hPlayerPawn"_hash );
			if ( primary )
			{
				const auto handle = g::memory.read<std::uint32_t>( controller + primary );
				if ( handle && handle != 0xFFFFFFFF )
				{
					return handle;
				}
			}

			const auto fallback = SCHEMA( "CCSPlayerController", "m_hPawn"_hash );
			if ( fallback )
			{
				const auto handle = g::memory.read<std::uint32_t>( controller + fallback );
				if ( handle && handle != 0xFFFFFFFF )
				{
					return handle;
				}
			}

			return 0;
		}
	}

	void collector::run( )
	{
		static std::uint32_t s_run_log_counter = 0;
		const auto raw = systems::g_entities.all( );

		const auto needs_visibility = settings::g_esp.m_player.m_box.enabled || settings::g_esp.m_player.m_skeleton.enabled || settings::g_esp.m_player.m_hitboxes.enabled;
		auto needs_hitboxes = settings::g_esp.m_player.m_hitboxes.enabled;
		if ( !needs_hitboxes )
		{
			for ( const auto& group : settings::g_combat.groups )
			{
				if ( group.aimbot.enabled || group.triggerbot.enabled )
				{
					needs_hitboxes = true;
					break;
				}
			}
		}

		this->collect_players( *raw, needs_visibility, needs_hitboxes );
		this->collect_items( *raw );
		this->collect_projectiles( *raw );

		auto players = this->players( );
		auto items = this->items( );
		auto projectiles = this->projectiles( );
		{
			++s_run_log_counter;
			if ( ( s_run_log_counter % 120 ) == 0 || !players->empty( ) )
			{
				LOG( logger::category::collector, "Collected: players=%d, items=%d, projectiles=%d, visibility_check=%s, hitboxes=%s",
					(int)players->size( ), (int)items->size( ), (int)projectiles->size( ),
					needs_visibility ? "yes" : "no", needs_hitboxes ? "yes" : "no" );
			}
		}
	}

	std::shared_ptr<const std::vector<collector::player>> collector::players( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_players;
	}

	std::shared_ptr<const std::vector<collector::item>> collector::items( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_items;
	}

	std::shared_ptr<const std::vector<collector::projectile>> collector::projectiles( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_projectiles;
	}

	void collector::collect_players( const std::vector<entities::cached>& raw, bool needs_visibility, bool needs_hitboxes )
	{
		const auto view_origin = systems::g_view.origin( );
		const auto local_pawn = systems::g_local.view_pawn( );
		const auto needs_info = settings::g_esp.m_player.m_info_flags.enabled;

		int player_count = 0;
		for ( const auto& e : raw )
			if ( e.type == entities::type::player ) ++player_count;

		std::vector<player> fresh{};
		fresh.reserve( player_count );
		int skipped_no_pawn_handle = 0;
		int skipped_no_pawn = 0;
		int skipped_local = 0;
		int skipped_scene_node = 0;
		int skipped_distance = 0;
		int skipped_dead = 0;

		for ( const auto& entry : raw )
		{
			if ( entry.type != entities::type::player )
				continue;

			const auto pawn_handle = read_player_pawn_handle( entry.ptr );
			if ( !pawn_handle )
			{
				++skipped_no_pawn_handle;
				continue;
			}

			const auto pawn = systems::g_entities.lookup( pawn_handle );
			if ( !pawn )
			{
				++skipped_no_pawn;
				continue;
			}

			if ( pawn == local_pawn )
			{
				++skipped_local;
				continue;
			}

			const auto scene_node = g::memory.read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
			if ( !scene_node )
			{
				++skipped_scene_node;
				continue;
			}

			const auto origin = g::memory.read<math::vector3>( scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
			const auto distance_m = view_origin.distance( origin ) * constants::esp::k_distance_scaling;
			if ( distance_m > constants::esp::k_max_esp_distance )
			{
				++skipped_distance;
				continue;
			}

			const auto health = g::memory.read<std::int32_t>( pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
			if ( health <= 0 )
			{
				++skipped_dead;
				continue;
			}

			const auto team = g::memory.read<std::int32_t>( pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );

			player p{};
			p.controller = entry.ptr;
			p.pawn = pawn;
			p.game_scene_node = scene_node;
			p.origin = origin;
			p.health = health;
			p.team = team;
			p.is_visible = true;

			p.bone_cache = g::memory.read<std::uintptr_t>( scene_node + SCHEMA( "CSkeletonInstance", "m_modelState"_hash ) + 0x80 );

			if ( needs_visibility && p.bone_cache && p.bone_cache >= 0x10000 )
			{
				const auto head = systems::g_bones.get_head_position( p.bone_cache );
				if ( head.has_value( ) )
					p.is_visible = !systems::g_bvh.trace_ray( view_origin, head.value( ) ).hit;
			}

			if ( needs_hitboxes && p.bone_cache && p.bone_cache >= 0x10000 && distance_m < constants::esp::k_max_bone_distance )
				p.hitboxes = systems::g_hitboxes.query( scene_node );

			p.armor = g::memory.read<std::int32_t>( pawn + SCHEMA( "C_CSPlayerPawn", "m_ArmorValue"_hash ) );
			p.is_scoped = g::memory.read<bool>( pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsScoped"_hash ) );
			p.is_defusing = g::memory.read<bool>( pawn + SCHEMA( "C_CSPlayerPawn", "m_bIsDefusing"_hash ) );
			p.is_flashed = g::memory.read<float>( pawn + SCHEMA( "C_BasePlayerPawnBase", "m_flFlashBangTime"_hash ) ) > 0.0f;
			p.invulnerable = g::memory.read<bool>( pawn + SCHEMA( "C_CSPlayerPawn", "m_bGunGameImmunity"_hash ) );

			if ( needs_info )
			{
				p.ping = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );

				const auto item_services = g::memory.read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pItemServices"_hash ) );
				if ( item_services )
				{
					p.has_helmet = g::memory.read<bool>( item_services + SCHEMA( "CCSPlayer_ItemServices", "m_bHasHelmet"_hash ) );
					p.has_defuser = g::memory.read<bool>( item_services + SCHEMA( "CCSPlayer_ItemServices", "m_bHasDefuser"_hash ) );
				}

				const auto money_services = g::memory.read<std::uintptr_t>( entry.ptr + SCHEMA( "CCSPlayerController", "m_pInGameMoneyServices"_hash ) );
				if ( money_services )
					p.money = g::memory.read<std::int32_t>( money_services + SCHEMA( "CCSPlayerController_InGameMoneyServices", "m_iAccount"_hash ) );
			}

			const auto weapon_services = g::memory.read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
			if ( weapon_services )
			{
				const auto weapon_handle = g::memory.read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
				if ( weapon_handle && weapon_handle != 0xFFFFFFFF )
				{
					p.weapon.ptr = systems::g_entities.lookup( weapon_handle );
					if ( p.weapon.ptr )
					{
						p.weapon.vdata = g::memory.read<std::uintptr_t>( p.weapon.ptr + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
						if ( p.weapon.vdata )
						{
							p.weapon.ammo = g::memory.read<std::int32_t>( p.weapon.ptr + SCHEMA( "C_BasePlayerWeapon", "m_iClip1"_hash ) );
							p.weapon.max_ammo = g::memory.read<std::int32_t>( p.weapon.vdata + SCHEMA( "CBasePlayerWeaponVData", "m_iMaxClip1"_hash ) );

							const auto name_ptr = g::memory.read<std::uintptr_t>( p.weapon.vdata + SCHEMA( "CCSWeaponBaseVData", "m_szName"_hash ) );
							if ( name_ptr )
							{
								p.weapon.name = g::memory.read_string( name_ptr, 64 );
								if ( p.weapon.name.starts_with( "weapon_" ) )
									p.weapon.name.erase( 0, 7 );
							}
						}
					}
				}
			}

			if ( needs_info )
			{
				const auto name_ptr = g::memory.read<std::uintptr_t>( entry.ptr + SCHEMA( "CCSPlayerController", "m_sSanitizedPlayerName"_hash ) );
				if ( name_ptr )
				{
					p.display_name = g::memory.read_string( name_ptr, 128 );
					std::ranges::transform( p.display_name, p.display_name.begin( ), []( unsigned char c ) { return std::tolower( c ); } );
				}
			}

			fresh.push_back( std::move( p ) );
		}

		std::ranges::sort( fresh, [&view_origin]( const player& a, const player& b ) { return view_origin.distance( a.origin ) > view_origin.distance( b.origin ); } );

		std::unique_lock lock( this->m_mutex );
		this->m_players = std::make_shared<const std::vector<player>>( std::move( fresh ) );
		static std::uint32_t s_pipeline_log_counter = 0;
		++s_pipeline_log_counter;
		if ( ( s_pipeline_log_counter % 120 ) == 0 || !this->m_players->empty( ) )
		{
			LOG( logger::category::collector, "Players pipeline: raw=%d, kept=%d, no_pawn_handle=%d, bad_pawn=%d, local=%d, no_scene=%d, far=%d, dead=%d",
				player_count, (int)this->m_players->size( ), skipped_no_pawn_handle, skipped_no_pawn, skipped_local, skipped_scene_node, skipped_distance, skipped_dead );
		}
	}

	void collector::collect_items( const std::vector<entities::cached>& raw )
	{
		const auto view_origin = systems::g_view.origin( );
		const auto max_dist = settings::g_esp.m_item.max_distance;

		int item_count = 0;
		for ( const auto& e : raw )
			if ( e.type == entities::type::item ) ++item_count;

		std::vector<item> fresh{};
		fresh.reserve( item_count );

		for ( const auto& entry : raw )
		{
			if ( entry.type != entities::type::item )
				continue;

			const auto subtype = classify_item( entry.schema_hash );
			if ( subtype == item_subtype::unknown )
				continue;

			const auto owner = g::memory.read<std::uint32_t>( entry.ptr + SCHEMA( "C_BaseEntity", "m_hOwnerEntity"_hash ) );
			if ( owner && owner != 0xffffffff )
				continue;

			const auto scene_node = g::memory.read<std::uintptr_t>( entry.ptr + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
			if ( !scene_node )
				continue;

			const auto origin = g::memory.read<math::vector3>( scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
			const auto distance_m = view_origin.distance( origin ) * constants::esp::k_distance_scaling;
			if ( distance_m > max_dist )
				continue;

			item i{};
			i.entity = entry.ptr;
			i.game_scene_node = scene_node;
			i.subtype = subtype;
			i.origin = origin;
			i.ammo = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "C_BasePlayerWeapon", "m_iClip1"_hash ) );

			const auto vdata = g::memory.read<std::uintptr_t>( entry.ptr + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
			if ( vdata )
				i.max_ammo = g::memory.read<std::int32_t>( vdata + SCHEMA( "CBasePlayerWeaponVData", "m_iMaxClip1"_hash ) );

			fresh.push_back( std::move( i ) );
		}

		std::ranges::sort( fresh, [&view_origin]( const item& a, const item& b ) { return view_origin.distance( a.origin ) > view_origin.distance( b.origin ); } );

		std::unique_lock lock( this->m_mutex );
		this->m_items = std::make_shared<const std::vector<item>>( std::move( fresh ) );
	}

	void collector::collect_projectiles( const std::vector<entities::cached>& raw )
	{
		const auto view_origin = systems::g_view.origin( );
		constexpr auto max_dist = constants::esp::k_max_esp_distance;

		int proj_count = 0;
		for ( const auto& e : raw )
			if ( e.type == entities::type::projectile ) ++proj_count;

		std::vector<projectile> fresh{};
		fresh.reserve( proj_count );

		for ( const auto& entry : raw )
		{
			if ( entry.type != entities::type::projectile )
				continue;

			const auto subtype = classify_projectile( entry.schema_hash );
			if ( subtype == projectile_subtype::unknown )
				continue;

			if ( subtype == projectile_subtype::molotov_fire )
			{
				const auto fire_count = g::memory.read<int>( entry.ptr + SCHEMA( "C_Inferno", "m_fireCount"_hash ) );
				if ( fire_count <= 0 )
					continue;

				const auto fire_pos_base = entry.ptr + SCHEMA( "C_Inferno", "m_firePositions"_hash );
				const auto fire_act_base = entry.ptr + SCHEMA( "C_Inferno", "m_bFireIsBurning"_hash );

				std::vector<math::vector3> fire_points{};
				fire_points.reserve( std::min( fire_count, 64 ) );

				for ( int i = 0; i < std::min( fire_count, 64 ); ++i )
				{
					if ( !g::memory.read<bool>( fire_act_base + i ) )
						continue;
					fire_points.push_back( g::memory.read<math::vector3>( fire_pos_base + i * sizeof( math::vector3 ) ) );
				}

				if ( fire_points.empty( ) )
					continue;

				auto center = math::vector3{};
				for ( const auto& pt : fire_points )
					center = center + pt;
				center = center * ( 1.0f / static_cast< float >( fire_points.size( ) ) );

				const auto center_distance_m = view_origin.distance( center ) * constants::esp::k_distance_scaling;
				if ( center_distance_m > max_dist )
					continue;

				const auto effect_tick = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "C_Inferno", "m_nFireEffectTickBegin"_hash ) );
				constexpr auto inferno_duration = 7.0f;

				projectile p{};
				p.entity = entry.ptr;
				p.subtype = subtype;
				p.origin = center;
				p.fire_points = std::move( fire_points );
				p.expire_time = static_cast< float >( effect_tick ) * ( 1.0f / 64.0f ) + inferno_duration;

				fresh.push_back( std::move( p ) );
				continue;
			}

			const auto scene_node = g::memory.read<std::uintptr_t>( entry.ptr + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
			if ( !scene_node )
				continue;

			const auto origin = g::memory.read<math::vector3>( scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) );
			const auto distance_m = view_origin.distance( origin ) * constants::esp::k_distance_scaling;
			if ( distance_m > max_dist )
				continue;

			projectile p{};
			p.entity = entry.ptr;
			p.game_scene_node = scene_node;
			p.subtype = subtype;
			p.origin = origin;
			p.velocity = g::memory.read<math::vector3>( entry.ptr + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			p.thrower_handle = g::memory.read<std::uint32_t>( entry.ptr + SCHEMA( "C_BaseGrenade", "m_hThrower"_hash ) );
			p.bounces = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "C_BaseCSGrenadeProjectile", "m_nBounces"_hash ) );

			if ( subtype == projectile_subtype::he_grenade || subtype == projectile_subtype::flashbang )
			{
				const auto detonate_tick = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "C_BaseCSGrenadeProjectile", "m_nExplodeEffectTickBegin"_hash ) );
				p.detonated = detonate_tick > 0;
			}
			else if ( subtype == projectile_subtype::smoke_grenade )
			{
				p.effect_tick_begin = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "C_SmokeGrenadeProjectile", "m_nSmokeEffectTickBegin"_hash ) );
				p.smoke_active = g::memory.read<bool>( entry.ptr + SCHEMA( "C_SmokeGrenadeProjectile", "m_bDidSmokeEffect"_hash ) );
			}
			else if ( subtype == projectile_subtype::decoy )
			{
				p.effect_tick_begin = g::memory.read<std::int32_t>( entry.ptr + SCHEMA( "C_DecoyProjectile", "m_nDecoyShotTick"_hash ) );
			}

			fresh.push_back( std::move( p ) );
		}

		std::ranges::sort( fresh, [&view_origin]( const projectile& a, const projectile& b ) { return view_origin.distance( a.origin ) > view_origin.distance( b.origin ); } );

		std::unique_lock lock( this->m_mutex );
		this->m_projectiles = std::make_shared<const std::vector<projectile>>( std::move( fresh ) );
	}

	collector::item_subtype collector::classify_item( std::uint32_t schema_hash )
	{
		switch ( schema_hash )
		{
		case "C_AK47"_hash:                return item_subtype::ak47;
		case "C_WeaponM4A1"_hash:          return item_subtype::m4a4;
		case "C_WeaponM4A1Silencer"_hash:  return item_subtype::m4a1s;
		case "C_WeaponAWP"_hash:           return item_subtype::awp;
		case "C_WeaponAug"_hash:           return item_subtype::aug;
		case "C_WeaponFamas"_hash:         return item_subtype::famas;
		case "C_WeaponGalilAR"_hash:       return item_subtype::galil_ar;
		case "C_WeaponSG556"_hash:         return item_subtype::sg553;
		case "C_WeaponG3SG1"_hash:         return item_subtype::g3sg1;
		case "C_WeaponSCAR20"_hash:        return item_subtype::scar20;
		case "C_WeaponSSG08"_hash:         return item_subtype::ssg08;
		case "C_WeaponMAC10"_hash:         return item_subtype::mac10;
		case "C_WeaponMP5SD"_hash:         return item_subtype::mp5sd;
		case "C_WeaponMP7"_hash:           return item_subtype::mp7;
		case "C_WeaponMP9"_hash:           return item_subtype::mp9;
		case "C_WeaponBizon"_hash:         return item_subtype::pp_bizon;
		case "C_WeaponP90"_hash:           return item_subtype::p90;
		case "C_WeaponUMP45"_hash:         return item_subtype::ump45;
		case "C_WeaponNOVA"_hash:          return item_subtype::nova;
		case "C_WeaponSawedoff"_hash:      return item_subtype::sawed_off;
		case "C_WeaponXM1014"_hash:        return item_subtype::xm1014;
		case "C_WeaponMag7"_hash:          return item_subtype::mag7;
		case "C_WeaponM249"_hash:          return item_subtype::m249;
		case "C_WeaponNegev"_hash:         return item_subtype::negev;
		case "C_DEagle"_hash:              return item_subtype::deagle;
		case "C_WeaponElite"_hash:         return item_subtype::dual_berettas;
		case "C_WeaponFiveSeven"_hash:     return item_subtype::five_seven;
		case "C_WeaponGlock"_hash:         return item_subtype::glock;
		case "C_WeaponHKP2000"_hash:       return item_subtype::p2000;
		case "C_WeaponUSPSilencer"_hash:   return item_subtype::usps;
		case "C_WeaponP250"_hash:          return item_subtype::p250;
		case "C_WeaponCZ75a"_hash:         return item_subtype::cz75;
		case "C_WeaponTec9"_hash:          return item_subtype::tec9;
		case "C_WeaponRevolver"_hash:      return item_subtype::r8_revolver;
		case "C_WeaponTaser"_hash:         return item_subtype::taser;
		case "C_Knife"_hash:               return item_subtype::knife;
		case "C_C4"_hash:                  return item_subtype::c4;
		case "C_Item_Healthshot"_hash:     return item_subtype::healthshot;
		case "C_HEGrenade"_hash:           return item_subtype::he_grenade;
		case "C_Flashbang"_hash:           return item_subtype::flashbang;
		case "C_SmokeGrenade"_hash:        return item_subtype::smoke_grenade;
		case "C_MolotovGrenade"_hash:      return item_subtype::molotov;
		case "C_IncendiaryGrenade"_hash:   return item_subtype::incendiary;
		case "C_DecoyGrenade"_hash:        return item_subtype::decoy;
		default:                           return item_subtype::unknown;
		}
	}

	collector::projectile_subtype collector::classify_projectile( std::uint32_t schema_hash )
	{
		switch ( schema_hash )
		{
		case "C_HEGrenadeProjectile"_hash:    return projectile_subtype::he_grenade;
		case "C_FlashbangProjectile"_hash:    return projectile_subtype::flashbang;
		case "C_SmokeGrenadeProjectile"_hash: return projectile_subtype::smoke_grenade;
		case "C_MolotovProjectile"_hash:      return projectile_subtype::molotov;
		case "C_Inferno"_hash:                return projectile_subtype::molotov_fire;
		case "C_DecoyProjectile"_hash:        return projectile_subtype::decoy;
		default:                              return projectile_subtype::unknown;
		}
	}

} // namespace systems
