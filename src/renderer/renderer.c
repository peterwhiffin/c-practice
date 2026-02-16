#include "cglm/types.h"
#include <stddef.h>
#define UFBX_REAL_IS_FLOAT
#include "cglm/struct/mat4.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "glad.c"
#include "ufbx.c"
#include "file.c"
#include "../arena.c"
#include "parse.c"
#include "load.c"
#include <stdio.h>

void draw_text(struct renderer *ren, struct resources *res, const char *text, size_t length, float x, float y,
	       float scale)
{
	glUseProgram(ren->text_shader);
	ren->text_proj = glms_ortho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 1000.0f);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniformMatrix4fv(4, 1, GL_FALSE, &ren->text_proj.m00);
	vec3s color = { 1.0f, 1.0f, 1.0f };
	glUniform3fv(11, 1, &color.x);
	glBindVertexArray(ren->text_quad_vao);

	for (int i = 0; i < length; i++) {
		struct font_character *c = &res->font_characters[text[i]];
		float xpos = x + c->bearing.x * scale;
		float ypos = y - (c->glyph_size.y - c->bearing.y) * scale;
		float w = c->glyph_size.x * scale;
		float h = c->glyph_size.y * scale;

		mat4s T = glms_translate(GLMS_MAT4_IDENTITY, (vec3s){ xpos, ypos, 0.0f });
		mat4s S = glms_scale(GLMS_MAT4_IDENTITY, (vec3s){ w, h, 1.0 });
		mat4s model = glms_mat4_mul(T, S);

		glUniformMatrix4fv(5, 1, GL_FALSE, &model.m00);
		glBindTextureUnit(0, c->tex->id);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);
		x += (c->advance >> 6) * scale;
	}
}

void render_debug(struct renderer *renderer, struct camera *cam, struct DebugLine *lines, struct DebugTri *tris,
		  size_t num_lines, size_t num_tris)
{
	glDisable(GL_DEPTH_TEST);
	glUseProgram(renderer->debug_shader);
	glUniformMatrix4fv(2, 1, GL_FALSE, &cam->viewProj.m00);

	size_t num_line_verts = num_lines * 2;
	struct DebugVertex *line_verts = malloc(sizeof(struct DebugVertex) * num_line_verts);

	for (int i = 0; i < num_lines; i++) {
		struct DebugLine *line = &lines[i];
		struct DebugVertex *line_vert = &line_verts[i * 2];
		struct DebugVertex *line_vert2 = &line_verts[i * 2 + 1];

		line_vert->pos = line->start;
		line_vert->color = (vec4s){
			(float)line->color[0] / 255,
			(float)line->color[1] / 255,
			(float)line->color[2] / 255,
			(float)line->color[3] / 255,
		};
		line_vert2->pos = line->end;
		line_vert2->color = (vec4s){
			(float)line->color[0] / 255,
			(float)line->color[1] / 255,
			(float)line->color[2] / 255,
			(float)line->color[3] / 255,
		};
	}

	GLuint lineVBO, lineVAO;
	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &lineVBO);
	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, num_line_verts * sizeof(struct DebugVertex), line_verts, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct DebugVertex), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct DebugVertex),
			      (void *)offsetof(struct DebugVertex, color));
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_LINES, 0, num_line_verts);

	glDeleteBuffers(1, &lineVBO);
	glDeleteVertexArrays(1, &lineVAO);

	size_t num_tri_verts = num_tris * 3;
	struct DebugVertex *tri_verts = malloc(sizeof(struct DebugVertex) * num_line_verts);

	for (int i = 0; i < num_tris; i++) {
		struct DebugTri *tri = &tris[i];
		struct DebugVertex *tri_vert = &tri_verts[i * 3];
		struct DebugVertex *tri_vert2 = &tri_verts[i * 3 + 1];
		struct DebugVertex *tri_vert3 = &tri_verts[i * 3 + 2];

		tri_vert->pos = tri->v0;
		tri_vert->color = (vec4s){
			(float)tri->color[0] / 255,
			(float)tri->color[1] / 255,
			(float)tri->color[2] / 255,
			(float)tri->color[3] / 255,
		};
		tri_vert2->pos = tri->v1;
		tri_vert2->color = (vec4s){
			(float)tri->color[0] / 255,
			(float)tri->color[1] / 255,
			(float)tri->color[2] / 255,
			(float)tri->color[3] / 255,
		};

		tri_vert2->pos = tri->v2;
		tri_vert2->color = (vec4s){
			(float)tri->color[0] / 255,
			(float)tri->color[1] / 255,
			(float)tri->color[2] / 255,
			(float)tri->color[3] / 255,
		};
	}
	GLuint triVBO, triVAO;
	glGenVertexArrays(1, &triVAO);
	glGenBuffers(1, &triVBO);
	glBindVertexArray(triVAO);
	glBindBuffer(GL_ARRAY_BUFFER, triVBO);
	glBufferData(GL_ARRAY_BUFFER, num_tri_verts * sizeof(struct DebugVertex), tri_verts, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct DebugVertex), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct DebugVertex),
			      (void *)offsetof(struct DebugVertex, color));
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, num_tri_verts);

	glDeleteBuffers(1, &triVBO);
	glDeleteVertexArrays(1, &triVAO);

	glEnable(GL_DEPTH_TEST);
	free(line_verts);
	free(tri_verts);
}

