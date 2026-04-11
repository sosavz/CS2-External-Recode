#pragma once

namespace systems {

	class convars
	{
	public:
		[[nodiscard]] std::uintptr_t find( std::uint32_t name_hash );

		template<typename T>
		[[nodiscard]] T get( std::uintptr_t cvar_ptr );
	};

	class schemas
	{
	public:
		[[nodiscard]] std::int32_t lookup( const char* class_name, std::uint32_t field_hash );

	private:
		struct cached_entry
		{
			std::uint32_t field_hash;
			std::int32_t offset;
		};

		[[nodiscard]] std::uintptr_t find_class_binding( const char* class_name );
		[[nodiscard]] std::uint32_t murmur2( const char* str );
		[[nodiscard]] std::uint8_t bucket_index( std::uint32_t hash );

		std::uintptr_t m_schema_system{};
		std::uintptr_t m_client_scope{};
	};

	class entities
	{
	public:
		enum class type : std::uint8_t
		{
			unknown,
			player,
			item,
			projectile
		};

		struct cached
		{
			std::uintptr_t ptr{};
			std::uint32_t schema_hash{};
			std::int16_t index{};
			type type{ type::unknown };
		};

		void refresh( );

		[[nodiscard]] std::uintptr_t lookup( std::uint32_t handle ) const;
		[[nodiscard]] std::vector<cached> by_type( type filter ) const;
		[[nodiscard]] std::shared_ptr<const std::vector<cached>> all( ) const;

	private:
		struct slot_cache
		{
			std::uintptr_t ptr{};
			std::uint32_t schema_hash{};
			type type{ type::unknown };
		};

		[[nodiscard]] std::uintptr_t get_entity_list( ) const;
		[[nodiscard]] std::uintptr_t get_by_index( std::uintptr_t entity_list, std::int32_t index ) const;
		[[nodiscard]] std::uint32_t get_schema_hash( std::uintptr_t entity ) const;
		[[nodiscard]] type classify( std::uint32_t schema_hash ) const;

		std::shared_ptr<const std::vector<cached>> m_entities{ std::make_shared<const std::vector<cached>>( ) };
		std::array<slot_cache, constants::entities::k_max_entity_slots> m_slot_cache{};
		mutable std::shared_mutex m_mutex{};
		std::uint32_t m_frame_counter{ 0 };
	};

	class local
	{
	public:
		void update( );

		[[nodiscard]] std::uintptr_t controller( ) const;
		[[nodiscard]] std::uintptr_t pawn( ) const;
		[[nodiscard]] std::int32_t team( ) const;
		[[nodiscard]] bool valid( ) const;
		[[nodiscard]] bool alive( ) const;
		[[nodiscard]] std::uintptr_t view_pawn( ) const;

		[[nodiscard]] std::uintptr_t weapon( ) const;
		[[nodiscard]] std::uintptr_t weapon_vdata( ) const;
		[[nodiscard]] std::uint32_t weapon_type( ) const;

		[[nodiscard]] bool is_enemy( std::int32_t other_team ) const
		{
			std::shared_lock lock( this->m_mutex );
			if ( !this->m_state.team_mode )
			{
				return true;
			}

			return this->m_state.view_team != other_team;
		}

		[[nodiscard]] bool is_being_spectated( ) const;
		[[nodiscard]] int spectator_count( ) const;

	private:
		struct state
		{
			std::uintptr_t controller{};
			std::uintptr_t pawn{};
			std::uintptr_t observer_pawn{};
			std::int32_t team{};
			std::int32_t view_team{};
			bool alive{};
			bool team_mode{ true };
			std::uintptr_t weapon{};
			std::uintptr_t weapon_vdata{};
			std::uint32_t weapon_type{};
			int spectator_count{};
		};

		void publish( state next );
		void reset( );

		mutable std::shared_mutex m_mutex{};
		state m_state{};
	};

	class view
	{
	public:
		void update( );

		[[nodiscard]] math::vector2 project( const math::vector3& world_pos );
		[[nodiscard]] bool projection_valid( const math::vector2& screen_pos ) const { return screen_pos.x != this->k_invalid && screen_pos.y != this->k_invalid; }
		[[nodiscard]] bool has_camera( ) const;
		[[nodiscard]] math::vector3 origin( ) const;
		[[nodiscard]] math::vector3 angles( ) const;
		[[nodiscard]] float fov( ) const;

	private:
		static constexpr auto k_invalid{ 0xdead };
		std::atomic< std::uintptr_t > m_view_render{};

		math::matrix4x4 m_matrix{};
		math::vector3 m_origin{};
		math::vector3 m_angles{};
		float m_fov{ 0.0f };
		mutable std::shared_mutex m_mutex{};
	};

	class bones
	{
	public:
		struct data
		{
			struct bone
			{
				math::vector3 position{};
				math::quaternion rotation{};
			};

			std::array<bone, static_cast< std::size_t >( 28 )> bones{};

			[[nodiscard]] bool is_valid( ) const;
			[[nodiscard]] math::vector3 get_position( std::uint32_t id ) const;
			[[nodiscard]] math::quaternion get_rotation( std::uint32_t id ) const;
		};

