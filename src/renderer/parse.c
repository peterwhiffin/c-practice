#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <unistd.h>
#include <wchar.h>
#include "../types.h"
#include "cglm/types-struct.h"
#include "file.h"
#include "load.h"
#include "../game/transform.c"
#include "../arena.h"

#define SEP ": "
#define INDENT "    "
#define OPEN "{\n"
#define CLOSE "}\n"

#define ENTITY_TYPE "Entity"
#define ENTITY_ID "id"
#define ENTITY_NAME "name"

#define TRANSFORM_TYPE "Transform"
#define TRANSFORM_POS "pos"
#define TRANSFORM_ROT "rot"
#define TRANSFORM_SCALE "scale"

#define RIGIDBODY_TYPE "Rigidbody"
#define RIGIDBODY_MOTION_TYPE "motion"
#define RIGIDBODY_SHAPE "shape"
#define RIGIDBODY_EXTENTS "extents"

#define CAMERA_TYPE "Camera"
#define CAMERA_FOV "fov"
#define CAMERA_NEAR "near"
#define CAMERA_FAR "far"

#define MESH_RENDERER_TYPE "MeshRenderer"
#define MESH_RENDERER_MESH "mesh"
#define MESH_RENDERER_MATS "materials"

#define MATERIAL_TYPE "Material"
#define MATERIAL_NAME "name"
#define MATERIAL_COLOR "color"
#define MATERIAL_ALBEDO "albedo"

enum token_type { TYPE, FIELD, SEPARATOR, OPENER, CLOSER, VALUE };

enum block_type { ENTITY, TRANSFORM, CAMERA, MESH_RENDERER, RIGIDBODY };

struct token {
	enum token_type type;
	char data[128];
};

struct token_pair {
	struct token_pair *next;
	struct token *field;
	struct token *value;
};

struct token_block {
	enum block_type type;
	struct token_pair *pair_list;
	struct token_block *parent;
	struct token_block *children;
	struct token_block *next;
};

void skip_white_space(const char *str, size_t *index)
{
	char c = str[*index];

	while (c == ' ' && c != '\0') {
		*index = *index + 1;
		c = str[*index];
	}
}

struct token *scene_get_tokens(char *filename, size_t *num_tokens, struct arena *arena)
{
	size_t token_index = 0;
	size_t str_index = 0;
	size_t file_length;

	const char *str = read_file(filename);
	struct token *tokens = alloc_struct(arena, typeof(*tokens), 40000);
	struct token *current_token = &tokens[token_index];
	enum token_type next_type = TYPE;

	while (str[str_index] != '\0') {
		switch (str[str_index]) {
		case ':':
			current_token = &tokens[token_index];
			current_token->type = SEPARATOR;
			next_type = VALUE;
			snprintf(current_token->data, 128, "%c", str[str_index]);
			token_index++;
			str_index++;
			break;
		case '{':
			current_token = &tokens[token_index];
			current_token->type = OPENER;
			tokens[token_index - 1].type = TYPE;
			snprintf(current_token->data, 128, "%c", str[str_index]);
			token_index++;
			str_index++;
			next_type = FIELD;
			break;
		case '}':
			current_token = &tokens[token_index];
			current_token->type = CLOSER;
			snprintf(current_token->data, 128, "%c", str[str_index]);
			token_index++;
			str_index++;
			next_type = TYPE;
			break;
		case ' ':
			str_index++;
			break;
		case '\n':
			str_index++;
			next_type = FIELD;
			break;
		case '\r':
			str_index++;
			break;
		default:
			current_token = &tokens[token_index];
			current_token->type = next_type;

			// skip_white_space(str, &str_index);
			size_t pos = 0;
			char c = str[str_index];
			while (c != ':' && c != '}' && c != '{' && c != '\n' && c != '\0' && c != '\r') {
				current_token->data[pos] = c;
				pos++;
				str_index++;
				c = str[str_index];
			}

			if (current_token->data[pos - 1] == ' ') {
				pos = pos - 1;
			}

			current_token->data[pos] = '\0';

			if (next_type == VALUE)
				next_type = FIELD;

			token_index++;

			break;
		}
	}

	free((void *)str);
	*num_tokens = token_index;
	return tokens;
}

void write_indent(FILE *f, int level)
{
	for (int i = 0; i < level; i++) {
		fprintf(f, "%s", INDENT);
	}
}

