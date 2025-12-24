#pragma once
#include "SDL3/SDL_video.h"
#include "cglm/types-struct.h"
#define UFBX_REAL_IS_FLOAT
#define CGLM_FORCE_LEFT_HANDED
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include "glad/glad.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include "cglm/struct.h"
#include "SDL3/SDL.h"

#define SHADER_PATH "../../src/shaders/"
// typedef int get_color(float *f);
// get_color *helper;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct arena {
	u8 *mem;
	u64 pos;
	size_t size;
};

struct vertex {
	vec3s pos;
	vec3s normal;
	vec2s uv;
	uint num_normals;
};

struct transform {
	vec3s pos;
	versors rot;
	vec3s scale;
	mat4s world_transform;
};

struct camera {
	struct entity *entity;
	mat4s proj;
	mat4s view;
	mat4s viewProj;
	float fov;
	float near;
	float far;
};

enum key_type { BUTTON, AXIS, COMPOSITE };

enum input_state { CANCELED = 0, STARTED, ACTIVE };

struct key_state {
	enum key_type value_type;
	enum input_state state;
	vec2s value;
};

struct input {
	bool (*lock_mouse)(SDL_Window *, bool);
	const bool *keyStates;
	vec2s cursorPosition;
	float lookX;
	float lookY;
	float oldX;
	float oldY;
	struct key_state mouse_wheel;
	struct key_state movement;
	struct key_state arrows;
	struct key_state mouse0;
	struct key_state mouse1;
	struct key_state space;
	struct key_state left_shift;
	struct key_state del;
	struct key_state f;
	struct key_state esc;
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

struct mesh_renderer {
	struct mesh *mesh;
};

struct mesh_info {
	char name[256];
	struct material *mats[12];
	size_t num_mats;
};

struct font_character {
	struct texture *tex;
	vec2s glyph_size;
	vec2s bearing;
	u32 advance;
};

struct resources {
	FT_Library ft;
	FT_Face font_face;
	GLuint font_vao;
	GLuint font_vbo;
	struct mesh_renderer quad;
	struct mesh *meshes;
	struct font_character font_characters[128];
	struct texture *textures;
	struct texture white_tex;
	struct material all_mats[512];
	size_t num_textures;
	size_t num_models;
	size_t num_mats;
};

struct renderer {
	float clear_color[4];
	float clear_depth;
	float light_active;
	mat4s text_proj;
	GLuint shader;
	GLuint font_shader;
};

struct window {
	SDL_Window *sdl_win;
	SDL_GLContext ctx;
	bool should_close;
};

struct entity {
	struct transform *transform;
	struct mesh_renderer *renderer;
	struct camera *camera;
};

struct scene {
	struct entity *scene_cam;
	struct entity *model;
	struct transform *transforms;
	struct mesh_renderer *renderers;
	struct camera *cameras;
	struct entity *entities;
	size_t num_entities;
	size_t num_transforms;
	size_t num_renderers;
	size_t num_cameras;
	float move_speed;
	float move_mod;
	float look_sens;
	float time;
	float dt;
	time_t last_time;
	int current_model;
	GLenum draw_mode;
};

struct game {
	void *lib_handle;
	void (*load_functions)(struct game *, GLADloadproc);
	void (*update)(struct scene *, struct input *, struct resources *, struct renderer *, struct window *);
	void (*draw_scene)(struct renderer *, struct resources *, struct scene *, struct window *);
	void (*init_renderer)(struct renderer *);
	void (*init_scene)(struct scene *, struct resources *);
	void (*load_resources)(struct resources *, struct renderer *, struct arena *);
	void (*reload_shaders)(struct renderer *);
};