		[[nodiscard]] data get( std::uintptr_t bone_array ) const;
		[[nodiscard]] std::optional<math::vector3> get_head_position( std::uintptr_t bone_cache ) const;
	};

	class bounds
	{
	public:
		struct data
		{
			math::vector2 min{};
			math::vector2 max{};

			[[nodiscard]] bool is_valid( ) const;
			[[nodiscard]] float width( ) const { return this->max.x - this->min.x; }
			[[nodiscard]] float height( ) const { return this->max.y - this->min.y; }
		};

		[[nodiscard]] data get( const bones::data& bone_data ) const;
	};

	class hitboxes
	{
	public:
		struct entry
		{
			int index{ -1 };
			int bone{ -1 };
			math::vector3 mins{};
			math::vector3 maxs{};
			float radius{};
		};

		struct set
		{
			std::array<entry, 20> entries{};
			int count{};

			[[nodiscard]] const entry* begin( ) const { return this->entries.data( ); }
			[[nodiscard]] const entry* end( ) const { return this->entries.data( ) + this->count; }
		};

		[[nodiscard]] set query( std::uintptr_t game_scene_node ) const;
		[[nodiscard]] int hitgroup_from_hitbox( int hitbox ) const;

	private:
		static constexpr int k_bone_map[ ]{ 6, -1, 0, 1, 2, 3, 4, 22, 25, 23, 26, 24, 27, 10, 15, 9, 8, 14, 13 };
	};

	class collector
	{
	public:
		enum class item_subtype : std::uint8_t
		{
			unknown = 0,
			ak47, m4a4, m4a1s, awp, aug, famas, galil_ar, sg553,
			g3sg1, scar20, ssg08,
			mac10, mp5sd, mp7, mp9, pp_bizon, p90, ump45,
			nova, sawed_off, xm1014, mag7,
			m249, negev,
			deagle, dual_berettas, five_seven, glock, p2000, usps, p250, cz75, tec9, r8_revolver,
			taser, knife, c4, healthshot,
			he_grenade, flashbang, smoke_grenade, molotov, incendiary, decoy
		};

		enum class projectile_subtype : std::uint8_t
		{
			unknown = 0,
			he_grenade, flashbang, smoke_grenade, molotov, molotov_fire, decoy
		};

		struct weapon_info
		{
			std::uintptr_t ptr{};
			std::uintptr_t vdata{};
			std::string name{};
			std::int32_t ammo{};
			std::int32_t max_ammo{};
		};

		struct player
		{
			std::uintptr_t controller{};
			std::uintptr_t pawn{};
			std::uintptr_t game_scene_node{};
			std::uintptr_t bone_cache{};
			math::vector3 origin{};
			std::string display_name{};
			weapon_info weapon{};
			std::int32_t health{};
			std::int32_t team{};
			std::int32_t money{};
			std::int32_t ping{};
			std::int32_t armor{};
			bool invulnerable{};
			bool has_helmet{};
			bool has_defuser{};
			bool is_scoped{};
			bool is_defusing{};
			bool is_flashed{};
			bool is_visible{};
			hitboxes::set hitboxes{};
		};

		struct item
		{
			std::uintptr_t entity{};
			std::uintptr_t game_scene_node{};
			math::vector3 origin{};
			item_subtype subtype{ item_subtype::unknown };
			std::int32_t ammo{};
			std::int32_t max_ammo{};
		};

		struct projectile
		{
			std::uintptr_t entity{};
			std::uintptr_t game_scene_node{};
			math::vector3 origin{};
			math::vector3 velocity{};
			projectile_subtype subtype{ projectile_subtype::unknown };
			std::uint32_t thrower_handle{};
			std::int32_t bounces{};
			std::int32_t effect_tick_begin{};
			bool detonated{};
			bool smoke_active{};
			std::vector<math::vector3> fire_points{};
			float expire_time{};
		};

		void run( );

		[[nodiscard]] std::shared_ptr<const std::vector<player>> players( ) const;
		[[nodiscard]] std::shared_ptr<const std::vector<item>> items( ) const;
		[[nodiscard]] std::shared_ptr<const std::vector<projectile>> projectiles( ) const;

	private:
		void collect_players( const std::vector<entities::cached>& raw, bool needs_visibility, bool needs_hitboxes );
		void collect_items( const std::vector<entities::cached>& raw );
		void collect_projectiles( const std::vector<entities::cached>& raw );

		[[nodiscard]] static item_subtype classify_item( std::uint32_t schema_hash );
		[[nodiscard]] static projectile_subtype classify_projectile( std::uint32_t schema_hash );

		std::shared_ptr<const std::vector<player>> m_players{ std::make_shared<const std::vector<player>>( ) };
		std::shared_ptr<const std::vector<item>> m_items{ std::make_shared<const std::vector<item>>( ) };
		std::shared_ptr<const std::vector<projectile>> m_projectiles{ std::make_shared<const std::vector<projectile>>( ) };
		mutable std::shared_mutex m_mutex{};
	};

