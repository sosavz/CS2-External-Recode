#pragma once

#include <stdafx.hpp>
#include <deque>

namespace perf {

	class counter {
	public:
		counter( ) = default;

		void record( float value ) {
			if ( m_samples.size( ) >= 60 )
				m_samples.pop_front( );
			m_samples.push_back( value );
		}

		[[nodiscard]] float current( ) const { return m_samples.empty( ) ? 0.0f : m_samples.back( ); }
		[[nodiscard]] float average( ) const {
			if ( m_samples.empty( ) ) return 0.0f;
			float sum = 0.0f;
			for ( const auto& s : m_samples ) sum += s;
			return sum / static_cast< float >( m_samples.size( ) );
		}
		[[nodiscard]] float min( ) const {
			if ( m_samples.empty( ) ) return 0.0f;
			float m = m_samples[ 0 ];
			for ( const auto& s : m_samples ) if ( s < m ) m = s;
			return m;
		}
		[[nodiscard]] float max( ) const {
			if ( m_samples.empty( ) ) return 0.0f;
			float m = m_samples[ 0 ];
			for ( const auto& s : m_samples ) if ( s > m ) m = s;
			return m;
		}

	private:
		std::deque<float> m_samples;
	};

	class module_stats {
	public:
		void record( float ms ) {
			if ( m_times.size( ) >= 120 )
				m_times.pop_front( );
			m_times.push_back( ms );
		}

		[[nodiscard]] float current( ) const { return m_times.empty( ) ? 0.0f : m_times.back( ); }
		[[nodiscard]] float average( ) const {
			if ( m_times.empty( ) ) return 0.0f;
			float sum = 0.0f;
			for ( const auto& t : m_times ) sum += t;
			return sum / static_cast< float >( m_times.size( ) );
		}
		[[nodiscard]] float peak( ) const {
			if ( m_times.empty( ) ) return 0.0f;
			float p = m_times[ 0 ];
			for ( const auto& t : m_times ) if ( t > p ) p = t;
			return p;
		}
		[[nodiscard]] int samples( ) const { return static_cast< int >( m_times.size( ) ); }

	private:
		std::deque<float> m_times;
	};

	class profiler {
	public:
		static profiler& get( ) {
			static profiler instance;
			return instance;
		}

		void new_frame( ) {
			std::lock_guard lock( m_mutex );
			const auto now = std::chrono::steady_clock::now( );
			if ( m_last_frame != std::chrono::steady_clock::time_point{ } ) {
				const auto dt = std::chrono::duration<float, std::milli>( now - m_last_frame ).count( );
				if ( dt > 0.001f ) {
					m_frame_times.record( 1000.0f / dt );
					m_frame_time_ms.record( dt );
				}
			}
			m_last_frame = now;
		}

		void begin_module( const char* name ) {
			std::lock_guard lock( m_mutex );
			m_module_start[ name ] = std::chrono::steady_clock::now( );
		}

		void end_module( const char* name ) {
			std::lock_guard lock( m_mutex );
			auto it = m_module_start.find( name );
			if ( it != m_module_start.end( ) ) {
				const auto elapsed = std::chrono::duration<float, std::milli>( std::chrono::steady_clock::now( ) - it->second ).count( );
				m_module_stats[ name ].record( elapsed );
				m_module_start.erase( it );
			}
		}

		[[nodiscard]] std::optional<module_stats> get_module_stats_copy( const char* name ) const {
			std::lock_guard lock( m_mutex );
			auto it = m_module_stats.find( name );
			return ( it != m_module_stats.end( ) ) ? std::optional<module_stats>( it->second ) : std::nullopt;
		}

		[[nodiscard]] int current_fps( ) const {
			std::lock_guard lock( m_mutex );
			return static_cast< int >( m_frame_times.current( ) );
		}
		[[nodiscard]] float current_frame_ms( ) const {
			std::lock_guard lock( m_mutex );
			return m_frame_time_ms.current( );
		}
		[[nodiscard]] float fps_average( ) const {
			std::lock_guard lock( m_mutex );
			return m_frame_times.average( );
		}
		[[nodiscard]] float fps_min( ) const {
			std::lock_guard lock( m_mutex );
			return m_frame_times.min( );
		}
		[[nodiscard]] float fps_max( ) const {
			std::lock_guard lock( m_mutex );
			return m_frame_times.max( );
		}

		[[nodiscard]] MEMORYSTATUSEX get_memory( ) const {
			MEMORYSTATUSEX mse{};
			mse.dwLength = sizeof( MEMORYSTATUSEX );
			::GlobalMemoryStatusEx( &mse );
			return mse;
		}

		[[nodiscard]] float get_cpu_usage( ) const {
			std::lock_guard lock( m_cpu_mutex );
			static int64_t last_idle = 0, last_total = 0;
			FILETIME idle_time, kernel_time, user_time;
			if ( !GetSystemTimes( &idle_time, &kernel_time, &user_time ) )
				return 0.0f;

			const auto to_int64 = []( const FILETIME& ft ) -> int64_t {
				return ( static_cast< int64_t >( ft.dwHighDateTime ) << 32 ) | ft.dwLowDateTime;
			};

			const int64_t idle = to_int64( idle_time );
			const int64_t total = to_int64( kernel_time ) + to_int64( user_time );

			if ( last_total != 0 ) {
				const int64_t idle_delta = idle - last_idle;
				const int64_t total_delta = total - last_total;
				if ( total_delta > 0 ) {
					last_idle = idle;
					last_total = total;
					return 100.0f * ( 1.0f - static_cast< float >( idle_delta ) / static_cast< float >( total_delta ) );
				}
			}
			last_idle = idle;
			last_total = total;
			return 0.0f;
		}

		[[nodiscard]] bool is_enabled( ) const { return m_enabled.load( std::memory_order_relaxed ); }
		void set_enabled( bool enabled ) { m_enabled.store( enabled, std::memory_order_relaxed ); }

		[[nodiscard]] int fps_warning_threshold( ) const { return m_fps_threshold.load( std::memory_order_relaxed ); }
		void set_fps_threshold( int threshold ) { m_fps_threshold.store( threshold, std::memory_order_relaxed ); }

	private:
		profiler( ) = default;

		mutable std::mutex m_mutex{};
		mutable std::mutex m_cpu_mutex{};
		std::atomic_bool m_enabled{ false };
		std::atomic_int m_fps_threshold{ 60 };
		std::chrono::steady_clock::time_point m_last_frame;
		counter m_frame_times;
		counter m_frame_time_ms;
		std::unordered_map<std::string, module_stats> m_module_stats;
		std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_module_start;
	};

}

#define PROFILE_BEGIN( name ) perf::profiler::get( ).begin_module( name )
#define PROFILE_END( name ) perf::profiler::get( ).end_module( name )
