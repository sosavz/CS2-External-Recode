#include <stdafx.hpp>
#include <utils/logger/logger.hpp>

namespace systems {

	/*
	 * Entity List Structure:
	 * - CS2 maintains an array of 2048 entity slots
	 * - Each slot contains a pointer to an entity object
	 * - Entity pointers are stored in a hierarchical structure:
	 *   - First level: chunk index (slot >> 9)
	 *   - Second level: entry within chunk (slot & 0x1FF)
	 * - Each entry is 112 bytes
	 *
	 * Handle System:
	 * - Entities reference each other via handles (m_hOwnerEntity, etc.)
	 * - Handle = (chunk_index << 9) | entry_index
	 * - To lookup: extract chunk, get pointer, extract entry index, read entity
	 */

	void entities::refresh( )
	{
		++m_frame_counter;
		
		const auto entity_list = this->get_entity_list( );
		if ( !entity_list )
		{
			LOG( logger::category::entity, "Entity list is null" );
			std::unique_lock lock( this->m_mutex );
			this->m_entities = std::make_shared<const std::vector<cached>>( );
			return;
		}

		std::vector<cached> fresh{};
		fresh.reserve( 128 );

		int player_count = 0;
		int item_count = 0;
		int projectile_count = 0;
		int invalid_count = 0;
		constexpr auto chunk_count = constants::entities::k_max_entity_slots / constants::entities::k_entity_chunk_size;
		std::array<std::uintptr_t, chunk_count> chunk_entries{};

		for ( std::int32_t chunk = 0; chunk < chunk_count; ++chunk )
		{
			chunk_entries[ chunk ] = g::memory.read<std::uintptr_t>(
				entity_list + ( static_cast< std::uintptr_t >( chunk ) * 8 ) + constants::entities::k_entity_list_pointer_offset
			);
		}

		const auto force_revalidate = ( m_frame_counter % 120 ) == 0;

		for ( std::int32_t i = 0; i < constants::entities::k_max_entity_slots; ++i )
		{
			const auto chunk_index = i >> constants::entities::k_entity_chunk_index_shift;
			const auto list_entry = chunk_entries[ chunk_index ];
			if ( !list_entry )
			{
				++invalid_count;
				this->m_slot_cache[ i ] = {};
				continue;
			}

			const auto entity = g::memory.read<std::uintptr_t>(
				list_entry + ( static_cast< std::uintptr_t >( i & constants::entities::k_entity_handle_index_mask ) * constants::entities::k_entity_entry_size )
			);
			if ( !entity )
			{
				++invalid_count;
				this->m_slot_cache[ i ] = {};
				continue;
			}

			auto& slot = this->m_slot_cache[ i ];
			std::uint32_t schema_hash{};
			type entity_type{ type::unknown };

			if ( !force_revalidate && slot.ptr == entity && slot.schema_hash )
			{
				schema_hash = slot.schema_hash;
				entity_type = slot.type;
			}
			else
			{
				schema_hash = this->get_schema_hash( entity );
				if ( !schema_hash )
				{
					++invalid_count;
					slot = { .ptr = entity, .schema_hash = 0, .type = type::unknown };
					continue;
				}

				entity_type = this->classify( schema_hash );
				slot = { .ptr = entity, .schema_hash = schema_hash, .type = entity_type };
			}

			if ( entity_type == type::unknown )
			{
				++invalid_count;
				continue;
			}

			switch ( entity_type )
			{
			case type::player: ++player_count; break;
			case type::item: ++item_count; break;
			case type::projectile: ++projectile_count; break;
			default: break;
			}

			fresh.push_back( { .ptr = entity, .schema_hash = schema_hash, .index = static_cast< std::int16_t >( i ), .type = entity_type } );
		}

		const auto total_entities = static_cast< int >( fresh.size( ) );

		std::unique_lock lock( this->m_mutex );
		this->m_entities = std::make_shared<const std::vector<cached>>( std::move( fresh ) );

		if ( ( m_frame_counter % 120 ) == 0 || player_count > 1 )
		{
			LOG( logger::category::entity, "Frame %d: players=%d, items=%d, projectiles=%d, invalid=%d, total_entities=%d",
				m_frame_counter, player_count, item_count, projectile_count, invalid_count, total_entities );
		}
	}

	/*
	 * Handle Lookup:
	 * 1. Extract chunk index from handle (bits 9-14: (handle >> 9) & 0x7F)
	 * 2. Read pointer to chunk from entity_list + chunk_index * 8 + 0x10
	 * 3. Extract entry index from handle (bits 0-8: handle & 0x1FF)
	 * 4. Read entity pointer from chunk + entry_index * 112
	 * 5. Validate entity pointer (> 0x10000 to filter invalid/low addresses)
	 */
	std::uintptr_t entities::lookup( std::uint32_t handle ) const
	{
		if ( !handle || handle == 0xffffffff )
		{
			return 0;
		}

		const auto entity_list = this->get_entity_list( );
		if ( !entity_list )
		{
			return 0;
		}

		const auto list_entry = g::memory.read<std::uintptr_t>( entity_list + ( static_cast< std::uintptr_t >( ( handle & 0x7fff ) >> 9 ) * 8 ) + 0x10 );
		if ( !list_entry )
		{
			return 0;
		}

		const auto entity = g::memory.read<std::uintptr_t>( list_entry + ( static_cast< std::uintptr_t >( handle & 0x1ff ) * 112 ) );
		if ( !entity || entity < 0x10000 )
		{
			return 0;
		}

		return entity;
	}

