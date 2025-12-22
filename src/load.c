#define STB_IMAGE_IMPLEMENTATION
#include "types.h"
#include "load.h"
#include "stdio.h"
#include <dirent.h>
#include "ufbx.h"
#include "parse.h"
#include "stb_image.h"

double quad_verts[8] = {
	// clang-format off
	-1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f
	// clang-format on
};

unsigned int quad_indices[6] = { 0, 1, 2, 2, 3, 1 };

const char *read_file(char *filename)
{
	long len;
	FILE *f = fopen(filename, "r");
	char *str;
	fseek(f, 0, SEEK_END);
	len = ftello(f);
	fseek(f, 0, SEEK_SET);
	str = malloc(len + 1);
	fread(str, sizeof(char), len, f);
	str[len] = '\0';
	fclose(f);

	return str;
}

void checkShader(GLuint shader)
{
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(shader, 1024, NULL, log);
		printf("Shader compilation error: %s\n", log);
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

	checkShader(vertShader);
	checkShader(fragShader);
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
	ren->shader = load_shader("default.vert", "default.frag");
}

void reload_shaders(struct renderer *ren)
{
	glDeleteProgram(ren->shader);
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
}

void create_texture(struct texture *tex, unsigned char *data)
{
	GLuint tex_id;
	glCreateTextures(GL_TEXTURE_2D, 1, &tex_id);
	glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, tex->wrap_type);
	glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, tex->wrap_type);
	glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, tex->filter_type);
	glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, tex->filter_type);
	glTextureStorage2D(tex_id, 1, tex->internal_format, tex->width, tex->height);
	glTextureSubImage2D(tex_id, 0, 0, 0, tex->width, tex->height, tex->format, tex->data_type, data);
	glGenerateTextureMipmap(tex_id);
	tex->tex_id = tex_id;
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

void get_extension(char *ext, char *path)
{
	char *dot = strrchr(path, '.');
	snprintf(ext, 256, "%s", dot + 1);
}

void get_filename(char *name, const char *path)
{
	char *slash = strrchr(path, '/');
	snprintf(name, 256, "%s", slash + 1);
}

void get_filename_no_ext(char *name, const char *path)
{
	char file_name[256];
	get_filename(file_name, path);

	char *dot = strrchr(file_name, '.');
	*dot = '\0';

	char *under = strrchr(file_name, '_');

	if (under && strcmp(under + 1, "TOM") == 0) {
		*under = '\0';
	}

	snprintf(name, 256, "%s", file_name);
}

void get_resource_files(char *path, struct file_info *model_files, struct file_info *texture_files, size_t *model_count,
			size_t *texture_count)
{
	char full_path[256];
	char extension[128];
	char file_name[128];
	char name[128];

	DIR *dir_stream;
	struct file_info *file_info;
	struct dirent *dir;

	dir_stream = opendir(path);

	while ((dir = readdir(dir_stream)) != NULL) {
		sprintf(full_path, "%s%s", path, dir->d_name);

		if (dir->d_type == DT_DIR) {
			if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
				sprintf(full_path, "%s/", full_path);
				get_resource_files(full_path, model_files, texture_files, model_count, texture_count);
			}

		} else if (dir->d_type == DT_REG) {
			sprintf(full_path, "%s", full_path);
			get_extension(extension, full_path);
			get_filename(file_name, full_path);
			get_filename_no_ext(name, full_path);

			if (strcmp(extension, "fbx") == 0) {
				file_info = &model_files[*model_count];
				*model_count = *model_count + 1;
			} else if (strcmp(extension, "png") == 0) {
				file_info = &texture_files[*texture_count];
				*texture_count = *texture_count + 1;
			} else {
				continue;
			}

			snprintf(file_info->full_path, 256, "%s", full_path);
			snprintf(file_info->extension, 128, "%s", extension);
			snprintf(file_info->file_name, 128, "%s", file_name);
			snprintf(file_info->name, 128, "%s", name);
		}
	}

	closedir(dir_stream);
}

struct mesh_info *get_mesh_info(struct resources *res, const char *name)
{
	struct mesh_info *mesh_info = NULL;

	for (int i = 0; i < res->num_mesh_infos; i++) {
		mesh_info = &res->mesh_infos[i];
		if (strcmp(name, mesh_info->name) == 0) {
			return &res->mesh_infos[i];
		}
	}

	return mesh_info;
}

