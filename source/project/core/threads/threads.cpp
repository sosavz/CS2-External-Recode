#include <stdafx.hpp>

namespace threads {

	namespace {

		std::atomic_bool g_stop_requested{ false };

	}

	void request_stop( ) noexcept
	{
		g_stop_requested.store( true, std::memory_order_relaxed );
	}

	void reset_stop( ) noexcept
	{
		g_stop_requested.store( false, std::memory_order_relaxed );
	}

	bool should_stop( ) noexcept
	{
		return g_stop_requested.load( std::memory_order_relaxed );
	}

	void game( )
	{
		std::string last_map{};
		int dead_streak{ 0 };

		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

		while ( !should_stop( ) )
		{
			if ( !g::memory.process_alive( ) )
			{
				if ( ++dead_streak < 25 )
				{
					std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
					continue;
				}

				request_stop( );
				if ( g::render.hwnd( ) )
				{
					::PostMessageW( g::render.hwnd( ), WM_CLOSE, 0, 0 );
				}

				return;
			}

			dead_streak = 0;

			systems::g_local.update( );

			if ( systems::g_local.valid( ) )
			{
				systems::g_view.update( );
				systems::g_entities.refresh( );
				systems::g_collector.run( );

				const auto global_vars = g::memory.read<std::uintptr_t>( g::offsets.global_vars );
				if ( global_vars )
				{
					const auto map_ptr = g::memory.read<std::uintptr_t>( global_vars + 0x188 );
					const auto current_map = map_ptr ? g::memory.read_string( map_ptr ) : std::string{};

					if ( !current_map.empty( ) && current_map != "<empty>" && current_map != last_map ) 
					{
						g::console.print( "map change: {} -> {}", last_map.empty( ) ? "none" : last_map, current_map );
						last_map = current_map;
						systems::g_bvh.clear( );
						g::console.print( "parsing bvh for {}...", current_map );
						systems::g_bvh.parse( );
						g::console.success( "bvh parsed." );
					}
				}
			}
			else
			{
				if ( !last_map.empty( ) )
				{
					last_map = {};
					systems::g_bvh.clear( );
				}
			}

			std::this_thread::yield( );
		}
	}

	void combat( )
	{
		constexpr auto target_tps{ 128 };
		constexpr auto tick_interval = std::chrono::nanoseconds( 1'000'000'000 / target_tps );
		auto next_tick = std::chrono::steady_clock::now( );
		int dead_streak{ 0 };

		while ( !should_stop( ) )
		{
			if ( !g::memory.process_alive( ) )
			{
				if ( ++dead_streak < 25 )
				{
					next_tick = std::chrono::steady_clock::now( );
					continue;
				}

				request_stop( );
				if ( g::render.hwnd( ) )
				{
					::PostMessageW( g::render.hwnd( ), WM_CLOSE, 0, 0 );
				}

				return;
			}

			dead_streak = 0;

			systems::g_local.update( );

			if ( systems::g_local.valid( ) )
			{
				features::combat::g_shared.tick( );
				features::combat::g_legit.tick( );
			}

			next_tick += tick_interval;

			const auto now = std::chrono::steady_clock::now( );
			if ( next_tick < now )
			{
				next_tick = now;
				continue;
			}

			std::this_thread::sleep_until( next_tick - std::chrono::milliseconds( 1 ) );

			while ( std::chrono::steady_clock::now( ) < next_tick )
			{
				_mm_pause( );
			}
		}
	}

} // namespace threads
