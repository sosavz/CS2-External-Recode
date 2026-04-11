#pragma once

namespace constants {

	namespace aimbot {
		constexpr float k_mouse_yaw_factor{ 0.022f };
		constexpr float k_max_aim_range{ 8192.0f };
		constexpr float k_capsule_radius{ 3.5f };
		constexpr float k_min_smoothing{ 0.0f };
		constexpr float k_max_smoothing{ 50.0f };
		constexpr float k_min_fov{ 1.0f };
		constexpr float k_max_fov{ 45.0f };
	}

	namespace esp {
		constexpr float k_max_esp_distance{ 150.0f };
		constexpr float k_max_bone_distance{ 50.0f };
		constexpr float k_desaturation_factor{ 0.7f };
		constexpr float k_damage_flash_duration{ 2.5f };
		constexpr float k_min_player_distance{ 1.0f };
		constexpr float k_distance_scaling{ 0.01905f };
	}

	namespace render {
		constexpr int k_fps_window_ms{ 250 };
		constexpr int k_min_fps_cap{ 60 };
		constexpr int k_max_fps_cap{ 1000 };
		constexpr int k_default_fps_cap{ 240 };
		constexpr int k_bone_count{ 28 };
		constexpr int k_max_bone_index{ 128 };
	}

	namespace entities {
		constexpr int k_max_entity_slots{ 2048 };
		constexpr int k_entity_chunk_size{ 512 };
		constexpr int k_entity_handle_index_mask{ 0x1FF };
		constexpr int k_entity_chunk_index_shift{ 9 };
		constexpr int k_entity_handle_chunk_shift{ 9 };
		constexpr int k_entity_handle_chunk_mask{ 0x7FFF };
		constexpr int k_entity_entry_size{ 112 };
		constexpr int k_entity_list_pointer_offset{ 0x10 };
	}

	namespace bvh {
		constexpr int k_max_leaf_triangles{ 8 };
		constexpr int k_max_build_depth{ 48 };
		constexpr size_t k_inner_node_size{ 32 };
		constexpr size_t k_outer_node_size{ 48 };
	}

	namespace schema {
		constexpr int k_class_name_buffer_size{ 64 };
		constexpr int k_player_name_buffer_size{ 128 };
		constexpr int k_weapon_name_buffer_size{ 64 };
	}

}
