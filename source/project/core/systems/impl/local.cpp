#include <stdafx.hpp>

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

	void local::update( )
	{
		state next{};
		const auto local_controller = g::memory.read<std::uintptr_t>( g::offsets.local_player_controller );
		if ( !local_controller )
		{
			this->reset( );
			return;
		}
		next.controller = local_controller;

		const auto player_pawn_handle = read_player_pawn_handle( local_controller );
		if ( !player_pawn_handle )
		{
			this->reset( );
			return;
		}

		const auto player_pawn = systems::g_entities.lookup( player_pawn_handle );
		if ( !player_pawn )
		{
			this->reset( );
			return;
		}

		next.pawn = player_pawn;

		const auto team_num = g::memory.read<std::int32_t>( player_pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
		next.team = team_num;

		const auto health = g::memory.read<std::int32_t>( player_pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
		next.alive = health > 0;

		if ( next.alive )
		{
			next.view_team = team_num;

			const auto weapon_services = g::memory.read<std::uintptr_t>( player_pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) );
			if ( weapon_services )
			{
				const auto active_weapon_handle = g::memory.read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) );
				if ( active_weapon_handle && active_weapon_handle != 0xFFFFFFFF )
				{
					next.weapon = systems::g_entities.lookup( active_weapon_handle );
					if ( next.weapon )
					{
						next.weapon_vdata = g::memory.read<std::uintptr_t>( next.weapon + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 );
						if ( next.weapon_vdata )
						{
							next.weapon_type = g::memory.read<std::uint32_t>( next.weapon_vdata + SCHEMA( "CCSWeaponBaseVData", "m_WeaponType"_hash ) );
						}
					}
				}
			}
		}
		else
		{
			const auto observer_pawn_handle = g::memory.read<std::uint32_t>( local_controller + SCHEMA( "CCSPlayerController", "m_hObserverPawn"_hash ) );
			if ( observer_pawn_handle )
			{
				const auto observer_pawn = systems::g_entities.lookup( observer_pawn_handle );
				if ( observer_pawn )
				{
					const auto observer_services = g::memory.read<std::uintptr_t>( observer_pawn + SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash ) );
					if ( observer_services )
					{
						const auto observer_target_handle = g::memory.read<std::uint32_t>( observer_services + SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash ) );
						if ( observer_target_handle && observer_target_handle != 0xFFFFFFFF )
						{
							next.observer_pawn = systems::g_entities.lookup( observer_target_handle );
							if ( next.observer_pawn )
							{
								next.view_team = g::memory.read<std::int32_t>( next.observer_pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
							}
						}
					}
				}
			}
		}

		const auto game_type = systems::g_convars.get<std::int32_t>( CONVAR( "game_type"_hash ) );
		const auto game_mode = systems::g_convars.get<std::int32_t>( CONVAR( "game_mode"_hash ) );
		const auto is_ffa = ( game_type == 1 && game_mode == 2 ) || ( game_type == 2 && game_mode == 0 );

		next.team_mode = !is_ffa;

		int spectator_count = 0;
		const auto local_view_pawn = next.alive ? next.pawn : next.observer_pawn;
		if ( local_view_pawn )
		{
			const auto entity_snapshot = g_entities.all( );
			for ( const auto& entity : *entity_snapshot )
			{
				if ( entity.type != systems::entities::type::player )
					continue;

				if ( entity.ptr == local_controller )
					continue;

				const auto controller = entity.ptr;
				if ( !controller )
					continue;

				const auto pawn_handle = read_player_pawn_handle( controller );
				if ( !pawn_handle )
					continue;

				const auto pawn = systems::g_entities.lookup( pawn_handle );
				if ( !pawn )
					continue;

				const auto observer_services = g::memory.read<std::uintptr_t>( pawn + SCHEMA( "C_BasePlayerPawn", "m_pObserverServices"_hash ) );
				if ( !observer_services )
					continue;

				const auto observer_target = g::memory.read<std::uint32_t>( observer_services + SCHEMA( "CPlayer_ObserverServices", "m_hObserverTarget"_hash ) );
				if ( !observer_target || observer_target == 0xFFFFFFFF )
					continue;

				const auto observer_target_pawn = systems::g_entities.lookup( observer_target );
				if ( observer_target_pawn == local_view_pawn )
					++spectator_count;
			}
		}
		next.spectator_count = spectator_count;
		this->publish( std::move( next ) );
	}

	std::uintptr_t local::controller( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.controller;
	}

	std::uintptr_t local::pawn( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.pawn;
	}

	std::int32_t local::team( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.team;
	}

	bool local::valid( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.pawn != 0 || this->m_state.observer_pawn != 0;
	}

	bool local::alive( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.alive;
	}

	std::uintptr_t local::view_pawn( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.alive ? this->m_state.pawn : this->m_state.observer_pawn;
	}

	std::uintptr_t local::weapon( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.weapon;
	}

	std::uintptr_t local::weapon_vdata( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.weapon_vdata;
	}

	std::uint32_t local::weapon_type( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.weapon_type;
	}

	bool local::is_being_spectated( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.spectator_count > 0;
	}

	int local::spectator_count( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_state.spectator_count;
	}

	void local::publish( state next )
	{
		std::unique_lock lock( this->m_mutex );
		this->m_state = std::move( next );
	}

	void local::reset( )
	{
		this->publish( {} );
	}

} // namespace systems