//TODO: must be a better way
void scene_write_entity(struct scene *scene, struct physics *phys, FILE *f, struct entity *current_entity,
			int indent_level)
{
	while (current_entity) {
		fprintf(f, "%s %s", ENTITY_TYPE, OPEN);
		indent_level++;
		write_indent(f, indent_level);
		fprintf(f, "%s%s%u\n", ENTITY_ID, SEP, current_entity->id);
		write_indent(f, indent_level);
		fprintf(f, "%s%s%s\n", ENTITY_NAME, SEP, current_entity->name);
		write_indent(f, indent_level);
		fprintf(f, "%s %s", TRANSFORM_TYPE, OPEN);
		indent_level++;
		write_indent(f, indent_level);
		fprintf(f, "%s%s%f, %f, %f\n", TRANSFORM_POS, SEP, current_entity->transform->pos.x,
			current_entity->transform->pos.y, current_entity->transform->pos.z);
		write_indent(f, indent_level);
		fprintf(f, "%s%s%f, %f, %f, %f\n", TRANSFORM_ROT, SEP, current_entity->transform->rot.x,
			current_entity->transform->rot.y, current_entity->transform->rot.z,
			current_entity->transform->rot.w);
		write_indent(f, indent_level);
		fprintf(f, "%s%s%f, %f, %f\n", TRANSFORM_SCALE, SEP, current_entity->transform->scale.x,
			current_entity->transform->scale.y, current_entity->transform->scale.z);
		indent_level--;
		write_indent(f, indent_level);
		fprintf(f, "%s", CLOSE);

		if (current_entity->camera) {
			write_indent(f, indent_level);
			fprintf(f, "%s %s", CAMERA_TYPE, OPEN);
			indent_level++;
			write_indent(f, indent_level);
			fprintf(f, "%s%s%f\n", CAMERA_FOV, SEP, current_entity->camera->fov);
			write_indent(f, indent_level);
			fprintf(f, "%s%s%f\n", CAMERA_NEAR, SEP, current_entity->camera->near_plane);
			write_indent(f, indent_level);
			fprintf(f, "%s%s%f\n", CAMERA_FAR, SEP, current_entity->camera->far_plane);
			indent_level--;
			write_indent(f, indent_level);
			fprintf(f, "%s", CLOSE);
		}

		if (current_entity->renderer) {
			write_indent(f, indent_level);
			fprintf(f, "%s %s", MESH_RENDERER_TYPE, OPEN);
			indent_level++;
			write_indent(f, indent_level);
			fprintf(f, "%s%s%s\n", MESH_RENDERER_MESH, SEP, current_entity->renderer->mesh->name);
			write_indent(f, indent_level);
			fprintf(f, "%s%s", MESH_RENDERER_MATS, SEP);
			for (int k = 0; k < current_entity->renderer->mesh->num_sub_meshes; k++) {
				if (k == current_entity->renderer->mesh->num_sub_meshes - 1) {
					fprintf(f, "%s\n", current_entity->renderer->mesh->sub_meshes[k].mat->name);
				} else {
					fprintf(f, "%s, ", current_entity->renderer->mesh->sub_meshes[k].mat->name);
				}
			}
			indent_level--;
			write_indent(f, indent_level);
			fprintf(f, "%s", CLOSE);
		}

		if (current_entity->body) {
			struct BodySettings settings = phys->physics_get_body_settings(phys, current_entity->body);
			char motion_string[128];
			char shape_string[128];

			switch (settings.motion) {
			case STATIC:
				snprintf(motion_string, 128, "%s", "static");
				break;
			case KINEMATIC:
				snprintf(motion_string, 128, "%s", "kinematic");
				break;
			case DYNAMIC:
				snprintf(motion_string, 128, "%s", "dynamic");
				break;
			}

			switch (settings.shape) {
			case BOX:
				snprintf(shape_string, 128, "%s", "box");
				break;
			case SPHERE:
				snprintf(shape_string, 128, "%s", "sphere");
				break;
			case CYLINDER:
				snprintf(shape_string, 128, "%s", "cylinder");
				break;
			case CAPSULE:
				snprintf(shape_string, 128, "%s", "capsule");
				break;
			}

			write_indent(f, indent_level);
			fprintf(f, "%s %s\n", RIGIDBODY_TYPE, OPEN);
			indent_level++;
			write_indent(f, indent_level);
			fprintf(f, "%s%s%s\n", RIGIDBODY_MOTION_TYPE, SEP, motion_string);
			write_indent(f, indent_level);
			fprintf(f, "%s%s%s\n", RIGIDBODY_SHAPE, SEP, shape_string);
			write_indent(f, indent_level);
			fprintf(f, "%s%s%f, %f, %f\n", RIGIDBODY_EXTENTS, SEP, settings.extents.x, settings.extents.y,
				settings.extents.z);

			indent_level--;
			write_indent(f, indent_level);
			fprintf(f, "%s", CLOSE);
		}

		if (current_entity->children) {
			scene_write_entity(scene, phys, f, current_entity->children, indent_level + 1);
		}

		current_entity = current_entity->next;
		indent_level--;
	}

	fprintf(f, "%s\n", CLOSE);
}

