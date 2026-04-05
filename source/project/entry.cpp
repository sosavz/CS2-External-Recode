#include <stdafx.hpp>

#include <timeapi.h>
#pragma comment( lib, "winmm.lib" )

namespace {

	constexpr wchar_t k_loader_class[ ] = L"VeloraLoaderWindow";
	bool g_launch_requested = false;
	RECT g_launch_rect{ 90, 196, 210, 234 };
	RECT g_close_rect{ 279, 9, 291, 21 };
	bool g_launch_hovered = false;
	bool g_close_hovered = false;
	bool g_loading = false;
	std::atomic_int g_loading_step{ 0 };
	bool g_cs2_available = false;
	HFONT g_title_font = nullptr;
	HFONT g_body_font = nullptr;
	HBRUSH g_background_brush = nullptr;
	HBRUSH g_panel_brush = nullptr;

	constexpr const wchar_t* k_loading_lines[ ]
	{
		L"checking cs2 process...",
		L"initializing input...",
		L"attaching to process...",
		L"loading modules...",
		L"resolving offsets...",
		L"starting worker threads...",
		L"starting overlay renderer...",
	};

	bool is_process_open( const wchar_t* process_name )
	{
		const auto snap = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
		if ( snap == INVALID_HANDLE_VALUE )
		{
			return false;
		}

		PROCESSENTRY32W entry{ .dwSize = sizeof( PROCESSENTRY32W ) };
		bool found = false;

		if ( ::Process32FirstW( snap, &entry ) )
		{
			do
			{
				if ( ::lstrcmpiW( entry.szExeFile, process_name ) == 0 )
				{
					found = true;
					break;
				}
			} while ( ::Process32NextW( snap, &entry ) );
		}

		::CloseHandle( snap );
		return found;
	}

