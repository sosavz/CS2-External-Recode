#include <stdafx.hpp>

namespace features::combat {

	void legit::on_render( zdraw::draw_list& draw_list )
	{
		const auto eye_pos = systems::g_view.origin( );
		const auto view_angles = systems::g_view.angles( );

		const auto& ctx = g_shared.ctx( );
		const auto& cfg = settings::g_combat.get( ctx.weapon_type );

		if ( !ctx.valid )
		{
			return;
		}

		const auto valid_weapon = cstypes::is_weapon_valid( ctx.weapon_type );

		this->m_fov_alpha.set_target( valid_weapon && cfg.aimbot.draw_fov && cfg.aimbot.enabled ? 1.0f : 0.0f );
		this->m_fov_alpha.update( );

		if ( this->m_fov_alpha.value( ) <= 0.01f )
		{
			return;
		}

		this->draw_fov( draw_list, eye_pos, view_angles, cfg.aimbot );
	}

	void legit::tick( )
	{
		if ( !this->m_rng_seeded )
		{
			this->m_rng.seed( static_cast< int >( std::chrono::steady_clock::now( ).time_since_epoch( ).count( ) & 0x7fffffff ) );
			this->m_rng_seeded = true;
		}

		if ( this->m_trigger_held )
		{
			const auto& ctx = g_shared.ctx( );
			if ( !ctx.valid || ctx.current_time >= this->m_trigger_release_time )
			{
				g::input.inject_mouse( 0, 0, input::left_up );
				this->m_trigger_held = false;
			}
		}

		const auto& ctx = g_shared.ctx( );
		if ( !ctx.valid )
		{
			return;
		}

		const auto valid_weapon = cstypes::is_weapon_valid( ctx.weapon_type );
		const auto& cfg = settings::g_combat.get( ctx.weapon_type );

		if ( !valid_weapon )
		{
			return;
		}

		const auto eye_pos = systems::g_view.origin( );
		const auto view_angles = systems::g_view.angles( );
		const auto players = systems::g_collector.players( );

		if ( !ctx.is_reloading && ctx.weapon_ready )
		{
			if ( cfg.aimbot.enabled )
			{
				const auto target = this->select_target( eye_pos, view_angles, players, cfg );
				if ( target.player )
				{
					this->aimbot( eye_pos, view_angles, target, cfg.aimbot );
				}
			}

			if ( cfg.triggerbot.enabled )
			{
				this->triggerbot( eye_pos, view_angles, players, cfg.triggerbot );
			}
		}
	}

	legit::target legit::select_target( const math::vector3& eye_pos, const math::vector3& view_angles, const std::vector<systems::collector::player>& players, const settings::combat::group_config& cfg ) const
	{
		target best{};
		best.fov = static_cast< float >( cfg.aimbot.fov );

		for ( const auto& player : players )
		{
			if ( !systems::g_local.is_enemy( player.team ) )
			{
				continue;
			}

			if ( player.invulnerable || player.hitboxes.count <= 0 )
			{
				continue;
			}

			const auto bones = systems::g_bones.get( player.bone_cache );
			if ( !bones.is_valid( ) )
			{
				continue;
			}

			auto damage{ 0.0f };
			auto hitbox{ -1 };
			auto penetrated{ false };

			const auto aim_point = this->get_aim_point( eye_pos, player, bones, cfg, damage, hitbox, penetrated );
			if ( hitbox < 0 )
			{
				continue;
			}

			const auto fov = this->get_fov( view_angles, eye_pos, aim_point );
			if ( fov > best.fov )
			{
				continue;
			}

			best.player = &player;
			best.bones = bones;
			best.aim_point = aim_point;
			best.hitbox = hitbox;
			best.damage = damage;
			best.fov = fov;
			best.penetrated = penetrated;
		}

		return best;
	}

	math::vector3 legit::get_aim_point( const math::vector3& eye_pos, const systems::collector::player& player, const systems::bones::data& bones, const settings::combat::group_config& cfg, float& out_damage, int& out_hitbox, bool& out_penetrated ) const
	{
		out_hitbox = -1;

		for ( const auto& hb : player.hitboxes )
		{
			if ( hb.index < 0 || hb.bone < 0 )
			{
				continue;
			}

			if ( cfg.aimbot.head_only && hb.index > 1 )
			{
				continue;
			}

			const auto pos = bones.get_position( hb.bone );
			const auto hitgroup = systems::g_hitboxes.hitgroup_from_hitbox( hb.index );

			if ( !cfg.aimbot.visible_only )
			{
				out_damage = combat::g_shared.pen( ).get_max_damage( hitgroup, player.armor, player.has_helmet, player.team );
				out_hitbox = hb.index;
				out_penetrated = false;
				return pos;
			}

			const auto trace = systems::g_bvh.trace_ray( eye_pos, pos );
			const auto visible = !trace.hit || trace.fraction > 0.97f;

			if ( visible )
			{
				out_damage = combat::g_shared.pen( ).get_max_damage( hitgroup, player.armor, player.has_helmet, player.team );
				out_hitbox = hb.index;
				out_penetrated = false;
				return pos;
			}

		}

		return {};
	}