	class bvh
	{
	public:
		struct surface_info
		{
			float penetration{};
			std::uint16_t surface_type{};
			std::uint8_t global_index{ 255 };
		};

		struct global_surface_entry
		{
			float unk_00{};
			float unk_04{};
			float penetration_mod{};
			float unk_0C{};
			float unk_10{};
			std::uint16_t surface_type{};
			std::uint16_t pad{};
			std::uint8_t pad2[ 8 ]{};
		};

		struct triangle
		{
			math::vector3 v0{};
			math::vector3 v1{};
			math::vector3 v2{};
			surface_info surface{};
		};

		struct trace_result
		{
			bool hit{};
			float fraction{};
			float distance{};
			math::vector3 end_pos{};
			math::vector3 normal{};
			surface_info surface{};
			std::int32_t triangle_index{ -1 };
		};

		struct hit_entry
		{
			float distance{};
			float fraction{};
			math::vector3 position{};
			math::vector3 normal{};
			surface_info surface{};
			std::int32_t triangle_index{ -1 };
			bool is_enter{ true };
		};

		struct penetration_segment
		{
			float enter_fraction{};
			float exit_fraction{};
			float enter_distance{};
			float exit_distance{};
			math::vector3 enter_pos{};
			math::vector3 exit_pos{};
			surface_info enter_surface{};
			surface_info exit_surface{};
			float thickness{};
			float min_pen_mod{};
		};

		void parse( );
		void clear( );

		[[nodiscard]] trace_result trace_ray( const math::vector3& start, const math::vector3& end, std::int32_t exclude_tri = -1 ) const;
		[[nodiscard]] std::vector<hit_entry> trace_ray_all( const math::vector3& start, const math::vector3& end ) const;
		[[nodiscard]] std::vector<penetration_segment> build_segments( const std::vector<hit_entry>& hits, float ray_length ) const;

		[[nodiscard]] std::vector<triangle> triangles( ) const;
		[[nodiscard]] std::size_t count( ) const;
		[[nodiscard]] bool valid( ) const;

	private:
		struct aabb
		{
			float mins[ 3 ]{ 1e12f, 1e12f, 1e12f };
			float maxs[ 3 ]{ -1e12f, -1e12f, -1e12f };

			void expand( const math::vector3& p );
			void expand( const aabb& o );
			[[nodiscard]] int longest_axis( ) const;
			[[nodiscard]] bool intersects_ray( const float origin[ 3 ], const float inv_dir[ 3 ], float max_t ) const;
		};

		struct bvh_node
		{
			aabb bounds{};
			std::int32_t left{ -1 };
			std::int32_t right{ -1 };
			std::int32_t tri_start{};
			std::int32_t tri_count{};
		};

		void rebuild_accel( );
		void rebuild_accel_unlocked( );
		std::int32_t build_recursive( std::int32_t start, std::int32_t end, std::int32_t depth );

		std::vector<triangle> m_triangles{};
		mutable std::shared_mutex m_mutex{};

		std::vector<bvh_node> m_nodes{};
		std::vector<std::int32_t> m_indices{};
		std::vector<aabb> m_tri_bounds{};
		std::vector<float> m_centroids{};

		static constexpr auto k_max_leaf_tris{ 8 };
		static constexpr auto k_max_depth{ 48 };
	};

	inline convars g_convars{};
	inline schemas g_schemas{};
	inline entities g_entities{};
	inline local g_local{};
	inline view g_view{};
	inline bones g_bones{};
	inline bounds g_bounds{};
	inline hitboxes g_hitboxes{};
	inline collector g_collector{};
	inline bvh g_bvh{};

} // namespace systems

/*
 * SCHEMA Macro:
 * - Caches field offsets using schema system
 * - Usage: SCHEMA("CCSPlayerController", "m_iPing"_hash)
 * - First call does hash lookup, subsequent calls use cached value
 * - Returns: std::int32_t offset from entity base
 *
 * CONVAR Macro:
 * - Caches convars (console variables) pointers
 * - Usage: CONVAR("sensitivity"_hash)
 * - First call does cvar lookup, subsequent calls use cached pointer
 * - Returns: std::uintptr_t pointer to convar object
 */
#define SCHEMA( class_name, field_hash ) \
	[]( ) -> std::int32_t { \
		static std::atomic<std::int32_t> val{}; \
		const auto cached = val.load( std::memory_order_acquire ); \
		if ( cached ) \
		{ \
			return cached; \
		} \
		const auto resolved = systems::g_schemas.lookup( class_name, field_hash ); \
		if ( resolved ) \
		{ \
			val.store( resolved, std::memory_order_release ); \
		} \
		return resolved; \
	}( )

#define CONVAR( name_hash ) \
	[]( ) -> std::uintptr_t { \
		static std::atomic<std::uintptr_t> val{}; \
		const auto cached = val.load( std::memory_order_acquire ); \
		if ( cached ) \
		{ \
			return cached; \
		} \
		const auto resolved = systems::g_convars.find( name_hash ); \
		if ( resolved ) \
		{ \
			val.store( resolved, std::memory_order_release ); \
		} \
		return resolved; \
	}( )
