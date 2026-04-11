#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <string>
#include <chrono>
#include <fstream>
#include <mutex>
#include <array>
#include <cstdarg>
#include <shlobj.h>

namespace logger {

	enum class category : std::uint8_t {
		general,
		entity,
		collector,
		esp,
		aimbot,
		triggerbot,
		render,
		memory,
		offsets,
		local,
		bones,
		hitboxes,
		bvh,
		visibility,
		input,
		count
	};

	constexpr auto k_category_count = static_cast<size_t>( category::count );

	inline const char* category_names[ static_cast<size_t>( k_category_count ) ] = {
		"[GENERAL]",
		"[ENTITY]",
		"[COLLECTOR]",
		"[ESP]",
		"[AIMBOT]",
		"[TRIGGERBOT]",
		"[RENDER]",
		"[MEMORY]",
		"[OFFSETS]",
		"[LOCAL]",
		"[BONES]",
		"[HITBOXES]",
		"[BVH]",
		"[VISIBILITY]",
		"[INPUT]"
	};

	class system {
	public:
		static system& get( ) {
			static system instance;
			return instance;
		}

		void initialize( ) {
			if ( m_initialized )
				return;

			const auto now = std::chrono::system_clock::now( );
			const auto time_t = std::chrono::system_clock::to_time_t( now );
			std::tm tm{};
			localtime_s( &tm, &time_t );

			char buf[ 128 ];
			strftime( buf, sizeof( buf ), "%Y%m%d_%H%M%S", &tm );

			m_filename = std::string( "velora_" ) + buf + ".log";

			char temp_path[ MAX_PATH ];
			if ( !GetTempPathA( MAX_PATH, temp_path ) ) {
				strcpy_s( temp_path, "C:\\" );
			}

			std::string log_dir = std::string( temp_path ) + "velora\\logs";

			CreateDirectoryA( log_dir.c_str( ), nullptr );

			m_full_path = log_dir + "\\" + m_filename;

			m_file.open( m_full_path.c_str( ), std::ios::out | std::ios::trunc );
			if ( m_file.is_open( ) ) {
				log( category::general, "Logger initialized" );
				log( category::general, "Log file: %s", m_full_path.c_str( ) );
			}

			m_initialized = true;
		}

		void shutdown( ) {
			if ( m_file.is_open( ) ) {
				log( category::general, "Logger shutting down" );
				m_file.close( );
			}
			m_initialized = false;
		}

		void set_enabled( bool enabled ) {
			m_enabled = enabled;
		}

		[[nodiscard]] bool is_enabled( ) const {
			return m_enabled;
		}

		void set_category_enabled( category cat, bool enabled ) {
			m_category_enabled[ static_cast<size_t>( cat ) ] = enabled;
		}

		void set_category_enabled_all( bool enabled ) {
			for ( auto& e : m_category_enabled ) {
				e = enabled;
			}
		}

		[[nodiscard]] const std::string& filename( ) const {
			return m_filename;
		}

		void log( category cat, const char* fmt, ... ) {
			if ( !m_enabled || !m_initialized )
				return;

			if ( !m_category_enabled[ static_cast<size_t>( cat ) ] )
				return;

			if ( !m_file.is_open( ) )
				return;

			const auto now = std::chrono::system_clock::now( );
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch( ) ) % 1000;
			const auto time_t = std::chrono::system_clock::to_time_t( now );
			std::tm tm{};
			localtime_s( &tm, &time_t );

			char time_buf[ 64 ];
			strftime( time_buf, sizeof( time_buf ), "%H:%M:%S", &tm );

			char message_buf[ 1024 ];
			va_list args;
			va_start( args, fmt );
			vsnprintf( message_buf, sizeof( message_buf ), fmt, args );
			va_end( args );

			std::lock_guard<std::mutex> guard( m_mutex );

			m_file << "[" << time_buf << "." << ms.count( ) << "] ";
			m_file << category_names[ static_cast<size_t>( cat ) ] << " ";
			m_file << message_buf << "\n";

			if ( m_flush_immediately ) {
				m_file.flush( );
			}
		}

		void log_raw( category cat, const char* msg ) {
			if ( !m_enabled || !m_initialized )
				return;

			if ( !m_category_enabled[ static_cast<size_t>( cat ) ] )
				return;

			if ( !m_file.is_open( ) )
				return;

			const auto now = std::chrono::system_clock::now( );
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch( ) ) % 1000;
			const auto time_t = std::chrono::system_clock::to_time_t( now );
			std::tm tm{};
			localtime_s( &tm, &time_t );

			char time_buf[ 64 ];
			strftime( time_buf, sizeof( time_buf ), "%H:%M:%S", &tm );

			std::lock_guard<std::mutex> guard( m_mutex );

			m_file << "[" << time_buf << "." << ms.count( ) << "] ";
			m_file << category_names[ static_cast<size_t>( cat ) ] << " " << msg << "\n";

			if ( m_flush_immediately ) {
				m_file.flush( );
			}
		}

		void set_flush_immediately( bool flush ) {
			m_flush_immediately = flush;
		}

		void flush( ) {
			if ( m_file.is_open( ) ) {
				m_file.flush( );
			}
		}

	private:
		system( ) {
			for ( auto& e : m_category_enabled ) {
				e = true;
			}
		}

		~system( ) {
			shutdown( );
		}

		bool m_initialized{ false };
		bool m_enabled{ true };
		bool m_flush_immediately{ true };
		std::ofstream m_file;
		std::string m_filename;
		std::string m_full_path;
		std::mutex m_mutex;
		std::array<bool, k_category_count> m_category_enabled{};
	};

}

#define LOG logger::system::get().log
