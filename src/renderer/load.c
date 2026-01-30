#include "cglm/struct/vec3-ext.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#define STB_IMAGE_IMPLEMENTATION

#include "../types.h"
#include "../arena.h"
#include "freetype/freetype.h"
#include "load.h"
#include "stb_image.h"
#include "file.h"

float quad_verts[16] = {
	// clang-format off
	-1.0f, -1.0f, 0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f
	// clang-format on
};

float text_quad_verts[16] = {
	// clang-format off
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 0.0f
	// clang-format on
};

unsigned int quad_indices[6] = { 0, 1, 2, 3 };

float skybox_verts[] = {
	// clang-format off
      -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,

      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f, -1.0f,
      1.0f, 1.0f, -1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, 1.0f
	// clang-format on
};

void checkShader(GLuint shader, const char *name)
{
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(shader, 1024, NULL, log);
		printf("Shader compilation error::%s:: %s\n", name, log);
	}
}

void checkProgram(GLuint program)
{
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(program, 1024, NULL, log);
		printf("Shader link error: %s\n", log);
	}
}

GLuint load_shader(const char *vertFile, const char *fragFile)
{
	GLuint vertShader;
	GLuint fragShader;
	GLuint program;

	char vertPath[256];
	char fragPath[256];

	snprintf(vertPath, 256, "%s%s", SHADER_PATH, vertFile);
	snprintf(fragPath, 256, "%s%s", SHADER_PATH, fragFile);

	const char *vertSource = read_file(vertPath);
	const char *fragSource = read_file(fragPath);

	vertShader = glCreateShader(GL_VERTEX_SHADER);
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	program = glCreateProgram();

	glShaderSource(vertShader, 1, &vertSource, NULL);
	glShaderSource(fragShader, 1, &fragSource, NULL);
	glCompileShader(vertShader);
	glCompileShader(fragShader);

	checkShader(vertShader, vertFile);
	checkShader(fragShader, fragFile);
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);
	checkProgram(program);

	free((void *)vertSource);
	free((void *)fragSource);
	return program;
}

void load_shaders(struct renderer *ren)
{
	ren->default_shader = load_shader("default.vert", "default.frag");
	ren->text_shader = load_shader("text.vert", "text.frag");
	ren->fullscreen_shader = load_shader("fullscreen.vert", "fullscreen.frag");
	ren->gui_shader = load_shader("gui.vert", "gui.frag");
	ren->skybox_shader = load_shader("skybox.vert", "skybox.frag");
}

void reload_shaders(struct renderer *ren)
{
	glDeleteProgram(ren->default_shader);
	glDeleteProgram(ren->text_shader);
	glDeleteProgram(ren->fullscreen_shader);
	glDeleteProgram(ren->gui_shader);
	glDeleteProgram(ren->skybox_shader);
	load_shaders(ren);
}

void create_vao(struct sub_mesh *sub_mesh, struct vertex *verts, u32 *indices, size_t vert_size, size_t indices_size)
{
	GLintptr index_offset = 0;

	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glCreateBuffers(1, &ebo);

	glNamedBufferStorage(vbo, vert_size, verts, 0);
	glNamedBufferStorage(ebo, indices_size, indices, 0);

	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(struct vertex));
	glVertexArrayElementBuffer(vao, ebo);

	glEnableVertexArrayAttrib(vao, 0);
	glEnableVertexArrayAttrib(vao, 1);
	glEnableVertexArrayAttrib(vao, 2);

	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(struct vertex, normal));
	glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(struct vertex, uv));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribBinding(vao, 2, 0);

	sub_mesh->ind_count = indices_size / sizeof(u32);
	sub_mesh->ind_offset = index_offset;
	sub_mesh->vao = vao;
	sub_mesh->vbo = vbo;
	sub_mesh->ebo = ebo;
}