void scene_write(struct scene *scene, struct physics *phys)
{
	FILE *f = fopen("test.scene", "w");

	for (int i = 0; i < scene->entities.count; i++) {
		struct entity *e = &scene->entities.data[i];
		if (!e->parent) {
			scene_write_entity(scene, phys, f, e, 0);
		}
	}

	fclose(f);
}

struct token_block *get_new_block(struct arena *arena)
{
	struct token_block *b = alloc_struct(arena, typeof(*b), 1);
	b->type = 0;
	b->pair_list = NULL;
	b->next = NULL;
	b->parent = NULL;
	b->children = NULL;
	return b;
}

struct token_pair *get_new_pair(struct arena *arena)
{
	struct token_pair *p = alloc_struct(arena, typeof(*p), 1);
	p->field = NULL;
	p->value = NULL;
	p->next = NULL;
	return p;
}

struct token_block *scene_parse_tokens(struct scene *scene, struct token *tokens, size_t num_tokens,
				       struct arena *arena)
{
	struct token *current_token;
	struct token_block *current_block = NULL;
	struct token_block *current_parent = NULL;

	for (int i = 0; i < num_tokens; i++) {
		current_token = &tokens[i];

		switch (current_token->type) {
		case TYPE:
			if (current_block) {
				if (current_parent) {
					struct token_block *new_block = get_new_block(arena);
					new_block->next = current_parent->children;
					new_block->parent = current_parent;
					current_parent->children = new_block;
					current_block = new_block;
				} else {
					struct token_block *new_block = get_new_block(arena);
					new_block->next = current_block;
					current_block = new_block;
				}
			} else {
				current_block = get_new_block(arena);
			}

			current_parent = current_block;

			if (!strcmp(current_token->data, ENTITY_TYPE)) {
				current_block->type = ENTITY;
			} else if (!strcmp(current_token->data, TRANSFORM_TYPE)) {
				current_block->type = TRANSFORM;
			} else if (!strcmp(current_token->data, CAMERA_TYPE)) {
				current_block->type = CAMERA;
			} else if (!strcmp(current_token->data, MESH_RENDERER_TYPE)) {
				current_block->type = MESH_RENDERER;
			} else if (!strcmp(current_token->data, RIGIDBODY_TYPE)) {
				current_block->type = RIGIDBODY;
			}

			break;
		case FIELD:
			if (current_block->pair_list) {
				struct token_pair *new_pair = get_new_pair(arena);
				new_pair->next = current_block->pair_list;
				current_block->pair_list = new_pair;
			} else {
				struct token_pair *new_pair = get_new_pair(arena);
				current_block->pair_list = new_pair;
			}

			current_block->pair_list->field = current_token;
			break;
		case VALUE:
			current_block->pair_list->value = current_token;
			break;
		case SEPARATOR:
			break;
		case OPENER:
			break;
		case CLOSER:
			if (current_parent) {
				current_block = current_parent;
				current_parent = current_parent->parent;
			}

			break;
		}
	}

	return current_block;
}

struct mesh *find_mesh(struct resources *res, const char *mesh_name)
{
	for (int i = 0; i < res->num_meshes; i++) {
		struct mesh *m = &res->meshes[i];
		if (!strcmp(mesh_name, m->name)) {
			return m;
		}
	}

	printf("couldn't find mesh: %s\n", mesh_name);
	return &res->meshes[0];
}

void create_scene(struct scene *scene, struct physics *phys, struct game *game, struct resources *res,
		  struct token_block *blocks, struct entity *current_entity)
{
	struct entity *parent_entity = current_entity;