void load_model(struct resources *res, const char *path, struct mesh *mesh)
{
	ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);

	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];

		if (!node->mesh)
			continue;

		ufbx_mesh *node_mesh = node->mesh;
		mesh->num_sub_meshes = node_mesh->material_parts.count;
		mesh->sub_meshes = malloc(sizeof(struct sub_mesh) * mesh->num_sub_meshes);
		snprintf(mesh->name, 256, "%s", node->name.data);

		struct mesh_info *current_mesh_info = get_mesh_info(res, node->name.data);
		size_t mat_index = 0;

		for (int k = 0; k < node_mesh->material_parts.count; k++) {
			ufbx_mesh_part *mesh_part = &node_mesh->material_parts.data[k];
			size_t num_vertices = 0;
			size_t num_triangles = mesh_part->num_triangles;
			size_t num_tri_indices = node_mesh->max_face_triangles * 3;
			ufbx_material *mat = node->materials.data[mesh_part->index];
			u32 *tri_indices = malloc(sizeof(u32) * num_tri_indices);
			struct vertex *vertices = malloc(sizeof(struct vertex) * num_triangles * 3);
			mesh->sub_meshes[k].mat = current_mesh_info->mats[k];

			for (size_t face_ix = 0; face_ix < mesh_part->num_faces; face_ix++) {
				ufbx_face face = node_mesh->faces.data[mesh_part->face_indices.data[face_ix]];
				u32 num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, node_mesh, face);

				for (size_t l = 0; l < num_tris * 3; l++) {
					u32 index = tri_indices[l];
					struct vertex *v = &vertices[num_vertices++];
					ufbx_vec3 pos = ufbx_get_vertex_vec3(&node_mesh->vertex_position, index);
					ufbx_vec3 normal = ufbx_get_vertex_vec3(&node_mesh->vertex_normal, index);
					ufbx_vec2 uv = ufbx_get_vertex_vec2(&node_mesh->vertex_uv, index);
					v->pos.x = pos.x;
					v->pos.y = pos.y;
					v->pos.z = pos.z;
					v->normal.x = normal.x;
					v->normal.y = normal.y;
					v->normal.z = normal.z;
					v->uv.x = uv.x;
					v->uv.y = uv.y;
				}
			}

			assert(num_vertices == num_triangles * 3);

			ufbx_vertex_stream streams[1] = {
				{ vertices, num_vertices, sizeof(struct vertex) },
			};

			size_t num_indices = num_triangles * 3;
			u32 *indices = malloc(sizeof(u32) * num_indices);
			num_vertices = ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

			create_vao(&mesh->sub_meshes[k], vertices, indices, num_vertices * sizeof(struct vertex),
				   num_indices * sizeof(u32));

			free(tri_indices);
			free(indices);
			free(vertices);
		}
	}

	ufbx_free_scene(scene);
}

void load_resources(struct resources *res, struct renderer *ren)
{
	stbi_set_flip_vertically_on_load(true);
	char *startDir = "../../res/horror/";
	struct file_info *model_files = malloc(sizeof(*model_files) * 2048);
	struct file_info *texture_files = malloc(sizeof(*texture_files) * 2048);
	size_t model_count = 0;
	size_t tex_count = 0;
	size_t num_mesh_infos = 0;

	get_resource_files(startDir, model_files, texture_files, &model_count, &tex_count);

	res->mesh_infos = malloc(sizeof(struct mesh_info) * 40000);
	res->meshes = malloc(sizeof(struct mesh) * model_count);
	res->textures = malloc(sizeof(struct texture) * tex_count);

	for (int i = 0; i < tex_count; i++) {
		struct texture *tex = &res->textures[res->num_textures];
		snprintf(tex->path, 128, "%s", texture_files[i].full_path);
		snprintf(tex->ext, 128, "%s", texture_files[i].extension);
		snprintf(tex->filename, 128, "%s", texture_files[i].file_name);
		snprintf(tex->name, 128, "%s", texture_files[i].name);
		load_image(texture_files[i].full_path, tex);
		// printf("loaded tex: %s\n", res->textures[res->num_textures].name);
		res->num_textures++;
	}

	// create_mesh_infos(res, ren, "../../res/dungeon/SourceFiles/MaterialList_PolygonDungeon.txt");
	create_mesh_infos(res, ren, "../../res/horror/SourceFiles/MaterialList_PolygonHorrorMansion.txt");

	for (int i = 0; i < model_count; i++) {
		load_model(res, model_files[i].full_path, &res->meshes[res->num_models]);
		res->num_models++;
	}

	free(model_files);
	free(texture_files);
	free(res->mesh_infos);
}