	float legit::get_fov( const math::vector3& view_angles, const math::vector3& eye_pos, const math::vector3& target_pos ) const
	{
		return math::helpers::calculate_fov( view_angles, eye_pos, target_pos );
	}

	float legit::get_fov_radius( const math::vector3& eye_pos, const math::vector3& view_angles, float fov_degrees ) const
	{
		if ( fov_degrees <= 0.0f )
		{
			return 0.0f;
		}

		math::vector3 forward{};
		view_angles.to_directions( &forward, nullptr, nullptr );

		auto offset_angles = view_angles;
		offset_angles.x -= fov_degrees;

		math::vector3 offset_forward{};
		offset_angles.to_directions( &offset_forward, nullptr, nullptr );

		const auto center = systems::g_view.project( eye_pos + forward * 1000.0f );
		const auto edge = systems::g_view.project( eye_pos + offset_forward * 1000.0f );

		if ( !systems::g_view.projection_valid( center ) || !systems::g_view.projection_valid( edge ) )
		{
			return 0.0f;
		}

		const auto dx = edge.x - center.x;
		const auto dy = edge.y - center.y;

		return std::sqrtf( dx * dx + dy * dy );
	}

	void legit::draw_fov( zdraw::draw_list& draw_list, const math::vector3& eye_pos, const math::vector3& view_angles, const settings::combat::aimbot& cfg )
	{
		const auto target_radius = this->get_fov_radius( eye_pos, view_angles, static_cast< float >( cfg.fov ) );
		const auto alpha = this->m_fov_alpha.value( );
		const auto radius = target_radius * alpha;

		if ( radius <= 0.5f )
		{
			return;
		}

		const auto [w, h] = zdraw::get_display_size( );
		const auto color = zdraw::rgba{ cfg.fov_color.r, cfg.fov_color.g, cfg.fov_color.b, static_cast< std::uint8_t >( alpha * 125.0f ) };

		draw_list.add_circle( w * 0.5f, h * 0.5f, radius, color, 16 );
	}

