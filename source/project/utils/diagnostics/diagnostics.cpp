#include <stdafx.hpp>

#include <dbghelp.h>
#include <fstream>
#include <mutex>

#pragma comment( lib, "Dbghelp.lib" )

namespace diagnostics {

	namespace {

		std::mutex g_lock{};
		std::string g_stage{ "startup" };

		std::filesystem::path get_base_dir( )
		{
			std::error_code ec{};
			auto dir = std::filesystem::temp_directory_path( ec );
			if ( ec )
			{
				dir = std::filesystem::current_path( );
			}

			return dir / "velora";
		}

		void append_log_line( const std::string& line ) noexcept
		{
			std::lock_guard<std::mutex> guard( g_lock );

			std::error_code ec{};
			auto dir = get_base_dir( ) / "logs";
			std::filesystem::create_directories( dir, ec );

			std::ofstream out( dir / "startup.log", std::ios::app );
			if ( !out.is_open( ) )
			{
				return;
			}

			out << line << '\n';
		}

		LONG WINAPI unhandled_exception_filter( EXCEPTION_POINTERS* exception_pointers )
		{
			std::error_code ec{};
			auto dir = get_base_dir( ) / "dumps";
			std::filesystem::create_directories( dir, ec );

			const auto file_name = std::format( "velora_{}_{}.dmp", ::GetCurrentProcessId( ), ::GetTickCount64( ) );
			const auto dump_path = dir / file_name;

			const auto dump_file = ::CreateFileW( dump_path.c_str( ), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
			if ( dump_file != INVALID_HANDLE_VALUE )
			{
				MINIDUMP_EXCEPTION_INFORMATION info{};
				info.ThreadId = ::GetCurrentThreadId( );
				info.ExceptionPointers = exception_pointers;
				info.ClientPointers = FALSE;

				::MiniDumpWriteDump( ::GetCurrentProcess( ), ::GetCurrentProcessId( ), dump_file, MiniDumpWithThreadInfo, &info, nullptr, nullptr );
				::CloseHandle( dump_file );
			}

			append_log_line( std::format( "crash: stage={} code=0x{:08X}", g_stage, exception_pointers && exception_pointers->ExceptionRecord ? exception_pointers->ExceptionRecord->ExceptionCode : 0u ) );
			return EXCEPTION_EXECUTE_HANDLER;
		}

	} // namespace

	void initialize( ) noexcept
	{
		::SetUnhandledExceptionFilter( unhandled_exception_filter );
		append_log_line( "diagnostics initialized" );
	}

	void set_startup_stage( const char* stage ) noexcept
	{
		if ( !stage )
		{
			return;
		}

		{
			std::lock_guard<std::mutex> guard( g_lock );
			g_stage = stage;
		}

		append_log_line( std::format( "startup stage: {}", stage ) );
	}

	void note_startup_failure( const char* stage, const char* reason ) noexcept
	{
		const auto safe_stage = stage ? stage : "unknown";
		const auto safe_reason = reason ? reason : "unknown";
		append_log_line( std::format( "startup failure: stage={} reason={}", safe_stage, safe_reason ) );
	}

} // namespace diagnostics