	while (blocks) {
		struct token_pair *current_pair = blocks->pair_list;

		switch (blocks->type) {
		case ENTITY:
			current_entity = game->get_new_entity(&scene->entities, &scene->transforms, &scene->next_id);
			game->entity_set_parent(current_entity, parent_entity);

			while (current_pair) {
				if (!strcmp(current_pair->field->data, ENTITY_NAME)) {
					snprintf(current_entity->name, 128, "%s", current_pair->value->data);
				} else if (!strcmp(current_pair->field->data, ENTITY_ID)) {
					current_entity->id = atoi(current_pair->value->data);
				}

				current_pair = current_pair->next;
			}
			break;
		case TRANSFORM:
			while (current_pair) {
				if (!strcmp(current_pair->field->data, TRANSFORM_POS)) {
					char *token = strtok(current_pair->value->data, ",");
					u32 index = 0;
					vec3s pos = (vec3s){ 0.0f, 0.0f, 0.0f };
					while (token) {
						pos.raw[index] = strtof(token, NULL);
						index++;
						token = strtok(NULL, ",");
					}

					set_position(current_entity->transform, pos);
				} else if (!strcmp(current_pair->field->data, TRANSFORM_ROT)) {
					char *token = strtok(current_pair->value->data, ",");
					u32 index = 0;
					versors rot = (versors){ 0.0f, 0.0f, 0.0f, 1.0f };
					while (token) {
						rot.raw[index] = strtof(token, NULL);
						index++;
						token = strtok(NULL, ",");
					}
					set_rotation(current_entity->transform, rot);
				} else if (!strcmp(current_pair->field->data, TRANSFORM_SCALE)) {
					char *token = strtok(current_pair->value->data, ",");
					u32 index = 0;
					vec3s scale = (vec3s){ 1.0f, 1.0f, 1.0f };
					while (token) {
						scale.raw[index] = strtof(token, NULL);
						index++;
						token = strtok(NULL, ",");
					}
					set_scale(current_entity->transform, scale);
				}
				current_pair = current_pair->next;
			}
			break;
		case CAMERA:
			game->add_camera(&scene->cameras, current_entity);
			while (current_pair) {
				if (!strcmp(current_pair->field->data, CAMERA_FOV)) {
					current_entity->camera->fov = strtof(current_pair->value->data, NULL);
				} else if (!strcmp(current_pair->field->data, CAMERA_NEAR)) {
					current_entity->camera->near_plane = strtof(current_pair->value->data, NULL);
				} else if (!strcmp(current_pair->field->data, CAMERA_FAR)) {
					current_entity->camera->far_plane = strtof(current_pair->value->data, NULL);
				}
				current_pair = current_pair->next;
			}

			break;
		case MESH_RENDERER:
			game->add_renderer(&scene->mesh_renderers, current_entity);
			while (current_pair) {
				if (!strcmp(current_pair->field->data, MESH_RENDERER_MESH)) {
					current_entity->renderer->mesh = find_mesh(res, current_pair->value->data);
				} else if (!strcmp(current_pair->field->data, MESH_RENDERER_MATS)) {
				}
				current_pair = current_pair->next;
			}
			break;

		case RIGIDBODY: {
			printf("found rigidbody component\n");
			struct BodySettings settings;

			while (current_pair) {
				if (!strcmp(current_pair->field->data, RIGIDBODY_MOTION_TYPE)) {
					if (!strcmp(current_pair->value->data, "static")) {
						settings.motion = STATIC;
					} else if (!strcmp(current_pair->value->data, "kinematic")) {
						settings.motion = KINEMATIC;
					} else if (!strcmp(current_pair->value->data, "dynamic")) {
						settings.motion = DYNAMIC;
					}
				} else if (!strcmp(current_pair->field->data, RIGIDBODY_SHAPE)) {
					if (!strcmp(current_pair->value->data, "box")) {
						settings.shape = BOX;
					} else if (!strcmp(current_pair->value->data, "sphere")) {
						settings.shape = SPHERE;
					} else if (!strcmp(current_pair->value->data, "cylinder")) {
						settings.shape = CYLINDER;
					} else if (!strcmp(current_pair->value->data, "capsule")) {
						settings.shape = CAPSULE;
					}
				} else if (!strcmp(current_pair->field->data, RIGIDBODY_EXTENTS)) {
					char *token = strtok(current_pair->value->data, ",");
					u32 index = 0;
					vec3s extents = (vec3s){ 0.5f, 0.5f, 0.5f };
					while (token) {
						extents.raw[index] = strtof(token, NULL);
						index++;
						token = strtok(NULL, ",");
					}
					settings.extents = extents;
				}

				current_pair = current_pair->next;
			}

			phys->add_rigidbody(phys, scene, current_entity, &settings);
			break;
		}
		}

		if (blocks->children) {
			create_scene(scene, phys, game, res, blocks->children, current_entity);
		}

		blocks = blocks->next;
		current_entity = parent_entity;
	}
}

