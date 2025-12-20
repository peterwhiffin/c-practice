#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include "cglm/types.h"
#include "glad/glad.h"
#include "cglm/struct.h"
#include "SDL3/SDL.h"

#define SHADER_PATH "../../src/shaders/"
typedef int get_color(float *f);
get_color *helper;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct vertex {
	vec3s pos;
	vec3s normal;
	vec2s uv;
};

struct transform {
	vec3s pos;
	versors rot;
	vec3s scale;
	mat4s world_transform;
};

struct camera {
	struct transform transform;
	mat4s proj;
	mat4s view;
	mat4s viewProj;
	float fov;
	float near;
	float far;
};

struct input {
	const bool *keyStates;
	vec2s movement;
	vec2s cursorPosition;
	float lookX;
	float lookY;
	float oldX;
	float oldY;
	bool esc;
	bool f;
	bool space;
	bool mouse0;
	bool mouse1;
	bool del;
};

struct file_info {
	char full_path[512];
	char file_name[128];
	char extension[128];
	char name[128];
};

struct texture {
	char path[512];
	char filename[256];
	char name[256];
	char ext[128];
	GLuint tex_id;
	GLenum format;
	GLenum internal_format;
	GLenum data_type;
	GLint wrap_type;
	GLint filter_type;
	GLuint width;
	GLuint height;
};

struct material {
	char name[256];
	GLuint shader;
	struct texture *tex;
};

struct sub_mesh {
	struct material *mat;
	GLuint vao;
	GLintptr ind_offset;
	GLuint ind_count;
};

struct mesh {
	char name[256];
	struct sub_mesh *sub_meshes;
	u32 num_sub_meshes;
};

struct renderer {
	struct mesh *mesh;
	float rot[3];
};

struct mesh_info {
	char name[256];
	struct material *mats[12];
	size_t num_mats;
};

struct resources {
	struct renderer quad;
	struct mesh *meshes;
	struct texture *textures;
	struct material all_mats[512];
	struct mesh_info *mesh_infos;
	size_t num_textures;
	size_t num_models;
	size_t num_mats;
	size_t num_mesh_infos;
	GLuint shader;
};

struct render_state {
	float clear_color[4];
	float clear_depth;
	float light_active;
	bool can_switch_light;
};

struct window {
	SDL_Window *sdl_win;
	SDL_GLContext ctx;
	bool should_close;
};

struct scene {
	struct camera scene_cam;
	time_t last_time;
	float time;
	float dt;
	float model_timer;
	int current_model;
	vec3s rot;
	bool can_switch;
};
