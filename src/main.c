#define UFBX_REAL_IS_FLOAT
#include <dirent.h>

#include "stb_image.h"
#include "types.h"
#include "ufbx.c"
#include "glad.c"
#include "parse.c"

double quad_verts[8] = {
	// clang-format off
	-1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f
	// clang-format on
};

unsigned int quad_indices[6] = { 0, 1, 2, 2, 3, 1 };

void window_init(struct window *win, struct input *input)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	win->sdl_win = SDL_CreateWindow("practice", 800, 600, SDL_WINDOW_OPENGL);
	win->ctx = SDL_GL_CreateContext(win->sdl_win);

	SDL_GL_MakeCurrent(win->sdl_win, win->ctx);
	SDL_GL_SetSwapInterval(1);
	input->keyStates = SDL_GetKeyboardState(NULL);
}

void check_input(struct input *input)
{
	input->movement.x = 0;
	input->movement.y = 0;
	SDL_MouseButtonFlags mouseButtonMask =
		SDL_GetGlobalMouseState(&input->cursorPosition.x, &input->cursorPosition.y);
	input->mouse0 = mouseButtonMask & SDL_BUTTON_LMASK;
	input->mouse1 = mouseButtonMask & SDL_BUTTON_RMASK;
	input->del = input->keyStates[SDL_SCANCODE_DELETE];
	input->space = input->keyStates[SDL_SCANCODE_SPACE];
	input->movement.x += input->keyStates[SDL_SCANCODE_A] ? 1 : 0;
	input->movement.x += input->keyStates[SDL_SCANCODE_D] ? -1 : 0;
	input->movement.y += input->keyStates[SDL_SCANCODE_S] ? -1 : 0;
	input->movement.y += input->keyStates[SDL_SCANCODE_W] ? 1 : 0;
	input->lookX = input->cursorPosition.x - input->oldX;
	input->lookY = input->oldY - input->cursorPosition.y;
	input->oldX = input->cursorPosition.x;
	input->oldY = input->cursorPosition.y;
}

void poll_events(struct window *win)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			win->should_close = true;
			break;
		}
	}
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

	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(struct vertex, uv));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);

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
					ufbx_vec2 uv = ufbx_get_vertex_vec2(&node_mesh->vertex_uv, index);
					v->pos.x v->pos.x = pos.x;
					v->pos.y = pos.y;
					v->pos.z = pos.z;
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

void load_resources(struct resources *res)
{
	char *startDir = "../../res/";
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
		printf("loaded tex: %s\n", res->textures[res->num_textures].name);
		res->num_textures++;
	}

	get_mesh_infos(res, "../../res/dungeon/SourceFiles/MaterialList_PolygonDungeon.txt");
	get_mesh_infos(res, "../../res/horror/SourceFiles/MaterialList_PolygonHorrorMansion.txt");

	for (int i = 0; i < model_count; i++) {
		load_model(res, model_files[i].full_path, &res->meshes[res->num_models]);
		res->num_models++;
	}

	free(model_files);
	free(texture_files);
	free(res->mesh_infos);
}

void update(struct scene *scene, struct input *input, struct resources *res)
{
	float time = SDL_GetTicks() / 1000.0f;
	scene->dt = time - scene->time;
	scene->time = time;

	if (input->mouse1) {
		scene->rot.x += input->lookY * 0.01f;
		scene->rot.y += input->lookX * 0.01f;
	}

	if (input->movement.x != 0) {
		if (scene->can_switch) {
			scene->can_switch = false;
			scene->model_timer = 0.0f;
			scene->current_model += input->movement.x;

			if (scene->current_model == res->num_models) {
				scene->current_model = 0;
			} else if (scene->current_model < 0) {
				scene->current_model = res->num_models - 1;
			}

			printf("current model: %s\n", res->meshes[scene->current_model].name);
		}

		scene->model_timer += scene->dt;
		if (scene->model_timer >= 0.03f) {
			scene->can_switch = true;
			scene->model_timer = 0.0f;
		}

	} else {
		scene->can_switch = true;
	}
}

void draw_scene(struct render_state *render_state, struct resources *res, struct scene *scene)
{
	float pos[3] = { 0.00f, 0.0f, 0.3f };
	float col[4] = { 0.0f, 0.3f, 0.9f };
	float scale = 0.006f;
	struct mesh *ren = &res->meshes[scene->current_model];

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearNamedFramebufferfv(0, GL_COLOR, 0, &render_state->clear_color[0]);
	glClearNamedFramebufferfv(0, GL_DEPTH, 0, &render_state->clear_depth);
	glUseProgram(res->shader);
	glUniform4fv(10, 1, &col[0]);
	glUniform3fv(5, 1, &pos[0]);
	glUniform3fv(7, 1, &scene->rot.x);
	glUniform1f(6, scale);

	for (int i = 0; i < ren->num_sub_meshes; i++) {
		struct sub_mesh *sm = &ren->sub_meshes[i];

		glBindTextureUnit(0, sm->mat->tex->tex_id);
		glBindVertexArray(sm->vao);
		glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
	}
}

int main()
{
	struct window *win = malloc(sizeof(*win));
	struct input *input = malloc(sizeof(*input));
	struct resources *res = malloc(sizeof(*res));
	struct render_state *render_state = malloc(sizeof(*render_state));
	struct scene *scene = malloc(sizeof(*scene));

	memset(input, 0, sizeof(*input));
	memset(win, 0, sizeof(*win));
	window_init(win, input);
	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

	res->shader = load_shader("default.vert", "default.frag");
	render_state->clear_color[0] = 0.0f;
	render_state->clear_color[1] = 0.0f;
	render_state->clear_color[2] = 0.0f;
	render_state->clear_color[3] = 1.0f;
	render_state->clear_depth = 1.0f;
	scene->can_switch = true;

	stbi_set_flip_vertically_on_load(true);
	load_resources(res);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!win->should_close) {
		poll_events(win);
		check_input(input);
		update(scene, input, res);
		draw_scene(render_state, res, scene);
		SDL_GL_SwapWindow(win->sdl_win);
	}

	return 0;
}