void create_texture(struct texture *tex, unsigned char *data)
{
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		// Process/log the error, e.g., print the error code.
		// fprintf(stderr, "OpenGL Error create texture: %d\n", error);
	}

	GLuint id;
	glCreateTextures(GL_TEXTURE_2D, 1, &id);

	glTextureParameteri(id, GL_TEXTURE_WRAP_S, tex->wrap_type);
	glTextureParameteri(id, GL_TEXTURE_WRAP_T, tex->wrap_type);
	glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, tex->filter_type);
	glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, tex->filter_type);

	glTextureStorage2D(id, 1, tex->internal_format, tex->width, tex->height);

	while ((error = glGetError()) != GL_NO_ERROR) {
		// Process/log the error, e.g., print the error code.
		fprintf(stderr, "OpenGL Error create texture: %d - name: %s\n", error, tex->file_info.name);
	}
	if (data)
		glTextureSubImage2D(id, 0, 0, 0, tex->width, tex->height, tex->format, tex->data_type, data);
	if (tex->mips)
		glGenerateTextureMipmap(id);
	tex->id = id;

	// GLenum error;
}

void create_skybox_texture(struct texture *tex, const char **paths)
{
	int x, y, channels;

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex->id);

	stbi_info(paths[0], &x, &y, &channels);
	tex->format = GL_RED;
	tex->internal_format = GL_RED;
	tex->data_type = GL_UNSIGNED_BYTE;
	tex->wrap_type = GL_CLAMP_TO_EDGE;
	tex->filter_type = GL_LINEAR;
	tex->width = x;
	tex->height = y;

	if (channels == 3) {
		tex->format = GL_RGB;
		tex->internal_format = GL_SRGB8;
	} else if (channels == 4) {
		tex->format = GL_RGBA;
		// tex->internal_format = GL_SRGB8_ALPHA8;
		tex->internal_format = GL_RGBA8;
	}

	glTextureStorage2D(tex->id, 1, tex->internal_format, x, y);

	for (int i = 0; i < 6; i++) {
		unsigned char *data = stbi_load(paths[i], &x, &y, &channels, 0);
		if (data) {
			glTextureSubImage3D(tex->id, 0, 0, 0, i, x, y, 1, tex->format, tex->data_type, data);
		}

		stbi_image_free(data);
	}

	glTextureParameteri(tex->id, GL_TEXTURE_WRAP_S, tex->wrap_type);
	glTextureParameteri(tex->id, GL_TEXTURE_WRAP_T, tex->wrap_type);
	glTextureParameteri(tex->id, GL_TEXTURE_WRAP_R, tex->wrap_type);
	glTextureParameteri(tex->id, GL_TEXTURE_MIN_FILTER, tex->filter_type);
	glTextureParameteri(tex->id, GL_TEXTURE_MAG_FILTER, tex->filter_type);
}

void load_image(char *path, struct texture *tex)
{
	int x;
	int y;
	int channels;

	stbi_uc *data = stbi_load(path, &x, &y, &channels, 0);

	if (!data) {
		printf("ERROR::STBI_IMAGE::Failed to load image %s\n", path);
		goto exit;
	}

	tex->format = GL_RED;
	tex->internal_format = GL_RED;
	tex->data_type = GL_UNSIGNED_BYTE;
	tex->wrap_type = GL_REPEAT;
	tex->filter_type = GL_NEAREST;
	tex->width = x;
	tex->height = y;

	if (channels == 3) {
		tex->format = GL_RGB;
		tex->internal_format = GL_SRGB8;
	} else if (channels == 4) {
		tex->format = GL_RGBA;
		tex->internal_format = GL_SRGB8_ALPHA8;
	}

	create_texture(tex, data);

exit:
	stbi_image_free(data);
}

struct mesh_info *get_mesh_info(struct resources *res, const char *name)
{
	struct mesh_info *mesh_info = NULL;

	for (int i = 0; i < res->num_mesh_infos; i++) {
		if (strcmp(name, res->mesh_infos[i].name) == 0) {
			return &res->mesh_infos[i];
		}
	}

	return mesh_info;
}

struct vert_group {
	struct vertex *verts[4096];
	u32 num_vertices;
	vec3s norm;
};

struct texture *find_texture(struct resources *res, const char *name)
{
	for (int i = 0; i < res->num_textures; i++) {
		struct texture *tex = &res->textures[i];
		if (strcmp(name, tex->file_info.filename) == 0) {
			printf("found texture: %s\n", name);
			return tex;
		}
	}