	void legit::aimbot( const math::vector3& eye_pos, const math::vector3& view_angles, const target& tgt, const settings::combat::aimbot& cfg )
	{
		if ( !( GetAsyncKeyState( cfg.key ) & 0x8000 ) )
		{
			this->m_aim_error = {};
			return;
		}

		constexpr auto m_yaw{ 0.022f };
		const auto sensitivity = systems::g_convars.get<float>( CONVAR( "sensitivity"_hash ) );
		const auto fov_adjust = g::memory.read<float>( systems::g_local.pawn( ) + SCHEMA( "C_BasePlayerPawn", "m_flFOVSensitivityAdjust"_hash ) );
		const auto deg_per_pixel = sensitivity * m_yaw * fov_adjust;

		if ( deg_per_pixel <= 0.0f )
		{
			return;
		}

		const auto freshest = systems::g_bones.get( tgt.player->bone_cache );
		if ( !freshest.is_valid( ) )
		{
			return;
		}

		auto aim_point = tgt.aim_point;

		for ( int i = 0; i < tgt.player->hitboxes.count; ++i )
		{
			const auto& hb = tgt.player->hitboxes.entries[ i ];
			if ( hb.index == tgt.hitbox && hb.bone >= 0 )
			{
				aim_point = freshest.get_position( hb.bone );
				break;
			}
		}

		if ( cfg.predictive )
		{
			const auto velocity = g::memory.read<math::vector3>( tgt.player->pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			const auto prediction_time = g_shared.get_prediction_time( );

			aim_point = aim_point + velocity * prediction_time;
		}

		auto desired = math::helpers::calculate_angle( eye_pos, aim_point );
		auto delta_x = desired.x - view_angles.x;
		auto delta_y = math::helpers::normalize_yaw( desired.y - view_angles.y );

		if ( cfg.smoothing > 1 )
		{
			const auto factor = static_cast< float >( cfg.smoothing );
			delta_x /= factor;
			delta_y /= factor;
		}

		const auto move_x = -delta_y / deg_per_pixel;
		const auto move_y = delta_x / deg_per_pixel;

		this->m_aim_error.x += move_x;
		this->m_aim_error.y += move_y;

		const auto dx = static_cast< int >( this->m_aim_error.x );
		const auto dy = static_cast< int >( this->m_aim_error.y );

		this->m_aim_error.x -= static_cast< float >( dx );
		this->m_aim_error.y -= static_cast< float >( dy );

		if ( dx != 0 || dy != 0 )
		{
			g::input.inject_mouse( dx, dy, input::move );
		}
	}

	legit::trigger_result legit::trace_crosshair( const math::vector3& eye_pos, const math::vector3& view_angles, const std::vector<systems::collector::player>& players, const settings::combat::triggerbot& cfg ) const
	{
		trigger_result result{};

		math::vector3 forward{};
		view_angles.to_directions( &forward, nullptr, nullptr );

		constexpr auto max_range{ 8192.0f };
		const auto end_pos = eye_pos + forward * max_range;

		const auto world_trace = systems::g_bvh.trace_ray( eye_pos, end_pos );
		auto best_dist_sq = max_range * max_range;
		const auto prediction_time = cfg.predictive ? g_shared.get_prediction_time( ) + static_cast< float >( cfg.delay ) * 0.001f : 0.0f;

		for ( const auto& player : players )
		{
			if ( !systems::g_local.is_enemy( player.team ) )
			{
				continue;
			}

			if ( player.invulnerable || player.hitboxes.count <= 0 )
			{
				continue;
			}

			const auto bones = systems::g_bones.get( player.bone_cache );
			if ( !bones.is_valid( ) )
			{
				continue;
			}

			math::vector3 velocity{};
			if ( cfg.predictive )
			{
				velocity = g::memory.read<math::vector3>( player.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			}

			for ( const auto& hb : player.hitboxes )
			{
				if ( hb.index < 0 || hb.bone < 0 )
				{
					continue;
				}

				const auto bone_pos = bones.get_position( hb.bone );
				const auto bone_rot = bones.get_rotation( hb.bone );

				const auto capsule_start = bone_pos + bone_rot.rotate_vector( hb.mins ) + velocity * prediction_time;
				const auto capsule_end = bone_pos + bone_rot.rotate_vector( hb.maxs ) + velocity * prediction_time;
				const auto radius = ( hb.radius > 0.0f ? hb.radius : 3.5f ) * 0.85f;

				if ( !g_shared.ray_hits_capsule( eye_pos, forward, capsule_start, capsule_end, radius ) )
				{
					continue;
				}

				const auto capsule_center = ( capsule_start + capsule_end ) * 0.5f;
				const auto dist_sq = ( capsule_center - eye_pos ).length_sqr( );

				if ( dist_sq >= best_dist_sq )
				{
					continue;
				}

				const auto vis_trace = systems::g_bvh.trace_ray( eye_pos, capsule_center );
				const auto visible = !vis_trace.hit || vis_trace.fraction > 0.97f;

				if ( visible )
				{
					const auto hitgroup = systems::g_hitboxes.hitgroup_from_hitbox( hb.index );
					const auto damage = combat::g_shared.pen( ).get_max_damage( hitgroup, player.armor, player.has_helmet, player.team );

					best_dist_sq = dist_sq;
					result.player = &player;
					result.bones = bones;
					result.hitbox = hb.index;
					result.hitgroup = hitgroup;
					result.damage = damage;
					result.penetrated = false;
				}
			}
		}

		return result;
	}

	void legit::triggerbot( const math::vector3& eye_pos, const math::vector3& view_angles, const std::vector<systems::collector::player>& players, const settings::combat::triggerbot& cfg )
	{
		if ( this->m_trigger_held )
		{
			return;
		}

		if ( !( GetAsyncKeyState( cfg.key ) & 0x8000 ) )
		{
			this->m_trigger_waiting = false;
			return;
		}

		const auto& ctx = g_shared.ctx( );
		if ( !ctx.weapon_ready )
		{
			this->m_trigger_waiting = false;
			return;
		}

		const auto result = this->trace_crosshair( eye_pos, view_angles, players, cfg );
		if ( !result.player )
		{
			this->m_trigger_waiting = false;
			return;
		}

		if ( !g_shared.is_sniper_accurate( ) )
		{
			return;
		}

		if ( cfg.hitchance > 0.0f )
		{
			const auto required = cfg.hitchance / 100.0f;
			const auto hc = g_shared.calculate_hitchance( eye_pos, view_angles, *result.player, result.bones );

			if ( hc < required )
			{
				this->m_trigger_waiting = false;
				return;
			}
		}

		const auto now = ctx.current_time;

		if ( !this->m_trigger_waiting )
		{
			this->m_trigger_waiting = true;
			this->m_trigger_delay_end = now + static_cast< float >( cfg.delay ) * 0.001f;
			return;
		}

		if ( now < this->m_trigger_delay_end )
		{
			return;
		}

		this->m_trigger_waiting = false;

		const auto hold_ms = this->m_rng.random_float( 50.0f, 120.0f );

		g::input.inject_mouse( 0, 0, input::left_down );
		this->m_trigger_held = true;
		this->m_trigger_release_time = now + hold_ms * 0.001f;
	}

} // namespace features::combat
