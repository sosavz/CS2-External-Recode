#include <stdafx.hpp>
#include <core\perf\perf.hpp>

bool render::initialize( )
{
	if ( !this->register_window_class( ) )
	{
		return false;
	}

	const auto screen_w = ::GetSystemMetrics( SM_CXSCREEN );
	const auto screen_h = ::GetSystemMetrics( SM_CYSCREEN );

	this->m_hwnd = ::CreateWindowExW( WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE, k_class_name, k_class_name, WS_POPUP, 0, 0, screen_w, screen_h, nullptr, nullptr, ::GetModuleHandleW( nullptr ), nullptr );
	if ( !this->m_hwnd )
	{
		return false;
	}

	constexpr MARGINS margins{ -1, -1, -1, -1 };
	::DwmExtendFrameIntoClientArea( this->m_hwnd, &margins );
	::SetLayeredWindowAttributes( this->m_hwnd, 0, 255, LWA_ALPHA );
	::ShowWindow( this->m_hwnd, SW_SHOW );
	::UpdateWindow( this->m_hwnd );

	if ( !this->setup_d3d( ) )
	{
		this->shutdown( );
		return false;
	}

	g::console.print( "render initialized." );
	return true;
}

bool render::register_window_class( )
{
	WNDCLASSEXW wc{};
	if ( ::GetClassInfoExW( ::GetModuleHandleW( nullptr ), k_class_name, &wc ) )
	{
		return true;
	}

	wc.cbSize = sizeof( WNDCLASSEXW );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = ::GetModuleHandleW( nullptr );
	wc.hbrBackground = static_cast< HBRUSH >( ::GetStockObject( BLACK_BRUSH ) );
	wc.hCursor = ::LoadCursorW( nullptr, IDC_ARROW );
	wc.lpszClassName = k_class_name;

	this->m_atom = ::RegisterClassExW( &wc );
	return this->m_atom != 0;
}

void render::run( )
{
	constexpr float clear[ 4 ]{ 0.0f, 0.0f, 0.0f, 0.0f };
	MSG msg{};
	auto fps_window_start = std::chrono::steady_clock::now( );
	int fps_frames = 0;
	int fps_value = 0;

	while ( true )
	{
		while ( ::PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				return;
			}

			::TranslateMessage( &msg );
			::DispatchMessageW( &msg );
		}

		perf::profiler::get( ).new_frame( );

		// Keep projection data on the render thread as fresh as possible so ESP stays in sync with camera motion.
		perf::profiler::get( ).begin_module( "View Update" );
		systems::g_view.update( );
		perf::profiler::get( ).end_module( "View Update" );

		this->update_input_window( );

		perf::profiler::get( ).begin_module( "Rendering" );
		this->m_context->OMSetRenderTargets( 1, &this->m_rtv, nullptr );
		this->m_context->ClearRenderTargetView( this->m_rtv, clear );

		zdraw::begin_frame( );
		{
			auto& draw_list = zdraw::get_draw_list( zdraw::draw_layer::background );

			++fps_frames;
			const auto now = std::chrono::steady_clock::now( );
			const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>( now - fps_window_start ).count( );
			if ( elapsed_ms >= constants::render::k_fps_window_ms )
			{
				fps_value = static_cast< int >( std::lround( static_cast< double >( fps_frames ) * 1000.0 / static_cast< double >( elapsed_ms ) ) );
				fps_frames = 0;
				fps_window_start = now;
			}

			if ( systems::g_local.valid( ) )
			{
				perf::profiler::get( ).begin_module( "ESP" );
				features::esp::g_player.on_render( draw_list );
				features::esp::g_item.on_render( draw_list );
				features::esp::g_projectile.on_render( draw_list );
				features::misc::g_grenades.on_render( draw_list );
				perf::profiler::get( ).end_module( "ESP" );

				perf::profiler::get( ).begin_module( "Aimbot" );
				features::combat::g_legit.on_render( draw_list );
				perf::profiler::get( ).end_module( "Aimbot" );
			}

			if ( settings::g_misc.watermark )
			{
				this->draw_watermark( draw_list, fps_value );
			}

			if ( settings::g_misc.spectator_warning && systems::g_local.is_being_spectated( ) )
			{
				this->draw_spectator_warning( draw_list );
			}

			this->draw_perf_overlay( draw_list );

			g::menu.draw( );
		}
		zdraw::end_frame( );
		perf::profiler::get( ).end_module( "Rendering" );

		if ( FAILED( this->m_swap_chain->Present( 0, 0 ) ) )
		{
			threads::request_stop( );
			break;
		}

		::DwmFlush( );

		if ( settings::g_misc.limit_fps )
		{
			const auto fps_cap = std::clamp( settings::g_misc.fps_limit, constants::render::k_min_fps_cap, constants::render::k_max_fps_cap );
			static timing::limiter limiter( settings::g_misc.fps_limit );
			limiter.set_target( fps_cap );
			limiter.limit( );
		}
	}
}

