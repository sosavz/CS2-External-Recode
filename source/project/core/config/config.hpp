#pragma once

class config_system
{
public:
	void initialize( );
	void tick( );

	bool create( std::string_view requested_name );
	bool save( std::string_view name );
	bool load( std::string_view name );
	bool remove( std::string_view name );

	void refresh( );
	void set_active( std::string name );

	[[nodiscard]] const std::vector<std::string>& names( ) const { return this->m_names; }
	[[nodiscard]] const std::string& active( ) const { return this->m_active; }
	[[nodiscard]] const std::string& last_error( ) const { return this->m_last_error; }
	[[nodiscard]] bool has_error( ) const { return !this->m_last_error.empty( ); }
	void clear_error( ) { this->m_last_error.clear( ); }

private:
	[[nodiscard]] std::filesystem::path path_for( std::string_view name ) const;
	[[nodiscard]] std::string sanitize_name( std::string_view raw ) const;
	[[nodiscard]] std::string unique_name( std::string base ) const;
	void reset_defaults( ) const;
	[[nodiscard]] std::uint64_t settings_hash( ) const;

	bool parse_json_map( const std::string& text, std::unordered_map<std::string, std::string>& out_map ) const;
	[[nodiscard]] std::string serialize_json( ) const;
	bool apply_map( const std::unordered_map<std::string, std::string>& map );
	bool save_atomic( const std::filesystem::path& target, const std::string& contents );
	bool load_with_backup( const std::filesystem::path& target, std::unordered_map<std::string, std::string>& out_map );
	void enforce_ranges( ) const;

	std::filesystem::path m_dir{};
	std::vector<std::string> m_names{};
	std::string m_active{};
	std::string m_last_error{};

	bool m_initialized{ false };
	std::uint64_t m_last_hash{ 0 };
	std::chrono::steady_clock::time_point m_last_auto_save{};
};