	std::vector<entities::cached> entities::by_type( type filter ) const
	{
		std::shared_lock lock( this->m_mutex );

		std::vector<cached> result{};
		result.reserve( this->m_entities->size( ) );

		for ( const auto& entry : *this->m_entities )
		{
			if ( entry.type == filter )
			{
				result.push_back( entry );
			}
		}

		return result;
	}

	std::shared_ptr<const std::vector<entities::cached>> entities::all( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_entities;
	}

	std::uintptr_t entities::get_entity_list( ) const
	{
		return g::memory.read<std::uintptr_t>( g::offsets.entity_list );
	}

	std::uintptr_t entities::get_by_index( std::uintptr_t entity_list, std::int32_t index ) const
	{
		const auto chunk_index = index >> 9;
		const auto list_entry = g::memory.read<std::uintptr_t>( entity_list + ( static_cast< std::uintptr_t >( chunk_index ) * 8 ) + 0x10 );

		if ( !list_entry )
		{
			return 0;
		}

		return g::memory.read<std::uintptr_t>( list_entry + ( static_cast< std::uintptr_t >( index & 0x1ff ) * 112 ) );
	}

	std::uint32_t entities::get_schema_hash( std::uintptr_t entity ) const
	{
		const auto entity_identity = g::memory.read<std::uintptr_t>( entity + 0x10 );
		if ( !entity_identity )
		{
			return 0;
		}

		const auto entity_class_info = g::memory.read<std::uintptr_t>( entity_identity + 0x8 );
		if ( !entity_class_info )
		{
			return 0;
		}

		const auto schema_name_ptr = g::memory.read<std::uintptr_t>( entity_class_info + 0x8 );
		if ( !schema_name_ptr )
		{
			return 0;
		}

		const auto schema_name = g::memory.read<std::uintptr_t>( schema_name_ptr );
		if ( !schema_name )
		{
			return 0;
		}

		char class_name[ 64 ]{};
		g::memory.read( schema_name, class_name, sizeof( class_name ) );

		if ( !class_name[ 0 ] )
		{
			return 0;
		}

		return fnv1a::runtime_hash( class_name );
	}

	entities::type entities::classify( std::uint32_t schema_hash ) const
	{
		switch ( schema_hash )
		{
		case "CCSPlayerController"_hash:
			return type::player;

		case "C_AK47"_hash:
		case "C_WeaponM4A1"_hash:
		case "C_WeaponM4A1Silencer"_hash:
		case "C_WeaponAWP"_hash:
		case "C_WeaponAug"_hash:
		case "C_WeaponFamas"_hash:
		case "C_WeaponGalilAR"_hash:
		case "C_WeaponSG556"_hash:
		case "C_WeaponG3SG1"_hash:
		case "C_WeaponSCAR20"_hash:
		case "C_WeaponSSG08"_hash:
		case "C_WeaponMAC10"_hash:
		case "C_WeaponMP5SD"_hash:
		case "C_WeaponMP7"_hash:
		case "C_WeaponMP9"_hash:
		case "C_WeaponBizon"_hash:
		case "C_WeaponP90"_hash:
		case "C_WeaponUMP45"_hash:
		case "C_WeaponNOVA"_hash:
		case "C_WeaponSawedoff"_hash:
		case "C_WeaponXM1014"_hash:
		case "C_WeaponMag7"_hash:
		case "C_WeaponM249"_hash:
		case "C_WeaponNegev"_hash:
		case "C_DEagle"_hash:
		case "C_WeaponElite"_hash:
		case "C_WeaponFiveSeven"_hash:
		case "C_WeaponGlock"_hash:
		case "C_WeaponHKP2000"_hash:
		case "C_WeaponUSPSilencer"_hash:
		case "C_WeaponP250"_hash:
		case "C_WeaponCZ75a"_hash:
		case "C_WeaponTec9"_hash:
		case "C_WeaponRevolver"_hash:
		case "C_WeaponTaser"_hash:
		case "C_Knife"_hash:
		case "C_C4"_hash:
		case "C_Item_Healthshot"_hash:
		case "C_HEGrenade"_hash:
		case "C_Flashbang"_hash:
		case "C_SmokeGrenade"_hash:
		case "C_MolotovGrenade"_hash:
		case "C_IncendiaryGrenade"_hash:
		case "C_DecoyGrenade"_hash:
			return type::item;

		case "C_HEGrenadeProjectile"_hash:
		case "C_FlashbangProjectile"_hash:
		case "C_SmokeGrenadeProjectile"_hash:
		case "C_MolotovProjectile"_hash:
		case "C_Inferno"_hash:
		case "C_DecoyProjectile"_hash:
			return type::projectile;

		default:
			return type::unknown;
		}
	}

} // namespace systems
