#include <stdafx.hpp>

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
		return false;
	}

	g::console.print( "render initialized." );

	this->run( );

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

		// Keep projection data on the render thread as fresh as possible so ESP stays in sync with camera motion.
		systems::g_view.update( );

		this->update_input_window( );

		this->m_context->OMSetRenderTargets( 1, &this->m_rtv, nullptr );
		this->m_context->ClearRenderTargetView( this->m_rtv, clear );

		zdraw::begin_frame( );
		{
			auto& draw_list = zdraw::get_draw_list( zdraw::draw_layer::background );

			++fps_frames;
			const auto now = std::chrono::steady_clock::now( );
			const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>( now - fps_window_start ).count( );
			if ( elapsed_ms >= 250 )
			{
				fps_value = static_cast< int >( std::lround( static_cast< double >( fps_frames ) * 1000.0 / static_cast< double >( elapsed_ms ) ) );
				fps_frames = 0;
				fps_window_start = now;
			}

			if ( systems::g_local.valid( ) )
			{
				features::esp::g_player.on_render( draw_list );
				features::esp::g_item.on_render( draw_list );
				features::esp::g_projectile.on_render( draw_list );
				features::misc::g_grenades.on_render( draw_list );
				features::combat::g_legit.on_render( draw_list );
			}

			if ( settings::g_misc.watermark )
			{
				this->draw_watermark( draw_list, fps_value );
			}

			g::menu.draw( );
		}
		zdraw::end_frame( );

		if ( FAILED( this->m_swap_chain->Present( 0, 0 ) ) )
		{
			break;
		}

		::DwmFlush( );

		if ( settings::g_misc.limit_fps )
		{
			const auto fps_cap = std::clamp( settings::g_misc.fps_limit, 60, 1000 );
			static timing::limiter limiter( settings::g_misc.fps_limit );
			limiter.set_target( fps_cap );
			limiter.limit( );
		}
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

void render::update_input_window( )
{
	const auto style = ::GetWindowLongW( this->m_hwnd, GWL_EXSTYLE );

	if ( g::menu.is_open( ) )
	{
		::SetWindowLongW( this->m_hwnd, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT );
	}
	else
	{
		::SetWindowLongW( this->m_hwnd, GWL_EXSTYLE, style | WS_EX_TRANSPARENT );
	}
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
