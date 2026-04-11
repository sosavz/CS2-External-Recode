#include <stdafx.hpp>

namespace systems {

	/*
	 * Bone Cache Structure:
	 * - Bone matrices stored as array of bone_data structures
	 * - Each bone_data is 48 bytes (16-byte aligned):
	 *   - 12 bytes: position vector3 (x, y, z)
	 *   - 4 bytes: float scale
	 *   - 16 bytes: quaternion rotation (x, y, z, w)
	 *   - 16 bytes padding for alignment
	 * - Index 0 = root bone (pelvis)
	 * - Index 6 = head bone
	 * - Indices 22-27 = hand/finger bones
	 */

	bool bones::data::is_valid( ) const
	{
		const auto& root = this->bones[ 0 ].position;
		return root.x != 0.0f || root.y != 0.0f || root.z != 0.0f;
	}

	math::vector3 bones::data::get_position( std::uint32_t id ) const
	{
		const auto index = static_cast< std::size_t >( id );
		if ( index >= this->bones.size( ) )
		{
			return {};
		}

		return this->bones[ index ].position;
	}

	math::quaternion bones::data::get_rotation( std::uint32_t id ) const
	{
		const auto index = static_cast< std::size_t >( id );
		if ( index >= this->bones.size( ) )
		{
			return {};
		}

		return this->bones[ index ].rotation;
	}

	bones::data bones::get( std::uintptr_t bone_cache ) const
	{
		if ( !bone_cache || bone_cache < 0x10000 )
		{
			return {};
		}

		struct alignas( 16 ) bone_data
		{
			math::vector3 position;
			float scale;
			math::quaternion rotation;
		};

		constexpr auto bone_count = constants::render::k_bone_count;
		std::array<bone_data, bone_count> raw{};
		if ( !g::memory.read( bone_cache, raw.data( ), sizeof( bone_data ) * bone_count ) )
		{
			return {};
		}

		data result{};
		for ( std::size_t i = 0; i < result.bones.size( ); ++i )
		{
			result.bones[ i ].position = raw[ i ].position;
			result.bones[ i ].rotation = raw[ i ].rotation;
		}

		return result;
	}

	std::optional<math::vector3> bones::get_head_position( std::uintptr_t bone_cache ) const
	{
		if ( !bone_cache )
			return std::nullopt;

		struct alignas( 16 ) bone_data
		{
			math::vector3 position;
			float scale;
			math::quaternion rotation;
		};

		bone_data head_bone{};
		if ( !g::memory.read( bone_cache + sizeof( bone_data ) * 6, &head_bone, sizeof( bone_data ) ) )
			return std::nullopt;

		if ( head_bone.position.x == 0.0f && head_bone.position.y == 0.0f && head_bone.position.z == 0.0f )
			return std::nullopt;

		return head_bone.position;
	}

} // namespace systems