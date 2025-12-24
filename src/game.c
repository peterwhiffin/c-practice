#include "cglm/struct/cam.h"
#include "cglm/struct/mat4.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "cglm/types.h"
#include "transform.h"
#include "types.h"
#include "glad.c"
#include "ufbx.c"
#include "transform.c"
#include "parse.c"
#include "load.c"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static struct transform *add_transform(struct scene *scene, struct entity *entity)
{
	struct transform *t = &scene->transforms[scene->num_transforms];
	set_position(t, (vec3s){ 0.0f, 0.0f, 0.0f });
	set_rotation(t, (versors){ 0.0f, 0.0f, 0.0f, 1.0f });
	set_scale(t, (vec3s){ 1.0f, 1.0f, 1.0f });
	entity->transform = t;
	scene->num_transforms++;
	return t;
}

struct camera *add_camera(struct scene *scene, struct entity *entity)
{
	struct camera *c = &scene->cameras[scene->num_cameras];
	c->near = 0.1f;
	c->far = 1000.0f;
	c->fov = glm_rad(78.0f);
	entity->camera = c;
	c->entity = entity;
	scene->num_cameras++;
	return c;
}

struct mesh_renderer *add_renderer(struct scene *scene, struct entity *entity)
{
	struct mesh_renderer *r = &scene->renderers[scene->num_renderers];
	entity->renderer = r;
	scene->num_renderers++;
	return r;
}

struct entity *get_new_entity(struct scene *scene)
{
	struct entity *e = &scene->entities[scene->num_entities];
	e->transform = add_transform(scene, e);
	scene->num_entities++;
	return e;
}

void update(struct scene *scene, struct input *input, struct resources *res, struct renderer *ren, struct window *win)
{
	if (input->arrows.state == STARTED) {
		scene->current_model += input->arrows.value.x;

		if (scene->current_model == res->num_models) {
			scene->current_model = 0;
		} else if (scene->current_model < 0) {
			scene->current_model = res->num_models - 1;
		}

		printf("current model: %s\n", res->meshes[scene->current_model].name);
	}

	if (input->space.state == STARTED) {
		ren->light_active = !ren->light_active;
	}

	if (input->mouse1.state == STARTED) {
		input->lock_mouse(win->sdl_win, true);
	} else if (input->mouse1.state == CANCELED) {
		input->lock_mouse(win->sdl_win, false);
	}

	if (input->mouse1.state != CANCELED) {
		versors current_rot = scene->scene_cam->transform->rot;

		versors new_rot = glms_euler_xyz_quat(
			(vec3s){ -input->lookY * scene->look_sens, -input->lookX * scene->look_sens, 0.0f });

		set_rotation(scene->scene_cam->transform, glms_quat_mul(current_rot, new_rot));

		vec3s forward = get_forward(scene->scene_cam->transform);
		vec3s right = get_right(scene->scene_cam->transform);
		vec3s move_dir = glms_vec3_add(glms_vec3_scale(forward, input->movement.value.y),
					       glms_vec3_scale(right, input->movement.value.x));
		vec3s new_pos = glms_vec3_add(
			scene->scene_cam->transform->pos,
			glms_vec3_scale(move_dir, scene->move_speed * scene->dt +
							  (scene->move_mod * input->left_shift.value.raw[0])));
		set_position(scene->scene_cam->transform, new_pos);
	}

	if (input->del.state == STARTED) {
		scene->draw_mode = scene->draw_mode == GL_FILL ? GL_LINE : GL_FILL;
		glPolygonMode(GL_FRONT_AND_BACK, scene->draw_mode);
	}
}

void update_cameras(struct scene *scene)
{
	for (int i = 0; i < scene->num_cameras; i++) {
		struct camera *cam = &scene->cameras[i];
		cam->proj = glms_perspective(cam->fov, 800.0f / 600.0f, cam->near, cam->far);
		cam->view = glms_lookat(cam->entity->transform->pos,
					glms_vec3_add(cam->entity->transform->pos, get_forward(cam->entity->transform)),
					get_up(cam->entity->transform));
		cam->viewProj = glms_mat4_mul(cam->proj, cam->view);
	}
}

void draw_text(struct renderer *ren, struct resources *res, const char *text, size_t length, float x, float y,
	       float scale)
{
	glUseProgram(ren->font_shader);
	ren->text_proj = glms_ortho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 1000.0f);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniformMatrix4fv(4, 1, GL_FALSE, &ren->text_proj.m00);
	vec3s color = { 1.0f, 1.0f, 1.0f };
	glUniform3fv(11, 1, &color.x);
	glBindVertexArray(res->font_vao);

	for (int i = 0; i < length; i++) {
		struct font_character *c = &res->font_characters[text[i]];

		float xpos = x + c->bearing.x * scale;
		float ypos = y - (c->glyph_size.y - c->bearing.y) * scale;
		float w = c->glyph_size.x * scale;
		float h = c->glyph_size.y * scale;

		float vertices[6][4] = {
			// clang-format off
			{xpos,     ypos + h, 0.0f, 0.0f},
			{xpos,     ypos,     0.0f, 1.0f},
			{xpos + w, ypos,     1.0f, 1.0f},

			{xpos, 	   ypos + h, 0.0f, 0.0f},
			{xpos + w, ypos,     1.0f, 1.0f},
			{xpos + w, ypos + h, 1.0f, 0.0f},
			// clang-format on
		};

		glBindTextureUnit(0, c->tex->tex_id);
		glBindBuffer(GL_ARRAY_BUFFER, res->font_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (c->advance >> 6) * scale;
	}
}