void draw_fullscreen_quad(struct renderer *ren)
{
	// glViewport(0, 0, ren->win->size.x, ren->win->size.y);
	glViewport(0, 0, ren->final_fbo.width, ren->final_fbo.height);

	glBindFramebuffer(GL_FRAMEBUFFER, ren->final_fbo.id);
	glClearNamedFramebufferfv(ren->final_fbo.id, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(ren->final_fbo.id, GL_DEPTH, 0, &ren->clear_depth);
	glUseProgram(ren->fullscreen_shader);
	vec2s res = (vec2s){ 800, 600 };
	glUniform2fv(13, 1, &res.x);
	glUniform2fv(14, 1, &ren->input->relativeCursorPosition.x);
	// glBindTextureUnit(0, ren->main_fbo.targets.id);
	glBindTextureUnit(0, ren->light_fbo.targets.id);
	glBindVertexArray(ren->quad_vao);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearNamedFramebufferfv(0, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(0, GL_DEPTH, 0, &ren->clear_depth);
}

void draw_lighting(struct renderer *ren, struct resources *res, struct scene *scene, struct window *win,
		   struct physics *phys, struct camera *cam)
{
	glViewport(0, 0, ren->main_fbo.width, ren->main_fbo.height);

	const GLint clear_stencil = 0xFF;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glBindFramebuffer(GL_FRAMEBUFFER, ren->light_fbo.id);

	float color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
	glClearNamedFramebufferfv(ren->light_fbo.id, GL_COLOR, 0, &color[0]);
	// glClearNamedFramebufferfv(ren->light_fbo.id, GL_DEPTH, 0, &ren->clear_depth);

	glUseProgram(ren->fullscreen_shader);
	glDisable(GL_DEPTH_TEST);
	glBindTextureUnit(0, ren->main_fbo.targets.id);
	glBindVertexArray(ren->quad_vao);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);
	glEnable(GL_DEPTH_TEST);

	for (int i = 0; i < scene->mesh_renderers.count; i++) {
		struct mesh_renderer *mr = &scene->mesh_renderers.data[i];
		if (!mr->is_lighting)
			continue;

		glUseProgram(ren->light_shader);

		vec3s light_dir = scene->light_direction;
		glUniform3fv(11, 1, &light_dir.x);
		glUniformMatrix4fv(9, 1, GL_FALSE, &cam->viewProj.m00);
		glUniformMatrix4fv(13, 1, GL_FALSE, &mr->entity->transform->world_transform.m00);
		glUniform1f(12, ren->light_active);
		glUniform1f(23, scene->time);

		for (int i = 0; i < mr->mesh->num_sub_meshes; i++) {
			struct sub_mesh *sm = &mr->mesh->sub_meshes[i];

			// glBindTextureUnit(0, sm->mat->tex->id);
			glBindTextureUnit(0, ren->main_fbo.targets.id);

			glUniform4fv(10, 1, &sm->mat->color.r);
			glBindVertexArray(sm->vao);
			glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
		}
	}

	// glDisable(GL_DEPTH_TEST);
	// glDepthFunc(GL_GREATER);

	// glClearNamedFramebufferfv(ren->light_fbo.id, GL_DEPTH, 0, &ren->clear_depth);
}

void draw_scene(struct renderer *ren, struct resources *res, struct scene *scene, struct window *win,
		struct physics *phys, struct camera *cam)
{
	glViewport(0, 0, ren->main_fbo.width, ren->main_fbo.height);
	// glPolygonMode(GL_FRONT_AND_BACK, scene->draw_mode);
	// vec3s lightDir = { 0.4f, -0.6f, 0.1f };
	// lightDir = glms_vec3_normalize(lightDir);

	vec4s white = (vec4s){ 1.0f, 1.0f, 1.0f, 1.0f };
	vec4s orange = (vec4s){ 1.0f, 0.60f, 0.0f, 1.0f };
	// struct mesh_renderer *mr = scene->preview_model->renderer;
	// mr->mesh = &res->meshes[scene->current_model];

	const GLint clear_stencil = 0xFF;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glBindFramebuffer(GL_FRAMEBUFFER, ren->main_fbo.id);
	glClearNamedFramebufferfv(ren->main_fbo.id, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(ren->main_fbo.id, GL_DEPTH, 0, &ren->clear_depth);
	glClearNamedFramebufferiv(ren->main_fbo.id, GL_STENCIL, 0, &clear_stencil);

	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glUseProgram(ren->skybox_shader);
	mat4s sky_view = glms_mat4_ins3(glms_mat4_pick3(cam->view), GLMS_MAT4_IDENTITY);
	vec3s sky_colors = { 1.0f, 1.0f, 1.0f };
	glUniformMatrix4fv(5, 1, GL_FALSE, &sky_view.m00);
	glUniformMatrix4fv(6, 1, GL_FALSE, &cam->proj.m00);
	glUniform3fv(10, 1, &sky_colors.x);
	glBindTextureUnit(0, ren->current_skybox->id);
	glBindVertexArray(ren->skybox_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	for (int i = 0; i < scene->mesh_renderers.count; i++) {
		struct mesh_renderer *mr = &scene->mesh_renderers.data[i];
		if (mr->is_lighting)
			continue;
		// vec3s scale = mr->entity->transform->scale;
		// vec3s up_scale = glms_vec3_scale(scale, 1.01f);
		glUseProgram(ren->default_shader);

		vec3s light_dir = scene->light_direction;
		// light_dir = glms_normalize(scene->light_direction);
		glUniform3fv(11, 1, &light_dir.x);
		glUniformMatrix4fv(9, 1, GL_FALSE, &cam->viewProj.m00);
		glUniformMatrix4fv(13, 1, GL_FALSE, &mr->entity->transform->world_transform.m00);
		glUniform1f(12, ren->light_active);
		glUniform1f(23, scene->time);

		glStencilMask(0x00);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		for (int i = 0; i < mr->mesh->num_sub_meshes; i++) {
			struct sub_mesh *sm = &mr->mesh->sub_meshes[i];

			glBindTextureUnit(0, sm->mat->tex->id);

			glUniform4fv(10, 1, &sm->mat->color.r);
			glBindVertexArray(sm->vao);
			glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
		}

		// mat4s scaled_up = glms_mat4_scale(mr->entity->transform->world_transform, 1.01f);
		// glUniformMatrix4fv(13, 1, GL_FALSE, &scaled_up.m00);
		// glUniform4fv(10, 1, &orange.x);
		//
		// glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		// glStencilMask(0x00);
		// glDisable(GL_DEPTH_TEST);
		//
		// glUniform1f(12, 0.0f);
		// glUniform1f(24, 0.008f);
		// for (int i = 0; i < mr->mesh->num_sub_meshes; i++) {
		// 	struct sub_mesh *sm = &mr->mesh->sub_meshes[i];
		//
		// 	glBindTextureUnit(0, res->white_tex.id);
		// 	glBindVertexArray(sm->vao);
		// 	glDrawElements(GL_TRIANGLES, sm->ind_count, GL_UNSIGNED_INT, (void *)sm->ind_offset);
		// }
		//
		// glUniform1f(24, 0.0f);
		// glStencilMask(0xFF);
		// glStencilFunc(GL_ALWAYS, 1, 0xFF);
		// glEnable(GL_DEPTH_TEST);
	}

	// glUseProgram(ren->gui_shader);
	// vec2s scale = { 0.5f, 0.5f };
	// vec4s color = { 0.0f, 0.5f, 1.0f, 1.0f };
	// glUniform2fv(5, 1, &scale.x);
	// glUniform4fv(10, 1, &color.x);
	// glBindVertexArray(ren->quad_vao);
	// glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);

	if (phys->draw_debug)
		render_debug(ren, cam, phys->lines, phys->tris, phys->num_lines, phys->num_tris);

	char fps_text[256];

	float fps = 1 / scene->dt;
	snprintf(fps_text, 256, "%i", (int)fps);

	int xPos = 220;
	int yPos = 400;
	// glEnable(GL_SCISSOR_TEST);
	// glScissor(200, 600 / 4, 400, 300);
	//
	// for (int i = 0; i < res->num_meshes; i++) {
	// 	draw_text(ren, res, res->meshes[i].name, strlen(res->meshes[i].name), xPos,
	// 		  yPos + scene->text_offset * 20, 0.3f);

	// 	yPos -= 40;
	// }
	//
	// glDisable(GL_SCISSOR_TEST);

	draw_lighting(ren, res, scene, win, phys, cam);
	draw_text(ren, res, fps_text, strlen(fps_text), 25, 25, 1.0f);
	draw_fullscreen_quad(ren);

	// GLenum error;
	// while ((error = glGetError()) != GL_NO_ERROR) {
	// 	// Process/log the error, e.g., print the error code.
	// 	// fprintf(stderr, "OpenGL Error load func: %d\n", error);
	// }
}

void draw_sdf(struct renderer *ren, struct camera *cam, struct scene *scene)
{
	glBindFramebuffer(GL_FRAMEBUFFER, ren->light_fbo.id);
	glClearNamedFramebufferfv(ren->light_fbo.id, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(ren->light_fbo.id, GL_DEPTH, 0, &ren->clear_depth);

	glUseProgram(ren->sdf_shader);
	mat4s inv_view_proj = glms_mat4_inv(cam->viewProj);
	float res[2] = { 800.0f, 600.0f };

	glUniform2fv(5, 1, &res[0]);
	glUniformMatrix4fv(6, 1, GL_FALSE, &inv_view_proj.m00);
	vec3s light_dir = scene->light_direction;
	glUniform3fv(7, 1, &light_dir.x);
	glUniform1f(8, scene->time);
	//

	glBindVertexArray(ren->quad_vao);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);

	draw_fullscreen_quad(ren);
}

void draw(struct renderer *ren, struct resources *res, struct scene *scene, struct window *win, struct physics *phys,
	  struct camera *cam)
{
	if (ren->sdf_renderer) {
		draw_sdf(ren, cam, scene);
	} else {
		draw_scene(ren, res, scene, win, phys, cam);
	}
}

void create_framebuffer(struct renderer *ren, struct framebuffer *fb)
{
	GLenum attachments[8];
	glCreateFramebuffers(1, &fb->id);

	glCreateRenderbuffers(1, &fb->rb.id);
	glNamedRenderbufferStorage(fb->rb.id, fb->rb.internal_format, fb->width, fb->height);

	for (int i = 0; i < fb->num_targets; i++) {
		GLenum attachment = GL_COLOR_ATTACHMENT0 + i;
		attachments[i] = attachment;
		fb->targets.attachment = attachment;
		create_texture(&fb->targets, NULL);
		glNamedFramebufferTexture(fb->id, attachment, fb->targets.id, 0);
	}

	glNamedFramebufferDrawBuffers(fb->id, fb->num_targets, attachments);
	glNamedFramebufferRenderbuffer(fb->id, fb->rb.attachment, GL_RENDERBUFFER, fb->rb.id);

	if (glCheckNamedFramebufferStatus(fb->id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("ERROR::CREATE_FRAMEBUFFER::Framebuffer not complete");
	}
}

void create_special_framebuffer(struct renderer *ren, struct framebuffer *fb)
{
	GLenum attachments[8];
	glCreateFramebuffers(1, &fb->id);

	// glCreateRenderbuffers(1, &fb->rb.id);
	// glNamedRenderbufferStorage(fb->rb.id, fb->rb.internal_format, fb->width, fb->height);

	for (int i = 0; i < fb->num_targets; i++) {
		GLenum attachment = GL_COLOR_ATTACHMENT0 + i;
		attachments[i] = attachment;
		fb->targets.attachment = attachment;
		create_texture(&fb->targets, NULL);
		glNamedFramebufferTexture(fb->id, attachment, fb->targets.id, 0);
	}

	glNamedFramebufferDrawBuffers(fb->id, fb->num_targets, attachments);
	// glNamedFramebufferRenderbuffer(fb->id, ren->main_fbo.rb.attachment, GL_RENDERBUFFER, fb->rb.id);
	glNamedFramebufferRenderbuffer(fb->id, ren->main_fbo.rb.attachment, GL_RENDERBUFFER, ren->main_fbo.rb.id);

	if (glCheckNamedFramebufferStatus(fb->id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("ERROR::CREATE_FRAMEBUFFER::Framebuffer not complete");
	}
}

struct texture *get_texture(struct renderer *ren, struct arena *ren_arena)
{
	struct texture *tex = ren->tex_list.first_free_tex;

	if (tex != 0) {
		ren->tex_list.first_free_tex = ren->tex_list.next;
	} else {
		tex = alloc_struct(ren_arena, struct texture, 1);
	}

	return tex;
}

void init_renderer(struct renderer *ren, struct arena *arena, struct window *win)
{
	// ren->main_fbo.targets = get_texture(ren, arena);
	struct texture *tex = &ren->main_fbo.targets;
	struct scene *scene;

	// scene_load(scene, "test.scene");
	ren->clear_color[0] = 1.0f;
	ren->clear_color[1] = 0.0f;
	ren->clear_color[2] = 0.0f;
	ren->clear_color[3] = 1.0f;
	ren->clear_depth = 1.0f;
	ren->light_active = 1.0f;
	ren->main_fbo.width = 800;
	ren->main_fbo.height = 600;
	ren->main_fbo.rb.attachment = GL_DEPTH_STENCIL_ATTACHMENT;
	ren->main_fbo.rb.internal_format = GL_DEPTH24_STENCIL8;
	ren->main_fbo.num_targets = 1;
	ren->text_proj = glms_ortho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 1000.0f);

	tex->format = GL_RGBA;
	tex->internal_format = GL_RGBA8;
	tex->width = ren->main_fbo.width;
	tex->height = ren->main_fbo.height;
	tex->mips = false;
	tex->filter_type = GL_NEAREST;
	tex->wrap_type = GL_CLAMP_TO_EDGE;

	ren->final_fbo.width = 800;
	ren->final_fbo.height = 600;
	ren->final_fbo.rb.attachment = GL_DEPTH_ATTACHMENT;
	ren->final_fbo.rb.internal_format = GL_DEPTH_COMPONENT16;
	ren->final_fbo.num_targets = 1;

	tex = &ren->final_fbo.targets;
	tex->format = GL_RGBA;
	tex->internal_format = GL_RGBA8;
	tex->width = ren->final_fbo.width;
	tex->height = ren->final_fbo.height;
	tex->mips = false;
	tex->filter_type = GL_NEAREST;
	tex->wrap_type = GL_CLAMP_TO_EDGE;

	ren->light_fbo.width = 800;
	ren->light_fbo.height = 600;
	ren->light_fbo.rb.attachment = GL_DEPTH_ATTACHMENT;
	ren->light_fbo.rb.internal_format = GL_DEPTH_COMPONENT16;
	ren->light_fbo.num_targets = 1;

	tex = &ren->light_fbo.targets;
	tex->format = GL_RGBA;
	tex->internal_format = GL_RGBA8;
	tex->width = ren->light_fbo.width;
	tex->height = ren->light_fbo.height;
	tex->mips = false;
	tex->filter_type = GL_NEAREST;
	tex->wrap_type = GL_CLAMP_TO_EDGE;

	create_framebuffer(ren, &ren->main_fbo);
	create_framebuffer(ren, &ren->final_fbo);
	create_special_framebuffer(ren, &ren->light_fbo);

	glUseProgram(ren->text_shader);
	glUniformMatrix4fv(4, 1, GL_FALSE, &ren->text_proj.m00);
	glUseProgram(ren->default_shader);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, win->size.x, win->size.y);
}

void delete_framebuffer(struct framebuffer *fbo, struct renderer *ren)
{
	glDeleteRenderbuffers(1, &fbo->rb.id);
	for (int i = 0; i < fbo->num_targets; i++) {
		glDeleteTextures(1, &fbo->targets.id);

		ren->tex_list.next = ren->tex_list.first_free_tex;
		ren->tex_list.first_free_tex = &fbo->targets;
	}
	glDeleteFramebuffers(1, &fbo->id);
}

void reload_renderer(struct renderer *ren, struct resources *res, struct arena *arena, struct window *win)
{
	delete_framebuffer(&ren->main_fbo, ren);
	delete_framebuffer(&ren->final_fbo, ren);
	init_renderer(ren, arena, win);
}

void window_resized(struct renderer *ren, struct window *win, struct arena *arena)
{
	// glViewport(0, 0, win->size.x, win->size.y);
	// printf("window resized\n");
	// delete_framebuffer(&ren->main_fbo, ren);
	// ren->main_fbo.width = win->size.x;
	// ren->main_fbo.height = win->size.y;
	// struct texture *tex = &ren->main_fbo.targets;
	// tex->format = GL_RGBA;
	// tex->internal_format = GL_RGBA8;
	// tex->width = ren->main_fbo.width;
	// tex->height = ren->main_fbo.height;
	// tex->mips = false;
	// tex->filter_type = GL_NEAREST;
	// tex->wrap_type = GL_CLAMP_TO_EDGE;
	//
	// create_framebuffer(ren, &ren->main_fbo);
}

void test_renderer(struct renderer *ren, struct window *win)
{
	glViewport(0, 0, win->size.x, win->size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ren->clear_color[0] = 0.0f;
	ren->clear_color[1] = 0.0f;
	ren->clear_color[2] = 1.0f;
	ren->clear_color[3] = 1.0f;
	glClearNamedFramebufferfv(0, GL_COLOR, 0, &ren->clear_color[0]);
	glClearNamedFramebufferfv(0, GL_DEPTH, 0, &ren->clear_depth);
}

PETE_API void load_renderer_functions(struct renderer *ren, GLADloadproc load)
{
	printf("Loading Renderer Functions\n");
	ren->init_renderer = init_renderer;
	ren->test_renderer = test_renderer;
	ren->reload_renderer = reload_renderer;
	ren->reload_shaders = reload_shaders;
	ren->load_resources = load_resources;
	ren->scene_write = scene_write;
	ren->window_resized = window_resized;
	ren->draw_scene = draw_scene;
	ren->draw = draw;
	ren->draw_fullscreen_quad = draw_fullscreen_quad;
	ren->reload_model = reload_model;
	gladLoadGLLoader(load);

	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// glClearNamedFramebufferfv(ren->final_fbo.id, GL_COLOR, 0, &ren->clear_color[0]);
	// glClearNamedFramebufferfv(ren->final_fbo.id, GL_DEPTH, 0, &ren->clear_depth);
}