void render::shutdown( )
{
	if ( this->m_rtv )
	{
		this->m_rtv->Release( );
		this->m_rtv = nullptr;
	}

	if ( this->m_swap_chain )
	{
		this->m_swap_chain->Release( );
		this->m_swap_chain = nullptr;
	}

	if ( this->m_context )
	{
		this->m_context->Release( );
		this->m_context = nullptr;
	}

	if ( this->m_device )
	{
		this->m_device->Release( );
		this->m_device = nullptr;
	}

	if ( this->m_hwnd )
	{
		::DestroyWindow( this->m_hwnd );
		this->m_hwnd = nullptr;
	}

	if ( this->m_atom )
	{
		::UnregisterClassW( k_class_name, ::GetModuleHandleW( nullptr ) );
		this->m_atom = 0;
	}
}

void render::draw_watermark( zdraw::draw_list& draw_list, int fps ) const
{
	zdraw::push_font( this->m_fonts.mochi_12 ? this->m_fonts.mochi_12 : zdraw::get_font( ) );

	const auto text = std::format( "velora | {} fps", std::max( fps, 0 ) );
	const auto [text_w, text_h] = zdraw::measure_text( text );

	constexpr auto x = 14.0f;
	constexpr auto y = 14.0f;
	constexpr auto pad_x = 8.0f;
	constexpr auto pad_y = 4.0f;

	const auto box_w = text_w + pad_x * 2.0f;
	const auto box_h = text_h + pad_y * 2.0f;

	draw_list.add_rect_filled( x, y, box_w, box_h, zdraw::rgba{ 11, 10, 16, 190 } );
	draw_list.add_rect( x, y, box_w, box_h, zdraw::rgba{ 164, 110, 188, 210 } );
	draw_list.add_text( x + pad_x, y + pad_y, text, nullptr, zdraw::rgba{ 238, 221, 244, 255 }, zdraw::text_style::outlined );

	zdraw::pop_font( );
}

void render::draw_spectator_warning( zdraw::draw_list& draw_list ) const
{
	zdraw::push_font( this->m_fonts.mochi_12 ? this->m_fonts.mochi_12 : zdraw::get_font( ) );

	const auto spectate_count = systems::g_local.spectator_count( );
	if ( spectate_count <= 0 )
	{
		zdraw::pop_font( );
		return;
	}

	const auto text = spectate_count == 1
		? std::string{ "WARNING: 1 PLAYER IS SPECTATING YOU" }
		: std::format( "WARNING: {} PLAYERS ARE SPECTATING YOU", spectate_count );

	const auto [text_w, text_h] = zdraw::measure_text( text );
	const auto [screen_w, screen_h] = zdraw::get_display_size( );
	const auto x = ( screen_w - text_w ) * 0.5f;
	const auto y = screen_h * 0.78f;
	constexpr auto pad_x = 14.0f;
	constexpr auto pad_y = 7.0f;

	draw_list.add_rect_filled( x - pad_x, y - pad_y, text_w + pad_x * 2.0f, text_h + pad_y * 2.0f, zdraw::rgba{ 38, 8, 8, 175 } );
	draw_list.add_rect( x - pad_x, y - pad_y, text_w + pad_x * 2.0f, text_h + pad_y * 2.0f, zdraw::rgba{ 200, 58, 58, 220 } );
	draw_list.add_text( x, y, text, nullptr, zdraw::rgba{ 255, 168, 168, 255 }, zdraw::text_style::outlined );

	zdraw::pop_font( );
}