	printf("couldn't find texture: %s\n", name);
	return NULL;
}

struct material *get_new_material(struct resources *res, struct renderer *ren, const char *mat_name)
{
	snprintf(res->all_mats[res->num_mats].name, 128, "%s", mat_name);
	res->all_mats[res->num_mats].shader = ren->default_shader;
	res->all_mats[res->num_mats].tex = NULL;
	res->all_mats[res->num_mats].color = (vec4s){ 1.0f, 1.0f, 1.0f, 1.0f };
	res->num_mats++;
	return &res->all_mats[res->num_mats - 1];
}

struct material *find_material(struct resources *res, const char *mat_name)
{
	for (int i = 0; i < res->num_mats; i++) {
		if (strcmp(res->all_mats[i].name, mat_name) == 0) {
			return &res->all_mats[i];
		}
	}

	return NULL;
}

void create_mesh(struct resources *res, struct renderer *ren, struct mesh *mesh, ufbx_node *node, ufbx_mesh *node_mesh)
{
	mesh->num_sub_meshes = node_mesh->material_parts.count;
	mesh->sub_meshes = malloc(sizeof(struct sub_mesh) * mesh->num_sub_meshes);
	snprintf(mesh->name, 256, "%s", node_mesh->name.data);

	// float scale = current_mesh_info->import_scale;
	float scale = 1.0f;
	size_t mat_index = 0;

	struct mesh_info *current_mesh_info = get_mesh_info(res, node->name.data);
	// struct mesh_info *current_mesh_info = NULL;

	for (int k = 0; k < node_mesh->material_parts.count; k++) {
		ufbx_mesh_part *mesh_part = &node_mesh->material_parts.data[k];
		size_t num_vertices = 0;
		size_t num_triangles = mesh_part->num_triangles;
		size_t num_tri_indices = node_mesh->max_face_triangles * 3;
		ufbx_material *ufbx_mat = node->materials.data[mesh_part->index];
		u32 *tri_indices = malloc(sizeof(u32) * num_tri_indices);
		struct vertex *vertices = malloc(sizeof(struct vertex) * num_triangles * 3);

		if (current_mesh_info) {
			mesh->sub_meshes[k].mat = current_mesh_info->mats[k];
		} else {
			struct material *mat = res->default_mat;
			if (ufbx_mat) {
				mat = find_material(res, ufbx_mat->name.data);

				if (!mat) {
					mat = get_new_material(res, ren, ufbx_mat->name.data);

					if (ufbx_mat->textures.count > 0) {
						char tex_name[256];
						get_filename(tex_name,
							     ufbx_mat->textures.data[0].texture->filename.data);

						struct texture *tex = find_texture(res, tex_name);
						if (!tex) {
							tex = &res->white_tex;
						}

						mat->tex = tex;
					} else {
						mat->tex = &res->white_tex;
					}
				}
			}

			mesh->sub_meshes[k].mat = mat;
		}

		for (size_t face_ix = 0; face_ix < mesh_part->num_faces; face_ix++) {
			ufbx_face face = node_mesh->faces.data[mesh_part->face_indices.data[face_ix]];

			u32 num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, node_mesh, face);

			for (size_t l = 0; l < num_tris * 3; l++) {
				u32 index = tri_indices[l];

				struct vertex *v = &vertices[num_vertices++];
				ufbx_vec3 pos = ufbx_get_vertex_vec3(&node_mesh->vertex_position, index);
				ufbx_vec3 normal = ufbx_get_vertex_vec3(&node_mesh->vertex_normal, index);
				ufbx_vec2 uv = ufbx_get_vertex_vec2(&node_mesh->vertex_uv, index);
				// v->pos.x = pos.x * scale;
				// v->pos.y = pos.y * scale;
				// v->pos.z = pos.z * scale;

				v->pos.x = pos.x;
				v->pos.y = pos.y;
				v->pos.z = pos.z;
				vec4s vert = (vec4s){ v->pos.x, v->pos.y, v->pos.z, 1.0f };
				mat4x3s mat43 = glms_mat4x3_make((float *)&node->node_to_world.m00);
				vec3s vert2 = glms_mat4x3_mulv(mat43, vert);
				v->pos.x = vert2.x;
				v->pos.y = vert2.y;
				v->pos.z = vert2.z;

				v->normal.x = normal.x;
				v->normal.y = normal.y;
				v->normal.z = normal.z;
				v->uv.x = uv.x;
				v->uv.y = uv.y;
				mesh->max = glms_vec3_maxv(mesh->max, v->pos);
				mesh->min = glms_vec3_minv(mesh->min, v->pos);
			}
		}

		// assert(num_vertices == num_triangles * 3);

		ufbx_vertex_stream streams[1] = {
			{ vertices, num_vertices, sizeof(struct vertex) },
		};

		size_t num_indices = num_triangles * 3;
		u32 *indices = malloc(sizeof(u32) * num_indices);
		num_vertices = ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

		create_vao(&mesh->sub_meshes[k], vertices, indices, num_vertices * sizeof(struct vertex),
			   num_indices * sizeof(u32));

		if (tri_indices)
			free(tri_indices);
		if (indices)
			free(indices);
		if (vertices)
			free(vertices);
	}

	mesh->center = glms_vec3_scale(glms_vec3_add(mesh->min, mesh->max), 0.5f);
	mesh->extent = glms_vec3_scale(glms_vec3_sub(mesh->max, mesh->min), 0.5f);
	res->num_meshes++;
}