void draw_scene(struct renderer *ren, struct resources *res, struct scene *scene, struct window *win)
{
	update_cameras(scene);
	vec3s lightDir = { 0.1f, 0.3f, 0.0f };
	lightDir = vec3_normalize(lightDir);

	vec4s white = (vec4s){ 1.0f, 1.0f, 1.0f, 1.0f };
	vec4s orange = (vec4s){ 1.0f, 0.60f, 0.0f, 1.0f };
	struct mesh_renderer *mr = scene->model->renderer;
	mr->mesh = &res->meshes[scene->current_model];

	const GLint clear_stencil = 0xFF;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearNamedFramebufferfv(0, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(0, GL_DEPTH, 0, &ren->clear_depth);
	glClearNamedFramebufferiv(0, GL_STENCIL, 0, &clear_stencil);

	for (int i = 0; i < scene->num_renderers; i++) {
		mr = &scene->renderers[i];
		vec3s scale = scene->model->transform->scale;
		vec3s up_scale = glms_vec3_scale(scale, 1.01f);
		glUseProgram(ren->shader);
		glUniform3fv(11, 1, &lightDir.x);
		glUniformMatrix4fv(9, 1, GL_FALSE, &scene->scene_cam->camera->viewProj.m00);
		glUniformMatrix4fv(13, 1, GL_FALSE, &scene->model->transform->world_transform.m00);
		glUniform1f(12, ren->light_active);
		glUniform1f(23, scene->time);
		glUniform4fv(10, 1, &white.x);

		glStencilMask(0x00);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		for (int i = 0; i < mr->mesh->num_sub_meshes; i++) {
			struct sub_mesh *sm = &mr->mesh->sub_meshes[i];

			glBindTextureUnit(0, sm->mat->tex->tex_id);
			glBindVertexArray(sm->vao);
			glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
		}

		set_scale(scene->model->transform, up_scale);
		glUniformMatrix4fv(13, 1, GL_FALSE, &scene->model->transform->world_transform.m00);
		glUniform4fv(10, 1, &orange.x);

		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glDisable(GL_DEPTH_TEST);

		glUniform1f(12, 0.0f);
		glUniform1f(24, 0.008f);
		for (int i = 0; i < mr->mesh->num_sub_meshes; i++) {
			struct sub_mesh *sm = &mr->mesh->sub_meshes[i];

			glBindTextureUnit(0, res->white_tex.tex_id);
			glBindVertexArray(sm->vao);
			glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
		}

		glUniform1f(24, 0.0f);
		set_scale(scene->model->transform, scale);
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glEnable(GL_DEPTH_TEST);
	}

	const char *test_text = "It's text.\0";

	draw_text(ren, res, test_text, strlen(test_text), 25, 25, 1.0f);
}

void init_scene(struct scene *scene, struct resources *res)
{
	scene->current_model = 0;
	scene->look_sens = 0.007f;
	scene->move_speed = 10.0f;
	scene->num_entities = 0;
	scene->num_transforms = 0;
	scene->num_cameras = 0;
	scene->num_renderers = 0;

	scene->scene_cam = get_new_entity(scene);
	add_camera(scene, scene->scene_cam);
	set_position(scene->scene_cam->transform, (vec3s){ 0.0f, 0.0f, -10.0f });
	scene->scene_cam->camera->fov = glm_rad(79.0f);
	scene->scene_cam->camera->near = 0.1f;
	scene->scene_cam->camera->far = 1000.0f;

	scene->model = get_new_entity(scene);
	add_renderer(scene, scene->model);
	set_scale(scene->model->transform, (vec3s){ 0.01f, 0.01f, 0.01f });
	scene->model->renderer->mesh = &res->meshes[scene->current_model];
}

void init_renderer(struct renderer *ren)
{
	load_shaders(ren);
	ren->text_proj = glms_ortho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 1000.0f);
	glUseProgram(ren->font_shader);
	glUniformMatrix4fv(4, 1, GL_FALSE, &ren->text_proj.m00);
	glUseProgram(ren->shader);

	ren->clear_color[0] = 0.0f;
	ren->clear_color[1] = 0.0f;
	ren->clear_color[2] = 0.0f;
	ren->clear_color[3] = 1.0f;
	ren->clear_depth = 1.0f;
	ren->light_active = 0.0f;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void load_game_functions(struct game *game, GLADloadproc load)
{
	game->init_renderer = init_renderer;
	game->init_scene = init_scene;
	game->reload_shaders = reload_shaders;
	game->load_resources = load_resources;
	game->update = update;
	game->draw_scene = draw_scene;
	gladLoadGLLoader(load);
}
