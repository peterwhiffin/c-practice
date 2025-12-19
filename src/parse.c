#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "types.h"

struct token {
	char data[128];
};

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

struct token *get_tokens(char *filename, size_t *num_tokens)
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

struct texture *find_texture(struct resources *win, char *name)
{
	for (int i = 0; i < win->num_textures; i++) {
		struct texture *tex = &win->textures[i];
		if (strcmp(name, tex->name) == 0) {
			return tex;
		}
	}

	return &win->textures[0];
}

struct material *find_material(struct resources *win, char *mat_name)
{
	for (int i = 0; i < win->num_mats; i++) {
		if (strcmp(win->all_mats[i].name, mat_name) == 0) {
			return &win->all_mats[i];
		}
	}

	snprintf(win->all_mats[win->num_mats].name, 128, "%s", mat_name);
	win->all_mats[win->num_mats].shader = win->shader;
	win->all_mats[win->num_mats].tex = NULL;
	win->num_mats++;
	return &win->all_mats[win->num_mats - 1];
}

void write_mesh_infos(struct resources *res)
{
	FILE *f = fopen("testmeshinfostoo.txt", "w");

	for (int i = 0; i < res->num_mesh_infos; i++) {
		struct mesh_info *mesh_info = &res->mesh_infos[i];

		fprintf(f, "mesh: %s\n", mesh_info->name);
		for (int k = 0; k < mesh_info->num_mats; k++) {
			struct material *mat = mesh_info->mats[k];
			fprintf(f, "	mat: %s\n", mat->name);
			fprintf(f, "		tex: %s\n", mat->tex->name);
		}
	}

	fclose(f);
}

void write_tokens(struct token *tokens, size_t num_tokens)
{
	FILE *f = fopen("tokentest.txt", "w");

	for (int i = 0; i < num_tokens; i++) {
		struct token *t = &tokens[i];
		fprintf(f, "token: %s\n", t->data);
	}

	fclose(f);
}

void create_mesh_infos(struct resources *res, char *filename)
{
	size_t num_mesh_infos = res->num_mesh_infos;
	size_t num_tokens = 0;
	size_t count = 0;
	struct mesh_info *current_mesh_info;
	struct material *current_mat;
	struct token *tokens = get_tokens(filename, &num_tokens);

	while (count < num_tokens) {
		struct token *t = &tokens[count];

		while (strcmp(t->data, "Mesh Name") == 0) {
			count += 2;
			t = &tokens[count];

			current_mesh_info = &res->mesh_infos[num_mesh_infos];
			current_mesh_info->num_mats = 0;
			snprintf(current_mesh_info->name, 128, "%s", t->data);
			num_mesh_infos++;

			count++;
			t = &tokens[count];

			while (strcmp(t->data, "Slot") == 0) {
				count += 2;
				t = &tokens[count];

				current_mat = find_material(res, t->data);
				current_mesh_info->mats[current_mesh_info->num_mats] = current_mat;
				current_mesh_info->num_mats++;

				count += 2;
				t = &tokens[count];

				if (current_mat->tex == NULL) {
					current_mat->tex = find_texture(res, t->data);
				}

				count += 2;
				t = &tokens[count];
			}

			if (current_mesh_info->num_mats == 0) {
				current_mesh_info->mats[0] = &res->all_mats[0];
				current_mesh_info->num_mats = 1;
			}
		}

		count = count + 3;
	}

	res->num_mesh_infos = num_mesh_infos - 1;
	free(tokens);
}