void reload_model(struct resources *res, struct renderer *ren, struct model_import *model)
{
	ufbx_load_opts opts = { 0 };
	ufbx_scene *scene = ufbx_load_file(model->file_info.path, &opts, NULL);
	struct mesh *temp_meshes = malloc(sizeof(struct mesh) * scene->meshes.count);

	struct mesh **current_meshes = model->meshes;

	model->meshes = malloc(sizeof(struct mesh *) * scene->meshes.count);

	size_t count = scene->meshes.count > model->num_meshes ? model->num_meshes : scene->meshes.count;

	for (int i = 0; i < count; i++) {
		model->meshes[i] = current_meshes[i];
	}

	for (int i = count; i < scene->meshes.count - model->num_meshes; i++) {
		model->meshes[i] = &res->meshes[res->num_meshes];
		res->num_meshes++;
	}

	model->num_meshes = scene->meshes.count;

	int mesh_count = 0;
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];

		if (!node->mesh) {
			continue;
		}

		struct mesh *mesh = model->meshes[mesh_count];
		ufbx_mesh *node_mesh = node->mesh;
		struct mesh_info *current_mesh_info = get_mesh_info(res, node->name.data);

		mesh->max = (vec3s){ 0.0f, 0.0f, 0.0f };
		mesh->min = (vec3s){ 0.0f, 0.0f, 0.0f };
		create_mesh(res, ren, mesh, node, node_mesh);
		mesh_count++;
	}

	struct mesh new_mesh = { 0 };
	free(current_meshes);
}

void load_model_scene(struct resources *res, struct renderer *ren, const char *path, struct mesh_info *mesh_infos,
		      size_t num_mesh_infos)
{
	ufbx_load_opts opts = { 0 };
	ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);

	if (!scene) {
		printf("failed to load scene\n");
	}

	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];

		if (!node->mesh)
			continue;

		create_mesh(res, ren, &res->meshes[res->num_meshes], node, node->mesh);

		// res->num_meshes++;
	}

	res->num_models++;
	ufbx_free_scene(scene);
}

struct texture *get_new_texture(struct resources *res)
{
	struct texture *t = &res->textures[res->num_textures];
	res->num_textures++;
	return t;
}

