#pragma once

namespace settings {

	struct combat
	{
		struct aimbot
		{
			bool enabled{ true };
			int key{ VK_XBUTTON2 };

			int fov{ 5 };
			int smoothing{ 5 };

			bool head_only{ true };
			bool visible_only{ true };

			bool draw_fov{ true };
			zdraw::rgba fov_color{ 225, 225, 225, 125 };

			bool predictive{ true };
		};

		struct triggerbot
		{
			bool enabled{ true };
			int key{ VK_XBUTTON2 };

			float hitchance{ 75.0f };
			int delay{ 10 };

			bool predictive{ true };
		};

		struct other
		{
		};

		struct group_config
		{
			aimbot aimbot{};
			triggerbot triggerbot{};
			other other{};
		};

		static constexpr std::uint32_t k_group_count{ 6 };

		std::array<group_config, k_group_count> groups{};

		group_config& get( std::uint32_t weapon_type )
		{
			const auto idx = weapon_type - cstypes::pistol;
			return this->groups[ idx < k_group_count ? idx : 2 ];
		}

		const group_config& get( std::uint32_t weapon_type ) const
		{
			const auto idx = weapon_type - cstypes::pistol;
			return this->groups[ idx < k_group_count ? idx : 2 ];
		}
	};

	struct esp
	{
		struct player
		{
			bool enabled{ true };

			struct box
			{
				enum class style_type : std::uint8_t { full, cornered };

				bool enabled{ true };
				style_type style{ style_type::full };
				bool fill{ true };
				bool outline{ true };
				float corner_length{ 10.0f };

				zdraw::rgba visible_color{ 255, 114, 214, 255 };
				zdraw::rgba occluded_color{ 156, 96, 180, 190 };
			} m_box{};

			struct skeleton
			{
				bool enabled{ true };
				float thickness{ 1.0f };

				zdraw::rgba visible_color{ 244, 176, 236, 255 };
				zdraw::rgba occluded_color{ 162, 118, 186, 190 };
			} m_skeleton{};

			struct hitboxes
			{
				bool enabled{ false };

				zdraw::rgba visible_color{ 255, 124, 220, 22 };
				zdraw::rgba occluded_color{ 164, 108, 192, 18 };

				bool fill{ true };
				bool outline{ true };
			} m_hitboxes{};

			struct health_bar
			{
				enum class position_type : std::uint8_t { left, top, bottom };

				bool enabled{ true };
				position_type position{ position_type::left };
				bool outline{ true };
				bool gradient{ true };
				bool show_value{ true };

				zdraw::rgba full_color{ 255, 132, 221, 255 };
				zdraw::rgba low_color{ 172, 84, 176, 255 };
				zdraw::rgba background_color{ 23, 14, 32, 160 };
				zdraw::rgba outline_color{ 23, 14, 32, 255 };
				zdraw::rgba text_color{ 238, 221, 241, 255 };
			} m_health_bar{};

			struct ammo_bar
			{
				enum class position_type : std::uint8_t { left, top, bottom };

				bool enabled{ true };
				position_type position{ position_type::bottom };
				bool outline{ true };
				bool gradient{ true };
				bool show_value{ false };

				zdraw::rgba full_color{ 255, 132, 221, 255 };
				zdraw::rgba low_color{ 172, 84, 176, 255 };
				zdraw::rgba background_color{ 23, 14, 32, 160 };
				zdraw::rgba outline_color{ 23, 14, 32, 255 };
				zdraw::rgba text_color{ 238, 221, 241, 255 };
			} m_ammo_bar{};

			struct info_flags
			{
				enum flag : std::uint8_t
				{
					none = 0,
					money = 1 << 0,
					armor = 1 << 1,
					kit = 1 << 2,
					scoped = 1 << 3,
					defusing = 1 << 4,
					flashed = 1 << 5,
					ping = 1 << 6,
					distance = 1 << 7
				};

				bool enabled{ true };
				std::uint8_t flags{ flag::money | flag::armor | flag::kit | flag::scoped | flag::defusing | flag::flashed | flag::ping };

				zdraw::rgba money_color{ 255, 178, 233, 255 };
				zdraw::rgba armor_color{ 236, 214, 242, 255 };
				zdraw::rgba kit_color{ 224, 137, 209, 255 };
				zdraw::rgba scoped_color{ 248, 198, 239, 255 };
				zdraw::rgba defusing_color{ 219, 120, 199, 255 };
				zdraw::rgba flashed_color{ 255, 220, 247, 255 };
				zdraw::rgba distance_color{ 162, 129, 178, 255 };

				[[nodiscard]] bool has( flag f ) const { return this->flags & f; }
			} m_info_flags{};

			struct name
			{
				bool enabled{ true };
				zdraw::rgba color{ 238, 221, 241, 235 };
			} m_name{};

			struct weapon
			{
				enum class display_type : std::uint8_t { text, icon, text_and_icon };

				bool enabled{ true };
				display_type display{ display_type::icon };

				zdraw::rgba text_color{ 238, 221, 241, 230 };
				zdraw::rgba icon_color{ 255, 255, 255, 255 };
			} m_weapon{};
		} m_player{};

		struct item
		{
			bool enabled{ true };
			float max_distance{ 40.0f };

			struct icon
			{
				bool enabled{ true };
				zdraw::rgba color{ 255, 255, 255, 255 };
			} m_icon{};

			struct name
			{
				bool enabled{ false };
				zdraw::rgba color{ 233, 210, 238, 210 };
			} m_name{};

			struct ammo
			{
				bool enabled{ true };
				zdraw::rgba color{ 224, 131, 206, 220 };
				zdraw::rgba empty_color{ 164, 96, 136, 210 };
			} m_ammo{};

			struct filters
			{
				bool rifles{ true };
				bool smgs{ true };
				bool shotguns{ true };
				bool snipers{ true };
				bool pistols{ true };
				bool heavy{ true };
				bool grenades{ true };
				bool utility{ true };
			} m_filters{};
		} m_item{};

		struct projectile
		{
			bool enabled{ true };

			bool show_icon{ true };
			bool show_name{ true };
			bool show_timer_bar{ true };
			bool show_inferno_bounds{ true };

			zdraw::rgba default_color{ 236, 204, 242, 215 };
			zdraw::rgba color_he{ 247, 142, 218, 230 };
			zdraw::rgba color_flash{ 255, 194, 240, 235 };
			zdraw::rgba color_smoke{ 208, 164, 232, 230 };
			zdraw::rgba color_molotov{ 255, 126, 196, 230 };
			zdraw::rgba color_decoy{ 196, 152, 221, 220 };

			zdraw::rgba timer_high_color{ 255, 132, 221, 255 };
			zdraw::rgba timer_low_color{ 174, 84, 177, 255 };
			zdraw::rgba bar_background{ 23, 14, 32, 160 };
		} m_projectile{};
	};

	struct misc
	{
		struct grenades
		{
			bool enabled{ true };
			bool local_only{ true };
			zdraw::rgba color{ 170, 175, 220, 200 };
		} m_grenades{};

		bool spectator_warning{ false };
		bool watermark{ true };
		bool perf_overlay{ false };
		bool dev_logging{ false };

		bool limit_fps{ false };
		int fps_limit{ 240 };
	};

	inline combat g_combat{};
	inline esp g_esp{};
	inline misc g_misc{};

} // namespace settings
