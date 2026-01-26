#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include "SDL3/SDL_events.h"
#include "cglm/types-struct.h"
#include "cglm/types.h"
#ifdef NO_GLAD
#else
#include "glad/glad.h"
#endif
#include "ft2build.h"
#include "renderer/ufbx.h"
#include FT_FREETYPE_H
#include "cglm/struct.h"
#include "SDL3/SDL.h"

#define SHADER_PATH "../../src/shaders/"

#if defined(_WIN32)
#define PETE_API __declspec(dllexport)
#else
#define PETE_API
#endif

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
	u32 num_normals;
};

struct transform {
	struct entity *entity;
	vec3s pos;
	versors rot;
	vec3s scale;
	vec3s euler_angles;
	mat4s world_transform;
};

struct camera {
	struct entity *entity;
	mat4s proj;
	mat4s view;
	mat4s viewProj;
	float fov;
	float near_plane;
	float far_plane;
};

enum action_type { BUTTON, AXIS, COMPOSITE };

enum action_state { CANCELED = 0, STARTED, ACTIVE };

struct key_action {
	enum action_type type;
	enum action_state state;
	union {
		vec2s composite;
		float axis;
		bool pushed;
	};
};

enum key_actions {
	MWHEEL = 0,
	MOUSE_DELTA,
	WASD,
	ARROWS,
	M0,
	M1,
	SPACE,
	LSHIFT,
	LCTRL,
	DEL,
	D,
	F,
	P,
	ESC,
	ACTION_COUNT
};

struct input {
	bool (*lock_mouse)(SDL_Window *, bool);
	const bool *sdl_keys;
	vec2s cursorPosition;
	vec2s relativeCursorPosition;
	struct key_action actions[ACTION_COUNT];
};

struct file_info {
	char path[512];
	char filename[256];
	char extension[128];
	char name[256];
};

struct texture {
	struct file_info file_info;
	GLuint id;
	GLenum format;
	GLenum internal_format;
	GLenum data_type;
	GLenum attachment;
	GLint wrap_type;
	GLint filter_type;
	GLuint width;
	GLuint height;
	bool mips;
};

struct material {
	char name[256];
	GLuint shader;
	vec4s color;
	struct texture *tex;
};

struct sub_mesh {
	struct material *mat;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	GLintptr ind_offset;
	GLuint ind_count;
};

struct mesh {
	char name[256];
	struct sub_mesh *sub_meshes;
	u32 num_sub_meshes;
};

struct model_import {
	struct file_info file_info;
	struct mesh **meshes;
	u32 num_meshes;
};

struct mesh_renderer {
	struct entity *entity;
	struct mesh *mesh;
};

struct mesh_info {
	char name[256];
	struct material *mats[12];
	size_t num_mats;
	float import_scale;
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
	// GLuint font_vao;
	// GLuint font_vbo;
	struct mesh_renderer quad;
	struct mesh *meshes;
	struct font_character font_characters[128];
	struct texture *textures;
	struct texture white_tex;
	struct material all_mats[512];
	struct material *default_mat;
	struct mesh_info *default_mesh_info;
	struct model_import *model_imports;
	struct mesh_info *mesh_infos;
	size_t num_models;
	size_t num_textures;
	size_t num_meshes;
	size_t num_mats;
	size_t num_mesh_infos;
};

struct renderbuffer {
	GLuint id;
	GLenum attachment;
	GLenum internal_format;
};

struct framebuffer {
	GLuint id;
	u32 width;
	u32 height;
	struct renderbuffer rb;
	struct texture targets;
	u32 num_targets;
};

struct window {
	SDL_Window *sdl_win;
	SDL_GLContext ctx;
	vec2s size;
	float aspect;
	bool should_close;
};

struct entity {
	u32 id;
	char name[128];
	struct entity *parent;
	struct entity *children;
	struct entity *prev;
	struct entity *next;
	struct transform *transform;
	struct mesh_renderer *renderer;
	struct camera *camera;
};

enum edit_mode { DEFAULT, PLACE };

struct scene {
	char filename[512];
	struct entity *scene_cam;
	struct entity *preview_model;
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
	int text_offset;
	enum edit_mode mode;
	vec3s place_pos;
	float pitch;
	float yaw;
	u32 next_id;
};

struct texture_list {
	struct texture *first_free_tex;
	struct texture *next;
};

struct renderer {
	void *lib_handle;
	void (*load_functions)(struct renderer *, void *);
	void (*test_renderer)(struct renderer *, struct window *);
	void (*reload_renderer)(struct renderer *, struct resources *, struct arena *, struct window *);
	void (*window_resized)(struct renderer *, struct window *, struct arena *);
	void (*draw_scene)(struct renderer *, struct resources *, struct scene *, struct window *);
	void (*draw_fullscreen_quad)(struct renderer *);
	void (*init_renderer)(struct renderer *, struct arena *, struct window *);
	void (*load_resources)(struct resources *, struct renderer *, struct arena *);
	void (*scene_write)(struct scene *);
	void (*reload_shaders)(struct renderer *);
	void (*reload_model)(struct resources *, struct renderer *, struct model_import *);

	struct input *input;
	float clear_color[4];
	float clear_depth;
	float light_active;
	mat4s text_proj;
	struct window *win;
	GLuint default_shader;
	GLuint fullscreen_shader;
	GLuint text_shader;
	GLuint gui_shader;
	GLuint skybox_shader;
	GLuint quad_vao;
	GLuint quad_vbo;
	GLuint quad_ebo;

	GLuint text_quad_vao;
	GLuint text_quad_vbo;
	GLuint text_quad_ebo;
	GLuint skybox_vao;
	GLuint skybox_vbo;
	struct texture_list tex_list;
	struct framebuffer main_fbo;
	struct framebuffer final_fbo;
	struct texture *skybox_tex;
	struct texture *skybox_night_tex;
	struct texture *current_skybox;

	struct framebuffer picking_fbo;
};

struct game {
	void *lib_handle;
	void (*load_functions)(struct game *);
	void (*update)(struct scene *, struct input *, struct resources *, struct renderer *, struct window *);
	void (*init_scene)(struct scene *, struct resources *);
	void (*entity_unset_parent)(struct entity *child);
	void (*entity_set_parent)(struct entity *child, struct entity *parent);
	struct entity *(*get_new_entity)(struct scene *scene);
	struct mesh_renderer *(*add_renderer)(struct scene *scene, struct entity *entity);
	struct camera *(*add_camera)(struct scene *scene, struct entity *entity);
	struct entity *(*entity_duplicate)(struct scene *, struct entity *);
};

struct physics {
	void *lib_handle;
	void (*load_functions)(struct physics *);
	void (*step_physics)(struct physics *, float);
	float time_accum;
};

struct editor {
	struct game *game;
	struct scene *scene;
	struct renderer *ren;
	struct window *win;
	struct resources *res;
	struct input *input;
	struct entity *selected_entity;
	bool show_demo;

	vec2s image_size;
	vec2s image_pos;
	void *imgui_ctx;
	void *lib_handle;
	void (*load_functions)(struct editor *);
	void (*update_editor)(struct editor *);
	void (*process_event)(SDL_Event *);
	void (*init_editor)(struct window *, struct editor *);
};