void load_font(struct resources *res)
{
	if (FT_Init_FreeType(&res->ft)) {
		printf("ERROR::Init freetype");
		return;
	}

	int error = 0;

	char font_file[512];
#if defined(__linux__)
	snprintf(font_file, 512, "%s", "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf");
#elif defined(_WIN32)
	snprintf(font_file, 512, "%s",
		 "C:/Users/jorda/AppData/Local/Microsoft/Windows/Fonts/JetBrainsMonoNerdFont-Regular.ttf");
#endif

	if ((error = FT_New_Face(res->ft, font_file, 0, &res->font_face))) {
		printf("ERROR::Load font face: %s\n", FT_Error_String(error));
		return;
	}

	FT_Set_Pixel_Sizes(res->font_face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int i = 0; i < 128; i++) {
		if (FT_Load_Char(res->font_face, i, FT_LOAD_RENDER)) {
			printf("ERROR::Load freetype char");
			return;
		}

		struct font_character *fc = &res->font_characters[i];
		struct texture *tex = get_new_texture(res);

		GLuint w = res->font_face->glyph->bitmap.width;
		GLuint h = res->font_face->glyph->bitmap.rows;

		tex->internal_format = GL_R8;
		tex->format = GL_RED;
		tex->data_type = GL_UNSIGNED_BYTE;
		tex->wrap_type = GL_CLAMP_TO_EDGE;
		tex->filter_type = GL_LINEAR;
		tex->mips = false;
		tex->width = w;
		tex->height = h;

		fc->glyph_size = (vec2s){ res->font_face->glyph->bitmap.width, res->font_face->glyph->bitmap.rows };
		fc->bearing = (vec2s){ res->font_face->glyph->bitmap_left, res->font_face->glyph->bitmap_top };
		fc->advance = res->font_face->glyph->advance.x;
		fc->tex = tex;

		create_texture(tex, res->font_face->glyph->bitmap.buffer);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	FT_Done_Face(res->font_face);
	FT_Done_FreeType(res->ft);
}

void load_resources(struct resources *res, struct renderer *ren, struct arena *arena)
{
	size_t model_count = 0;
	size_t tex_count = 0;
	// size_t num_mesh_infos = 0;
	char *startDir = "../../res/export/pete/";
	struct arena temp_arena = get_new_arena((size_t)1 << 30);
	struct file_info *model_files = alloc_struct(&temp_arena, typeof(*model_files), 2048);
	struct file_info *texture_files = alloc_struct(&temp_arena, typeof(*texture_files), 2048);
	// struct mesh_info *mesh_infos = alloc_struct(&temp_arena, typeof(*mesh_infos), 40000);

	get_resource_files(startDir, model_files, texture_files, &model_count, &tex_count);

	res->num_mesh_infos = 0;
	res->mesh_infos = alloc_struct(arena, typeof(*res->mesh_infos), 40000);
	res->meshes = alloc_struct(arena, typeof(*res->meshes), model_count);
	res->textures = alloc_struct(arena, typeof(*res->textures), tex_count + 1000);
	res->model_imports = alloc_struct(arena, typeof(*res->model_imports), model_count);

	stbi_set_flip_vertically_on_load(true);

	for (int i = 0; i < tex_count; i++) {
		struct texture *tex = get_new_texture(res);
		tex->file_info = texture_files[i];
		load_image(texture_files[i].path, tex);
	}

	struct texture *tex = &res->white_tex;

	unsigned char white_pixel[3] = { 255, 255, 255 };

	tex->data_type = GL_UNSIGNED_BYTE;
	tex->wrap_type = GL_REPEAT;
	tex->filter_type = GL_NEAREST;
	tex->width = 1;
	tex->height = 1;
	tex->format = GL_RGB;
	tex->internal_format = GL_SRGB8;
	create_texture(tex, white_pixel);

	res->default_mat = &res->all_mats[res->num_mats];
	res->num_mats++;
	snprintf(res->default_mat->name, 256, "%s", "default");
	res->default_mat->tex = &res->white_tex;
	res->default_mat->shader = ren->default_shader;
	res->default_mat->color = (vec4s){ 1.0f, 1.0f, 1.0f, 1.0f };
	res->default_mesh_info = &res->mesh_infos[res->num_mesh_infos];
	res->num_mesh_infos++;
	snprintf(res->default_mesh_info->name, 256, "%s", "default");
	res->default_mesh_info->num_mats = 1;
	res->default_mesh_info->mats[0] = res->default_mat;
	res->default_mesh_info->import_scale = 1.0f;

	// create_mesh_infos(res, ren, "../../res/export/dungeon/SourceFiles/MaterialList_PolygonDungeon.txt", mesh_infos,
	// 		  &num_mesh_infos, 1.0f);
	// create_mesh_infos(res, ren, "../../res/export/horror/SourceFiles/MaterialList_PolygonHorrorMansion.txt", mesh_infos,
	// 		  &num_mesh_infos, 0.01f);

	for (int i = 0; i < model_count; i++) {
		size_t current_num = res->num_meshes;
		struct model_import *model = &res->model_imports[res->num_models];
		load_model_scene(res, ren, model_files[i].path, res->mesh_infos, res->num_mesh_infos);
		size_t new_num = res->num_meshes;
		size_t num_added = new_num - current_num;

		model->file_info = model_files[i];

		if (num_added > 0) {
			model->meshes = malloc(sizeof(struct mesh *) * num_added);
		}
		model->num_meshes = num_added;

		for (int i = current_num; i < new_num; i++) {
			model->meshes[i] = &res->meshes[i];
		}

		for (int i = 0; i < num_added; i++) {
			model->meshes[i] = &res->meshes[current_num + i];
		}

		// just need to document this:
		// this was trashing memory, which should be obvious.
		// indexing into model->meshes starting at current_num makes no sense.
		// if current num is 2, and i've only allocated 1 mesh struct, then this is going to trash some memory, like it did.
		// was a nightmare to debug.
		// using an arena for the meshes allocation might have made this less of an issue to debug.
		// this is also just a dumb way to do this in general.
		// Thank you for attending my TED talk.
		//
		// for (int i = current_num; i < new_num; i++) {
		// 	model->meshes[i] = &res->meshes[i];
		// }

		// res->num_models++;
	}

	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glCreateBuffers(1, &ebo);

	glNamedBufferStorage(vbo, sizeof(quad_verts), quad_verts, 0);
	glNamedBufferStorage(ebo, sizeof(quad_indices), quad_indices, 0);

	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 4);
	glVertexArrayElementBuffer(vao, ebo);

	glEnableVertexArrayAttrib(vao, 0);
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);

	ren->quad_vao = vao;
	ren->quad_vbo = vbo;
	ren->quad_ebo = ebo;

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glCreateBuffers(1, &ebo);

	glNamedBufferStorage(vbo, sizeof(text_quad_verts), text_quad_verts, 0);
	glNamedBufferStorage(ebo, sizeof(quad_indices), quad_indices, 0);

	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 4);
	glVertexArrayElementBuffer(vao, ebo);

	glEnableVertexArrayAttrib(vao, 0);
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);

	ren->text_quad_vao = vao;
	ren->text_quad_vbo = vbo;
	ren->text_quad_ebo = ebo;

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, sizeof(skybox_verts), skybox_verts, 0);
	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 3);
	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);

	ren->skybox_vao = vao;
	ren->skybox_vbo = vbo;

	ren->skybox_tex = get_new_texture(res);
	ren->skybox_night_tex = get_new_texture(res);

	const char *paths[] = {
		"../../res/export/skybox/px.png", "../../res/export/skybox/nx.png", "../../res/export/skybox/py.png",
		"../../res/export/skybox/ny.png", "../../res/export/skybox/pz.png", "../../res/export/skybox/nz.png",
	};

	const char *night_paths[] = {
		"../../res/export/skybox/night1/px.png", "../../res/export/skybox/night1/nx.png",
		"../../res/export/skybox/night1/py.png", "../../res/export/skybox/night1/ny.png",
		"../../res/export/skybox/night1/pz.png", "../../res/export/skybox/night1/nz.png",
	};

	stbi_set_flip_vertically_on_load(false);

	create_skybox_texture(ren->skybox_tex, paths);
	create_skybox_texture(ren->skybox_night_tex, night_paths);
	ren->current_skybox = ren->skybox_tex;

	load_font(res);
	load_shaders(ren);
	arena_free(&temp_arena);
}