void init_entities(struct scene *scene, struct physics *phys)
{
	for (int i = 0; i < scene->entities.count; i++) {
		struct entity *e = &scene->entities.data[i];
		if (e->body) {
			phys->rigidbody_init(phys, e);
		}
	}
}

void scene_load(struct scene *scene, struct physics *phys, struct game *game, struct resources *res, char *filename)
{
	struct arena temp_arena = get_new_arena((size_t)1 << 30);
	size_t num_tokens;
	struct token *tokens = scene_get_tokens(filename, &num_tokens, &temp_arena);
	struct token_block *blocks = scene_parse_tokens(scene, tokens, num_tokens, &temp_arena);
	create_scene(scene, phys, game, res, blocks, NULL);
	init_entities(scene, phys);
	arena_free(&temp_arena);
}

struct token *mesh_info_get_tokens(char *filename, size_t *num_tokens)
{
	size_t token_index = 0;
	size_t str_index = 0;
	size_t file_length;

	const char *str = read_file(filename);
	struct token *tokens = malloc(sizeof(*tokens) * 40000);
	struct token *current_token = &tokens[token_index];

	while (str[str_index] != '\0') {
		switch (str[str_index]) {
		case ':':
			current_token = &tokens[token_index];
			snprintf(current_token->data, 128, "%c", str[str_index]);
			token_index++;
			str_index++;
			break;
		case '(':
			current_token = &tokens[token_index];
			snprintf(current_token->data, 128, "%c", str[str_index]);
			token_index++;
			str_index++;
			break;
		case ')':
			current_token = &tokens[token_index];
			snprintf(current_token->data, 128, "%c", str[str_index]);
			token_index++;
			str_index++;
			break;
		case '-':
			str_index++;
			break;
		case ' ':
			str_index++;
			break;
		case '\n':
			str_index++;
			break;
		case '\r':
			str_index++;
			break;
		default:
			current_token = &tokens[token_index];
			size_t pos = 0;
			char c = str[str_index];
			while (c != ':' && c != '(' && c != ')' && c != '\n' && c != '\0' && c != '\r') {
				current_token->data[pos] = c;
				pos++;
				str_index++;
				c = str[str_index];
			}

			current_token->data[pos] = '\0';
			token_index++;

			break;
		}
	}

	free((void *)str);
	*num_tokens = token_index;
	return tokens;
}

// void create_mesh_infos(struct resources *res, struct renderer *ren, char *filename, struct mesh_info *mesh_infos,
// 		       size_t *num_mesh_infos, float import_scale)
// {
// 	size_t num_tokens = 0;
// 	size_t count = 0;
// 	struct mesh_info *current_mesh_info;
// 	struct material *current_mat;
// 	struct token *tokens = mesh_info_get_tokens(filename, &num_tokens);
//
// 	while (count < num_tokens) {
// 		struct token *t = &tokens[count];
//
// 		while (strcmp(t->data, "Mesh Name") == 0) {
// 			count += 2;
// 			t = &tokens[count];
//
// 			current_mesh_info = &mesh_infos[*num_mesh_infos];
// 			current_mesh_info->num_mats = 0;
// 			current_mesh_info->import_scale = import_scale;
// 			snprintf(current_mesh_info->name, 128, "%s", t->data);
// 			*num_mesh_infos += 1;
//
// 			count++;
// 			t = &tokens[count];
//
// 			while (strcmp(t->data, "Slot") == 0) {
// 				count += 2;
// 				t = &tokens[count];
//
// 				current_mat = find_material(res, t->data);
// 				if (!current_mat) {
// 					get_new_material(res, ren, t->data);
// 				}
// 				current_mesh_info->mats[current_mesh_info->num_mats] = current_mat;
// 				current_mesh_info->num_mats++;
//
// 				count += 2;
// 				t = &tokens[count];
//
// 				if (current_mat->tex == NULL) {
// 					current_mat->tex = find_texture(res, t->data);
// 				}
//
// 				count += 2;
// 				t = &tokens[count];
// 			}
//
// 			if (current_mesh_info->num_mats == 0) {
// 				current_mesh_info->mats[0] = &res->all_mats[0];
// 				current_mesh_info->num_mats = 1;
// 			}
// 		}
//
// 		count = count + 3;
// 	}
//
// 	*num_mesh_infos -= 1;
// 	free(tokens);
// }
