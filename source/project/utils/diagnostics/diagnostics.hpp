#pragma once

namespace diagnostics {

	void initialize( ) noexcept;
	void set_startup_stage( const char* stage ) noexcept;
	void note_startup_failure( const char* stage, const char* reason ) noexcept;

} // namespace diagnostics
