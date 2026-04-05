#include <stdafx.hpp>

namespace systems {

	void view::update( )
	{
		const auto matrix = g::memory.read<math::matrix4x4>( g::offsets.view_matrix );

		auto view_render = this->m_view_render.load( std::memory_order_relaxed );
		if ( !view_render )
		{
			view_render = g::memory.find_vtable_instance( g::modules.client, "CViewRender" );
			this->m_view_render.store( view_render, std::memory_order_relaxed );
		}

		if ( !view_render )
		{
			std::unique_lock lock( this->m_mutex );
			this->m_matrix = matrix;
			this->m_origin = { k_invalid, k_invalid, k_invalid };
			this->m_angles = { k_invalid, k_invalid, k_invalid };
			this->m_fov = k_invalid;
			return;
		}

		const auto view = view_render + 0x10;
		math::vector3 origin{ k_invalid, k_invalid, k_invalid };
		math::vector3 angles{ k_invalid, k_invalid, k_invalid };
		float fov = k_invalid;

		if ( view )
		{
			origin = g::memory.read<math::vector3>( view + 0x0 );
			angles = g::memory.read<math::vector3>( view + 0xc );
			fov = g::memory.read<float>( view + 0x18 );
		}

		std::unique_lock lock( this->m_mutex );
		this->m_matrix = matrix;
		this->m_origin = origin;
		this->m_angles = angles;
		this->m_fov = fov;
	}

	math::vector2 view::project( const math::vector3& world_pos )
	{
		math::matrix4x4 m{};
		{
			std::shared_lock lock( this->m_mutex );
			m = this->m_matrix;
		}

		if ( m[ 3 ][ 3 ] == 0.0f )
		{
			return { this->k_invalid, this->k_invalid };
		}

		const auto w = m[ 3 ][ 0 ] * world_pos.x + m[ 3 ][ 1 ] * world_pos.y + m[ 3 ][ 2 ] * world_pos.z + m[ 3 ][ 3 ];

		if ( w < 0.01f )
		{
			return { this->k_invalid, this->k_invalid };
		}

		const auto x = m[ 0 ][ 0 ] * world_pos.x + m[ 0 ][ 1 ] * world_pos.y + m[ 0 ][ 2 ] * world_pos.z + m[ 0 ][ 3 ];
		const auto y = m[ 1 ][ 0 ] * world_pos.x + m[ 1 ][ 1 ] * world_pos.y + m[ 1 ][ 2 ] * world_pos.z + m[ 1 ][ 3 ];

		const auto display = zdraw::get_display_size( );
		const auto inv_w = 1.0f / w;

		return
		{
			display.first * 0.5f * ( 1.0f + x * inv_w ),
			display.second * 0.5f * ( 1.0f - y * inv_w )
		};
	}

	bool view::has_camera( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_fov != this->k_invalid;
	}

	math::vector3 view::origin( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_origin;
	}

	math::vector3 view::angles( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_angles;
	}

	float view::fov( ) const
	{
		std::shared_lock lock( this->m_mutex );
		return this->m_fov;
	}

} // namespace systems
