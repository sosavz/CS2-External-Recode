#pragma once

namespace threads {

	void game( );
	void combat( );
	void request_stop( ) noexcept;
	void reset_stop( ) noexcept;
	[[nodiscard]] bool should_stop( ) noexcept;

} // namespace threads
