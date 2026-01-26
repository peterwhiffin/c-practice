#include "cglm/struct/affine.h"
#include "cglm/struct/euler.h"
#include "cglm/struct/mat4.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "cglm/types.h"
#include "transform.h"
#include <math.h>
#include <string.h>
#define CGLM_FORCE_LEFT_HANDED

#define UFBX_REAL_IS_FLOAT

#include "../types.h"
#include "transform.c"
#include <stddef.h>
#include <stdio.h>

static struct transform *add_transform(struct scene *scene, struct entity *entity)
{
	struct transform *t = &scene->transforms[scene->num_transforms];
	t->entity = entity;
	entity->transform = t;
	set_position(t, (vec3s){ 0.0f, 0.0f, 0.0f });
	set_rotation(t, (versors){ 0.0f, 0.0f, 0.0f, 1.0f });
	set_scale(t, (vec3s){ 1.0f, 1.0f, 1.0f });
	scene->num_transforms++;
	return t;
}

struct camera *add_camera(struct scene *scene, struct entity *entity)
{
	struct camera *c = &scene->cameras[scene->num_cameras];
	c->near_plane = 0.1f;
	c->far_plane = 1000.0f;
	c->fov = glm_rad(78.0f);
	entity->camera = c;
	c->entity = entity;
	scene->num_cameras++;
	scene->scene_cam = entity;
	return c;
}

struct mesh_renderer *add_renderer(struct scene *scene, struct entity *entity)
{
	struct mesh_renderer *r = &scene->renderers[scene->num_renderers];
	entity->renderer = r;
	r->entity = entity;
	scene->num_renderers++;
	return r;
}

struct entity *get_new_entity(struct scene *scene)
{
	struct entity *e = &scene->entities[scene->num_entities];
	e->id = scene->next_id++;
	e->transform = add_transform(scene, e);
	e->parent = NULL;
	e->children = NULL;
	e->next = NULL;
	e->prev = NULL;
	snprintf(e->name, 128, "%s %u", "Entity", e->id);
	scene->num_entities++;
	return e;
}

void entity_unset_parent(struct entity *entity)
{
	if (!entity->parent)
		return;

	if (entity->parent->children == entity) {
		entity->parent->children = entity->next;
	} else {
		entity->prev->next = entity->next;
	}

	entity->next->prev = entity->prev;
	entity->next = NULL;
	entity->prev = NULL;
	entity->parent = NULL;

	struct transform *t = entity->transform;

	vec4s pos4;
	mat4s rot_mat;
	vec3s scale;

	glms_decompose(t->world_transform, &pos4, &rot_mat, &scale);

	vec3s pos = (vec3s){ pos4.x, pos4.y, pos4.z };
	versors rot = glms_mat4_quat(rot_mat);

	set_position(t, pos);
	set_rotation(t, rot);
	set_scale(t, scale);
}

void entity_set_parent(struct entity *child, struct entity *parent)
{
	entity_unset_parent(child);

	if (parent == NULL)
		return;

	child->parent = parent;

	if (!parent->children) {
		parent->children = child;
	} else {
		parent->children->prev = child;
		child->next = parent->children;
		parent->children = child;
	}

	mat4s inverse_parent = glms_mat4_inv(parent->transform->world_transform);
	mat4s world_to_local = glms_mat4_mul(inverse_parent, child->transform->world_transform);

	struct transform *t = child->transform;

	vec4s pos4;
	mat4s rot_mat;
	vec3s scale;

	glms_decompose(world_to_local, &pos4, &rot_mat, &scale);

	vec3s pos = (vec3s){ pos4.x, pos4.y, pos4.z };
	versors rot = glms_mat4_quat(rot_mat);

	set_position(t, pos);
	set_rotation(t, rot);
	set_scale(t, scale);
}

struct entity *entity_duplicate(struct scene *scene, struct entity *e)
{
	struct entity *new_entity = get_new_entity(scene);
	memcpy(new_entity->transform, e->transform, sizeof(struct transform));