void render::update_input_window( )
{
	if ( !this->m_hwnd )
	{
		return;
	}

	const auto want_input = g::menu.is_open( );
	if ( want_input == this->m_input_visible )
	{
		return;
	}

	const auto style = ::GetWindowLongW( this->m_hwnd, GWL_EXSTYLE );
	const auto next_style = want_input ? ( style & ~WS_EX_TRANSPARENT ) : ( style | WS_EX_TRANSPARENT );
	if ( next_style != style )
	{
		::SetWindowLongW( this->m_hwnd, GWL_EXSTYLE, next_style );
	}
	this->m_input_visible = want_input;
}

bool render::setup_d3d( )
{
	DXGI_SWAP_CHAIN_DESC desc{};
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = this->m_hwnd;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL levels[ ]{ D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL selected{};

	if ( FAILED( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED, levels, 2, D3D11_SDK_VERSION, &desc, &this->m_swap_chain, &this->m_device, &selected, &this->m_context ) ) )
	{
		return false;
	}

	if ( IDXGIDevice1* dxgi_device{}; SUCCEEDED( this->m_device->QueryInterface( IID_PPV_ARGS( &dxgi_device ) ) ) )
	{
		dxgi_device->SetMaximumFrameLatency( 1 );
		dxgi_device->Release( );
	}

	ID3D11Texture2D* back_buffer{};
	if ( FAILED( this->m_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) ) ) )
	{
		return false;
	}

	const auto hr = this->m_device->CreateRenderTargetView( back_buffer, nullptr, &this->m_rtv );

	D3D11_TEXTURE2D_DESC bb_desc{};
	back_buffer->GetDesc( &bb_desc );
	back_buffer->Release( );

	if ( FAILED( hr ) )
	{
		return false;
	}

	D3D11_VIEWPORT vp{};
	vp.Width = static_cast< float >( bb_desc.Width );
	vp.Height = static_cast< float >( bb_desc.Height );
	vp.MaxDepth = 1.0f;
	this->m_context->RSSetViewports( 1, &vp );

	if ( !zdraw::initialize( this->m_device, this->m_context ) )
	{
		return false;
	}

	zui::initialize( this->m_hwnd );

	{
		this->m_fonts.mochi_12 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::mochi ), sizeof( resources::fonts::mochi ) ), 12.0f, 512, 512 );
		this->m_fonts.pretzel_12 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::pretzel ), sizeof( resources::fonts::pretzel ) ), 12.0f, 512, 512 );
		this->m_fonts.pixel7_10 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::pixel7 ), sizeof( resources::fonts::pixel7 ) ), 10.0f, 512, 512 );
		this->m_fonts.weapons_15 = zdraw::add_font_from_memory( std::span( reinterpret_cast< const std::byte* >( resources::fonts::weapons ), sizeof( resources::fonts::weapons ) ), 16.0f, 512, 512 );
	}

	return true;
}

LRESULT CALLBACK render::wnd_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	zui::process_wndproc_message( msg, wp, lp );

	if ( msg == WM_DESTROY )
	{
		::PostQuitMessage( 0 );
		return 0;
	}

	return ::DefWindowProcW( hwnd, msg, wp, lp );
}

