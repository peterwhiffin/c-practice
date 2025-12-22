#include "types.h"
#include "glad.c"
#include "ufbx.c"
#include "transform.c"
#include "parse.c"
#include "load.c"

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
		vec3s new_pos = glms_vec3_add(scene->scene_cam->transform->pos,
					      glms_vec3_scale(move_dir, scene->move_speed * scene->dt));
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

void draw_scene(struct renderer *ren, struct resources *res, struct scene *scene, struct window *win)
{
	update_cameras(scene);
	vec3s lightDir = { 0.1f, 0.3f, 0.0f };
	lightDir = vec3_normalize(lightDir);

	struct mesh_renderer *mr = scene->model->renderer;
	mr->mesh = &res->meshes[scene->current_model];

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearNamedFramebufferfv(0, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(0, GL_DEPTH, 0, &ren->clear_depth);

	for (int i = 0; i < scene->num_renderers; i++) {
		mr = &scene->renderers[i];
		glUseProgram(ren->shader);
		glUniform3fv(11, 1, &lightDir.x);
		glUniformMatrix4fv(9, 1, GL_FALSE, &scene->scene_cam->camera->viewProj.m00);
		glUniformMatrix4fv(13, 1, GL_FALSE, &scene->model->transform->world_transform.m00);
		glUniform1f(12, ren->light_active);
		glUniform1f(23, scene->time);

		for (int i = 0; i < mr->mesh->num_sub_meshes; i++) {
			struct sub_mesh *sm = &mr->mesh->sub_meshes[i];

			glBindTextureUnit(0, sm->mat->tex->tex_id);
			glBindVertexArray(sm->vao);
			glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
		}
	}
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
	ren->clear_color[0] = 0.0f;
	ren->clear_color[1] = 0.0f;
	ren->clear_color[2] = 0.0f;
	ren->clear_color[3] = 1.0f;
	ren->clear_depth = 1.0f;
	ren->light_active = 0.0f;
	glEnable(GL_DEPTH_TEST);
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