	void paint_loader( HWND hwnd )
	{
		PAINTSTRUCT ps{};
		const auto hdc = ::BeginPaint( hwnd, &ps );
		if ( !hdc )
		{
			return;
		}

		RECT rc{};
		::GetClientRect( hwnd, &rc );
		::FillRect( hdc, &rc, g_background_brush );

		RECT panel{ 14, 14, rc.right - 14, rc.bottom - 14 };
		::FillRect( hdc, &panel, g_panel_brush );

		::SetBkMode( hdc, TRANSPARENT );

		{
			if ( g_close_hovered )
			{
				HBRUSH close_bg = ::CreateSolidBrush( RGB( 34, 20, 40 ) );
				::FillRect( hdc, &g_close_rect, close_bg );
				::DeleteObject( close_bg );
			}

			const auto close_col = g_close_hovered ? RGB( 228, 170, 214 ) : RGB( 176, 130, 192 );
			HPEN close_pen = ::CreatePen( PS_SOLID, 1, close_col );
			auto old_close_pen = static_cast< HPEN >( ::SelectObject( hdc, close_pen ) );

			::MoveToEx( hdc, g_close_rect.left, g_close_rect.top, nullptr );
			::LineTo( hdc, g_close_rect.right, g_close_rect.bottom );
			::MoveToEx( hdc, g_close_rect.right, g_close_rect.top, nullptr );
			::LineTo( hdc, g_close_rect.left, g_close_rect.bottom );

			::SelectObject( hdc, old_close_pen );
			::DeleteObject( close_pen );
		}

		::SetTextColor( hdc, RGB( 218, 194, 225 ) );

		if ( g_title_font )
		{
			::SelectObject( hdc, g_title_font );
		}

		RECT title_rect{ 0, 62, rc.right, 94 };
		::DrawTextW( hdc, L"VELORA", -1, &title_rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER );

		if ( g_body_font )
		{
			::SelectObject( hdc, g_body_font );
		}

		if ( !g_loading )
		{
			const auto enabled = g_cs2_available;
			const auto btn_fill = !enabled ? RGB( 46, 39, 52 ) : ( g_launch_hovered ? RGB( 121, 75, 140 ) : RGB( 96, 62, 113 ) );
			HBRUSH btn_brush = ::CreateSolidBrush( btn_fill );
			::FillRect( hdc, &g_launch_rect, btn_brush );
			::DeleteObject( btn_brush );

			const auto border = enabled ? RGB( 164, 112, 186 ) : RGB( 70, 60, 80 );
			HPEN pen = ::CreatePen( PS_SOLID, 1, border );
			auto old_pen = static_cast< HPEN >( ::SelectObject( hdc, pen ) );
			auto old_brush = static_cast< HBRUSH >( ::SelectObject( hdc, ::GetStockObject( HOLLOW_BRUSH ) ) );
			::Rectangle( hdc, g_launch_rect.left, g_launch_rect.top, g_launch_rect.right, g_launch_rect.bottom );
			::SelectObject( hdc, old_pen );
			::SelectObject( hdc, old_brush );
			::DeleteObject( pen );

			::SetTextColor( hdc, enabled ? RGB( 235, 222, 240 ) : RGB( 120, 110, 128 ) );
			RECT launch_text = g_launch_rect;
			::DrawTextW( hdc, L"load", -1, &launch_text, DT_CENTER | DT_SINGLELINE | DT_VCENTER );

			::SetTextColor( hdc, g_cs2_available ? RGB( 151, 214, 168 ) : RGB( 205, 166, 166 ) );
			RECT state_rect{ 30, 232, rc.right - 30, 256 };
			::DrawTextW( hdc, g_cs2_available ? L"cs2 detected" : L"open cs2 to enable loading", -1, &state_rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
		}
		else
		{
			const auto tick = static_cast< int >( ::GetTickCount64( ) / 90ull );
			constexpr auto cx{ 150.0f };
			constexpr auto cy{ 184.0f };
			constexpr auto radius{ 18.0f };

			for ( int i = 0; i < 12; ++i )
			{
				const auto ang = ( static_cast< float >( i ) / 12.0f ) * 6.2831853f;
				const auto px = cx + std::cosf( ang ) * radius;
				const auto py = cy + std::sinf( ang ) * radius;

				auto alpha = 65;
				if ( i == ( tick % 12 ) ) alpha = 255;
				else if ( i == ( ( tick + 11 ) % 12 ) ) alpha = 180;
				else if ( i == ( ( tick + 10 ) % 12 ) ) alpha = 120;

				HBRUSH dot = ::CreateSolidBrush( RGB( static_cast< BYTE >( 130 + alpha / 4 ), static_cast< BYTE >( 88 + alpha / 6 ), static_cast< BYTE >( 147 + alpha / 4 ) ) );
				RECT r{ static_cast< LONG >( px - 3 ), static_cast< LONG >( py - 3 ), static_cast< LONG >( px + 3 ), static_cast< LONG >( py + 3 ) };
				::FillRect( hdc, &r, dot );
				::DeleteObject( dot );
			}

			::SetTextColor( hdc, RGB( 203, 178, 214 ) );
			auto step = g_loading_step.load( );
			if ( step < 0 )
			{
				step = 0;
			}
			if ( step >= static_cast< int >( std::size( k_loading_lines ) ) )
			{
				step = static_cast< int >( std::size( k_loading_lines ) - 1 );
			}
			RECT status_rect{ 38, 226, rc.right - 38, 252 };
			::DrawTextW( hdc, k_loading_lines[ step ], -1, &status_rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER );

			const auto progress = static_cast< float >( step + 1 ) / static_cast< float >( std::size( k_loading_lines ) );
			RECT bar_bg{ 48, 262, rc.right - 48, 272 };
			HBRUSH bar_bg_brush = ::CreateSolidBrush( RGB( 33, 26, 40 ) );
			::FillRect( hdc, &bar_bg, bar_bg_brush );
			::DeleteObject( bar_bg_brush );

			RECT bar_fill{ bar_bg.left, bar_bg.top, bar_bg.left + static_cast< LONG >( ( bar_bg.right - bar_bg.left ) * progress ), bar_bg.bottom };
			HBRUSH bar_fill_brush = ::CreateSolidBrush( RGB( 153, 98, 176 ) );
			::FillRect( hdc, &bar_fill, bar_fill_brush );
			::DeleteObject( bar_fill_brush );
		}

		::EndPaint( hwnd, &ps );
	}

	bool launch_runtime( )
	{
		threads::reset_stop( );
		diagnostics::set_startup_stage( "input" );

		if ( !g::input.initialize( ) )
		{
			diagnostics::note_startup_failure( "input", "initialize failed" );
			::MessageBoxW( nullptr, L"Failed to initialize input.", L"Loader Error", MB_OK | MB_ICONERROR );
			return false;
		}

		diagnostics::set_startup_stage( "memory" );
		if ( !g::memory.initialize( L"cs2.exe" ) )
		{
			diagnostics::note_startup_failure( "memory", "attach failed" );
			::MessageBoxW( nullptr, L"Failed to attach to cs2.exe.", L"Loader Error", MB_OK | MB_ICONERROR );
			return false;
		}

		diagnostics::set_startup_stage( "modules" );
		if ( !g::modules.initialize( ) )
		{
			diagnostics::note_startup_failure( "modules", "initialize failed" );
			::MessageBoxW( nullptr, L"Failed to initialize modules.", L"Loader Error", MB_OK | MB_ICONERROR );
			return false;
		}

		diagnostics::set_startup_stage( "offsets" );
		if ( !g::offsets.initialize( ) )
		{
			diagnostics::note_startup_failure( "offsets", "initialize failed" );
			::MessageBoxW( nullptr, L"Failed to initialize offsets.", L"Loader Error", MB_OK | MB_ICONERROR );
			return false;
		}

		diagnostics::set_startup_stage( "threads" );
		std::thread( threads::game ).detach( );
		std::thread( threads::combat ).detach( );

		diagnostics::set_startup_stage( "render" );
		if ( !g::render.initialize( ) )
		{
			threads::request_stop( );
			diagnostics::note_startup_failure( "render", "initialize failed" );
			::MessageBoxW( nullptr, L"Failed to initialize render.", L"Loader Error", MB_OK | MB_ICONERROR );
			return false;
		}

		diagnostics::set_startup_stage( "running" );

		return true;
	}

	void start_fake_loading( HWND hwnd )
	{
		g_cs2_available = is_process_open( L"cs2.exe" );
		if ( !g_cs2_available )
		{
			::InvalidateRect( hwnd, nullptr, FALSE );
			return;
		}

		g_loading = true;
		g_loading_step.store( 0 );
		::SetTimer( hwnd, 1, 30, nullptr );

		std::thread( [hwnd]
			{
				for ( int i = 0; i < static_cast< int >( std::size( k_loading_lines ) ); ++i )
				{
					g_loading_step.store( i );
					::PostMessageW( hwnd, WM_APP + 2, 0, 0 );
					std::this_thread::sleep_for( std::chrono::milliseconds( 650 + i * 120 ) );
				}

				std::this_thread::sleep_for( std::chrono::milliseconds( 950 ) );
				::PostMessageW( hwnd, WM_APP + 1, 0, 0 );
			} ).detach( );
	}

	LRESULT CALLBACK loader_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
	{
		switch ( msg )
		{
		case WM_CREATE:
		{
			g_cs2_available = is_process_open( L"cs2.exe" );
			g_background_brush = ::CreateSolidBrush( RGB( 8, 8, 10 ) );
			g_panel_brush = ::CreateSolidBrush( RGB( 12, 10, 15 ) );

			g_title_font = ::CreateFontW( 28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Inter" );
			g_body_font = ::CreateFontW( 16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Inter" );
			::SetTimer( hwnd, 1, 250, nullptr );

			return 0;
		}
		case WM_MOUSEMOVE:
		{
			const POINT pt{ GET_X_LPARAM( lparam ), GET_Y_LPARAM( lparam ) };
			const auto launch_hovered = !g_loading && g_cs2_available && ::PtInRect( &g_launch_rect, pt ) != FALSE;
			const auto close_hovered = ::PtInRect( &g_close_rect, pt ) != FALSE;

			if ( launch_hovered != g_launch_hovered || close_hovered != g_close_hovered )
			{
				g_launch_hovered = launch_hovered;
				g_close_hovered = close_hovered;
				::InvalidateRect( hwnd, nullptr, FALSE );
			}

			TRACKMOUSEEVENT tme{ sizeof( TRACKMOUSEEVENT ), TME_LEAVE, hwnd, 0 };
			::TrackMouseEvent( &tme );
			return 0;
		}
		case WM_MOUSELEAVE:
			if ( g_launch_hovered || g_close_hovered )
			{
				g_launch_hovered = false;
				g_close_hovered = false;
				::InvalidateRect( hwnd, nullptr, FALSE );
			}
			return 0;
		case WM_LBUTTONUP:
		{
			const POINT pt{ GET_X_LPARAM( lparam ), GET_Y_LPARAM( lparam ) };
			if ( ::PtInRect( &g_close_rect, pt ) )
			{
				::DestroyWindow( hwnd );
				return 0;
			}

			if ( ::PtInRect( &g_launch_rect, pt ) )
			{
				if ( !g_loading )
				{
					start_fake_loading( hwnd );
				}
				return 0;
			}
			return 0;
		}
		case WM_TIMER:
			if ( !g_loading )
			{
				const auto before = g_cs2_available;
				g_cs2_available = is_process_open( L"cs2.exe" );
				if ( before != g_cs2_available )
				{
					::InvalidateRect( hwnd, nullptr, FALSE );
				}
			}
			::InvalidateRect( hwnd, nullptr, FALSE );
			return 0;
		case WM_APP + 2:
			::InvalidateRect( hwnd, nullptr, FALSE );
			return 0;
		case WM_APP + 1:
			g_launch_requested = true;
			::DestroyWindow( hwnd );
			return 0;
		case WM_PAINT:
			paint_loader( hwnd );
			return 0;
		case WM_CLOSE:
			::DestroyWindow( hwnd );
			return 0;
		case WM_DESTROY:
			::KillTimer( hwnd, 1 );

			if ( g_title_font )
			{
				::DeleteObject( g_title_font );
				g_title_font = nullptr;
			}

			if ( g_body_font )
			{
				::DeleteObject( g_body_font );
				g_body_font = nullptr;
			}

			if ( g_background_brush )
			{
				::DeleteObject( g_background_brush );
				g_background_brush = nullptr;
			}

			if ( g_panel_brush )
			{
				::DeleteObject( g_panel_brush );
				g_panel_brush = nullptr;
			}

			::PostQuitMessage( 0 );
			return 0;
		default:
			break;
		}

		return ::DefWindowProcW( hwnd, msg, wparam, lparam );
	}

} // namespace

int APIENTRY wWinMain( HINSTANCE instance, HINSTANCE, LPWSTR, int show_cmd )
{
	diagnostics::initialize( );
	timeBeginPeriod( 1 );

	WNDCLASSEXW wc{ sizeof( WNDCLASSEXW ) };
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = loader_wndproc;
	wc.hInstance = instance;
	wc.hCursor = ::LoadCursorW( nullptr, reinterpret_cast< LPCWSTR >( IDC_ARROW ) );
	wc.hbrBackground = nullptr;
	wc.lpszClassName = k_loader_class;

	if ( !::RegisterClassExW( &wc ) )
	{
		return 1;
	}

	constexpr auto w = 300;
	constexpr auto h = 300;

	const auto screen_w = ::GetSystemMetrics( SM_CXSCREEN );
	const auto screen_h = ::GetSystemMetrics( SM_CYSCREEN );

	const auto x = ( screen_w - w ) / 2;
	const auto y = ( screen_h - h ) / 2;

	const auto hwnd = ::CreateWindowExW( 0, k_loader_class, L"", WS_POPUP, x, y, w, h, nullptr, nullptr, instance, nullptr );
	if ( !hwnd )
	{
		return 1;
	}

	::ShowWindow( hwnd, show_cmd );
	::UpdateWindow( hwnd );

	MSG msg{};
	while ( ::GetMessageW( &msg, nullptr, 0, 0 ) > 0 )
	{
		::TranslateMessage( &msg );
		::DispatchMessageW( &msg );
	}

	if ( !g_launch_requested )
	{
		return 0;
	}

	return launch_runtime( ) ? 0 : 1;
}