void render::draw_perf_overlay( zdraw::draw_list& draw_list )
{
	if ( !settings::g_misc.perf_overlay )
		return;

	zdraw::push_font( this->m_fonts.mochi_12 ? this->m_fonts.mochi_12 : zdraw::get_font( ) );

	const auto& perf = perf::profiler::get( );
	const auto fps = perf.current_fps( );
	const auto frame_ms = perf.current_frame_ms( );
	const auto fps_avg = static_cast< int >( perf.fps_average( ) );
	const auto fps_min = static_cast< int >( perf.fps_min( ) );
	const auto fps_max = static_cast< int >( perf.fps_max( ) );
	const auto cpu = perf.get_cpu_usage( );
	const auto mem = perf.get_memory( );

	const auto [screen_w, screen_h] = zdraw::get_display_size( );

	const float panel_w = 320.0f;
	const float panel_h = 280.0f;
	float x = screen_w - panel_w - 10.0f;
	float y = 60.0f;

	draw_list.add_rect_filled( x, y, x + panel_w, y + panel_h, zdraw::rgba{ 8, 8, 10, 220 } );
	draw_list.add_rect( x, y, x + panel_w, y + panel_h, zdraw::rgba{ 100, 100, 120, 200 } );

	float text_x = x + 10.0f;
	float text_y = y + 10.0f;
	float line_h = 14.0f;

	zdraw::rgba fps_color = fps >= 120 ? zdraw::rgba{ 100, 255, 100, 255 } :
							fps >= 60 ? zdraw::rgba{ 255, 255, 100, 255 } :
							zdraw::rgba{ 255, 100, 100, 255 };

	draw_list.add_text( text_x, text_y, "VELORA PERF", nullptr, zdraw::rgba{ 180, 130, 220, 255 }, zdraw::text_style::outlined );
	text_y += line_h + 4.0f;

	draw_list.add_text( text_x, text_y, std::format( "FPS: {} (avg: {}, min: {}, max: {})", fps, fps_avg, fps_min, fps_max ), nullptr, fps_color, zdraw::text_style::outlined );
	text_y += line_h;

	draw_list.add_text( text_x, text_y, std::format( "Frame: {:.2f}ms | CPU: {:.1f}%", frame_ms, cpu ), nullptr, zdraw::rgba{ 200, 200, 200, 255 }, zdraw::text_style::outlined );
	text_y += line_h;

	const auto mem_used = mem.ullTotalPhys - mem.ullAvailPhys;
	const auto mem_total = mem.ullTotalPhys;
	const auto mem_free_pct = mem.ullTotalPhys > 0 ? ( mem.ullAvailPhys * 100.0 / mem.ullTotalPhys ) : 0.0;
	draw_list.add_text( text_x, text_y, std::format( "RAM: {:.1f}/{:.1f}GB ({:.0f}% free)",
		mem_used / ( 1024.0f * 1024.0f * 1024.0f ),
		mem_total / ( 1024.0f * 1024.0f * 1024.0f ),
		mem_free_pct ), nullptr, zdraw::rgba{ 200, 200, 200, 255 }, zdraw::text_style::outlined );
	text_y += line_h + 6.0f;

	draw_list.add_text( text_x, text_y, "- RUNTIME DATA -", nullptr, zdraw::rgba{ 150, 150, 180, 255 }, zdraw::text_style::outlined );
	text_y += line_h;

	const auto players = systems::g_collector.players( );
	const auto items = systems::g_collector.items( );
	const auto projectiles = systems::g_collector.projectiles( );

	const auto view_origin = systems::g_view.origin( );
	const auto view_angles = systems::g_view.angles( );
	const auto has_camera = systems::g_view.has_camera( );

	draw_list.add_text( text_x, text_y, std::format( "Players: {} | Items: {} | Projectiles: {}", 
		(int)players->size( ), (int)items->size( ), (int)projectiles->size( ) ), nullptr, zdraw::rgba{ 200, 200, 200, 255 }, zdraw::text_style::outlined );
	text_y += line_h;

	draw_list.add_text( text_x, text_y, std::format( "View: ({:.1f}, {:.1f}, {:.1f})", 
		view_origin.x, view_origin.y, view_origin.z ), nullptr, zdraw::rgba{ 180, 180, 180, 255 }, zdraw::text_style::outlined );
	text_y += line_h;

	draw_list.add_text( text_x, text_y, std::format( "Angles: ({:.1f}, {:.1f}) | Camera: {}", 
		view_angles.x, view_angles.y, has_camera ? "YES" : "NO" ), nullptr, zdraw::rgba{ 180, 180, 180, 255 }, zdraw::text_style::outlined );
	text_y += line_h;

	const auto local_pawn = systems::g_local.pawn( );
	const auto local_alive = systems::g_local.alive( );
	const auto local_valid = systems::g_local.valid( );
	draw_list.add_text( text_x, text_y, std::format( "Local: pawn=0x{:X} alive={} valid={}", 
		local_pawn, local_alive ? "YES" : "NO", local_valid ? "YES" : "NO" ), nullptr, zdraw::rgba{ 180, 180, 180, 255 }, zdraw::text_style::outlined );
	text_y += line_h + 6.0f;

	draw_list.add_text( text_x, text_y, "- MODULE TIMINGS -", nullptr, zdraw::rgba{ 150, 150, 180, 255 }, zdraw::text_style::outlined );
	text_y += line_h;

	struct module_info {
		std::string name;
		float time;
		float avg;
	};
	std::vector<module_info> modules;

	const char* module_names[] = { "View Update", "ESP", "Combat", "Entity Loop", "Rendering" };
	for ( const auto& name : module_names ) {
		const auto stats = perf.get_module_stats_copy( name );
		if ( stats.has_value( ) && stats->samples( ) > 0 ) {
			modules.push_back( { name, stats->current( ), stats->average( ) } );
		}
	}

	std::sort( modules.begin( ), modules.end( ), []( const auto& a, const auto& b ) { return a.time > b.time; } );

	for ( const auto& mod : modules ) {
		zdraw::rgba mod_color = mod.time > 5.0f ? zdraw::rgba{ 255, 100, 100, 255 } :
							   mod.time > 2.0f ? zdraw::rgba{ 255, 255, 100, 255 } :
							   zdraw::rgba{ 100, 255, 100, 255 };

		draw_list.add_text( text_x + 5.0f, text_y, std::format( "{}: {:.2f}ms (avg: {:.2f})", mod.name, mod.time, mod.avg ), nullptr, mod_color, zdraw::text_style::outlined );
		text_y += line_h;
	}

	if ( fps < perf.fps_warning_threshold( ) ) {
		draw_list.add_text( x + panel_w / 2.0f - 30.0f, y + panel_h - 20.0f, "FPS DROP!", nullptr, zdraw::rgba{ 255, 50, 50, 255 }, zdraw::text_style::outlined );
	}

	zdraw::pop_font( );
}

void render::draw_perf_graph( zdraw::draw_list& draw_list, float x, float y, float w, float h, const std::vector<float>& data, zdraw::rgba color )
{
	if ( data.empty( ) ) return;

	draw_list.add_rect( x, y, x + w, y + h, zdraw::rgba{ 50, 50, 50, 150 } );

	const float step = w / static_cast< float >( data.size( ) );
	float prev_x = x;
	float prev_y = y + h;

	for ( std::size_t i = 0; i < data.size( ); ++i ) {
		const float curr_x = x + i * step;
		float normalized = data[ i ] / 144.0f;
		normalized = std::clamp( normalized, 0.0f, 1.0f );
		const float curr_y = y + h - normalized * h;

		if ( i > 0 ) {
			draw_list.add_line( prev_x, prev_y, curr_x, curr_y, color );
		}

		prev_x = curr_x;
		prev_y = curr_y;
	}
}