	if (e->camera) {
		add_camera(scene, new_entity);
		memcpy(new_entity->camera, e->camera, sizeof(struct camera));
		new_entity->camera->entity = new_entity;
	}

	if (e->renderer) {
		add_renderer(scene, new_entity)->mesh = e->renderer->mesh;
	}

	return new_entity;
}

void update_cameras(struct scene *scene)
{
	for (int i = 0; i < scene->num_cameras; i++) {
		struct camera *cam = &scene->cameras[i];
		cam->proj = glms_perspective(cam->fov, 800.0f / 600.0f, cam->near_plane, cam->far_plane);
		cam->view = glms_lookat(cam->entity->transform->pos,
					glms_vec3_add(cam->entity->transform->pos, get_forward(cam->entity->transform)),
					get_up(cam->entity->transform));

		cam->viewProj = glms_mat4_mul(cam->proj, cam->view);
	}
}

void update(struct scene *scene, struct input *input, struct resources *res, struct renderer *ren, struct window *win)
{
	if (input->actions[SPACE].state == STARTED) {
		ren->light_active = !ren->light_active;
	}

	if (input->actions[M1].state == STARTED) {
		input->lock_mouse(win->sdl_win, true);
	} else if (input->actions[M1].state == CANCELED) {
		input->lock_mouse(win->sdl_win, false);
	}

	if (input->actions[M1].state != CANCELED) {
		versors current_rot = scene->scene_cam->transform->rot;

		scene->pitch -= input->actions[MOUSE_DELTA].composite.y * scene->look_sens;
		scene->yaw -= input->actions[MOUSE_DELTA].composite.x * scene->look_sens;

		versors target_rot = glms_euler_zyx_quat((vec3s){ scene->pitch, scene->yaw, 0.0f });
		set_rotation(scene->scene_cam->transform, target_rot);

		vec3s forward = get_forward(scene->scene_cam->transform);
		vec3s right = get_right(scene->scene_cam->transform);

		vec3s move_dir = glms_vec3_add(glms_vec3_scale(forward, input->actions[WASD].composite.y),
					       glms_vec3_scale(right, -input->actions[WASD].composite.x));

		vec3s new_pos = glms_vec3_add(scene->scene_cam->transform->pos,
					      glms_vec3_scale(move_dir, scene->move_speed * scene->dt));
		set_position(scene->scene_cam->transform, new_pos);
	}

	if (input->actions[DEL].state == STARTED) {
		scene->draw_mode = scene->draw_mode == GL_FILL ? GL_LINE : GL_FILL;
	}

	if (input->actions[P].state == STARTED) {
		ren->current_skybox = ren->current_skybox == ren->skybox_tex ? ren->skybox_night_tex : ren->skybox_tex;
	}

	update_cameras(scene);
}

void init_scene(struct scene *scene, struct resources *res)
{
	scene->current_model = 0;
	scene->next_id = 1;
	scene->look_sens = 0.007f;
	scene->move_speed = 10.0f;
	scene->num_entities = 0;
	scene->num_transforms = 0;
	scene->num_cameras = 0;
	scene->num_renderers = 0;

	// scene->scene_cam = get_new_entity(scene);
	// add_camera(scene, scene->scene_cam);
	// set_position(scene->scene_cam->transform, (vec3s){ 0.0f, 0.0f, -10.0f });
	// scene->scene_cam->camera->fov = glm_rad(79.0f);
	// scene->scene_cam->camera->near_plane = 0.1f;
	// scene->scene_cam->camera->far_plane = 1000.0f;

	// for (int i = 0; i < res->num_meshes; i++) {
	// 	struct entity *e = get_new_entity(scene);
	// 	add_renderer(scene, e);
	// 	e->renderer->mesh = &res->meshes[i];
	// }
}

PETE_API void load_game_functions(struct game *game)
{
	game->init_scene = init_scene;
	game->update = update;
	game->entity_duplicate = entity_duplicate;
	game->get_new_entity = get_new_entity;
	game->add_camera = add_camera;
	game->add_renderer = add_renderer;
	game->entity_unset_parent = entity_unset_parent;
	game->entity_set_parent = entity_set_parent;
}
