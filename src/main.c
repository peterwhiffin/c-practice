#define CGLM_FORCE_LEFT_HANDED
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_mouse.h"
#include "cglm/types-struct.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_video.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "arena.h"
#include "types.h"

#include "renderer/parse.c"
#include "arena.c"
#include "renderer/file.c"
#include "reload.c"

struct arena main_arena;
struct arena render_arena;

void window_init(struct window *win, struct input *input)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	win->sdl_win = SDL_CreateWindow("practice", 800, 600, flags);
	win->ctx = SDL_GL_CreateContext(win->sdl_win);

	if (!win->ctx) {
		printf("SDL GL CONTEXT NOT CREATED\n");
	}

	SDL_GL_MakeCurrent(win->sdl_win, win->ctx);
	SDL_GL_SetSwapInterval(1);
	SDL_SetWindowPosition(win->sdl_win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	win->should_close = false;
	win->size.x = 800;
	win->size.y = 600;

	input->sdl_keys = SDL_GetKeyboardState(NULL);
	input->lock_mouse = SDL_SetWindowRelativeMouseMode;
}

void poll_events(struct window *win, struct input *input, struct renderer *ren, struct editor *editor,
		 struct arena *ren_arena)
{
	SDL_Event event;

	input->actions[MOUSE_DELTA].composite = (vec2s){ 0.0f, 0.0f };

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_MOUSE_MOTION:
			input->actions[MOUSE_DELTA].composite = (vec2s){ event.motion.xrel, -event.motion.yrel };
		case SDL_EVENT_MOUSE_WHEEL:
			input->actions[MWHEEL].axis = event.wheel.y;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			win->size.x = event.window.data1;
			win->size.y = event.window.data2;
			win->aspect = (float)event.window.data1 / event.window.data2;
			// ren->window_resized(ren, win, ren_arena);
			break;
		case SDL_EVENT_QUIT:
			win->should_close = true;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			win->should_close = true;
			break;
		}

		editor->process_event(&event);
	}
}

void check_input(struct input *input)
{
	float oldX = input->relativeCursorPosition.x;
	float oldY = input->relativeCursorPosition.y;

	SDL_GetMouseState(&input->relativeCursorPosition.x, &input->relativeCursorPosition.y);
	SDL_MouseButtonFlags mouseButtonFlags =
		SDL_GetGlobalMouseState(&input->cursorPosition.x, &input->cursorPosition.y);

	input->actions[MWHEEL].axis = 0.0f;
	input->actions[M0].pushed = mouseButtonFlags & SDL_BUTTON_LMASK;
	input->actions[M1].pushed = mouseButtonFlags & SDL_BUTTON_RMASK;
	input->actions[DEL].pushed = input->sdl_keys[SDL_SCANCODE_DELETE];
	input->actions[D].pushed = input->sdl_keys[SDL_SCANCODE_D];
	input->actions[F].pushed = input->sdl_keys[SDL_SCANCODE_F];
	input->actions[P].pushed = input->sdl_keys[SDL_SCANCODE_P];
	input->actions[L].pushed = input->sdl_keys[SDL_SCANCODE_L];
	input->actions[SPACE].pushed = input->sdl_keys[SDL_SCANCODE_SPACE];
	input->actions[LSHIFT].pushed = input->sdl_keys[SDL_SCANCODE_LSHIFT];
	input->actions[LCTRL].pushed = input->sdl_keys[SDL_SCANCODE_LCTRL];
	input->actions[WASD].composite = (vec2s){ input->sdl_keys[SDL_SCANCODE_D] - input->sdl_keys[SDL_SCANCODE_A],
						  input->sdl_keys[SDL_SCANCODE_W] - input->sdl_keys[SDL_SCANCODE_S] };
	input->actions[ARROWS].composite = (vec2s){
		input->sdl_keys[SDL_SCANCODE_LEFT] - input->sdl_keys[SDL_SCANCODE_RIGHT],
		input->sdl_keys[SDL_SCANCODE_DOWN] - input->sdl_keys[SDL_SCANCODE_UP],
	};

	for (int i = 0; i < ACTION_COUNT; i++) {
		struct key_action *action = &input->actions[i];

		switch (action->state) {
		case STARTED:
			action->state = action->pushed ? ACTIVE : CANCELED;
			break;
		case ACTIVE:
			if (!action->pushed)
				action->state = CANCELED;
			break;
		case CANCELED:
			if (action->pushed)
				action->state = STARTED;
			break;
		}
	}
}

void update_time(struct scene *scene)
{
	float time = SDL_GetTicks() / 1000.0f;
	scene->dt = time - scene->time;
	scene->time = time;
}

int main()
{
	main_arena = get_new_arena((size_t)1 << 30);
	render_arena = get_new_arena((size_t)1 << 30);
	struct notify *notify = alloc_struct(&main_arena, typeof(*notify), 1);
	struct game *game = alloc_struct(&main_arena, typeof(*game), 1);
	struct physics *physics = alloc_struct(&main_arena, typeof(*physics), 1);
	struct window *win = alloc_struct(&main_arena, typeof(*win), 1);
	struct input *input = alloc_struct(&main_arena, typeof(*input), 1);
	struct resources *res = alloc_struct(&main_arena, typeof(*res), 1);
	struct renderer *ren = alloc_struct(&main_arena, typeof(*ren), 1);
	struct scene *scene = alloc_struct(&main_arena, typeof(*scene), 1);
	struct editor *editor = alloc_struct(&main_arena, typeof(*editor), 1);

	game->lib_handle = NULL;
	ren->lib_handle = NULL;
	ren->win = win;
	ren->input = input;
	scene->entities = alloc_struct(&main_arena, typeof(*scene->entities), 4096);
	scene->cameras = alloc_struct(&main_arena, typeof(*scene->cameras), 32);
	scene->transforms = alloc_struct(&main_arena, typeof(*scene->transforms), 4096);
	scene->renderers = alloc_struct(&main_arena, typeof(*scene->renderers), 4096);
	editor->ren = ren;
	editor->res = res;
	editor->input = input;
	editor->win = win;
	editor->scene = scene;
	editor->game = game;
	editor->physics = physics;

	window_init(win, input);
	load_game_lib(game);
	load_renderer(ren);
	load_physics_lib(physics);
	load_editor_lib(editor);
	file_watch_init(notify);

	ren->init_renderer(ren, &render_arena, win);
	ren->load_resources(res, ren, &render_arena);
	game->init_scene(scene, res);
	physics->physics_init(physics, scene, &main_arena);
	editor->init_editor(win, editor);

	scene_load(scene, physics, game, res, "test.scene");
	update_time(scene);

	while (!win->should_close) {
		check_modified(notify, game, ren, res, &main_arena, &render_arena, win, editor);
		update_time(scene);
		check_input(input);
		poll_events(win, input, ren, editor, &render_arena);
		physics->step_physics(physics, scene, game, scene->dt);
		game->update(scene, input, res, ren, win, physics, game);
		ren->draw_scene(ren, res, scene, win, physics);
		editor->update_editor(editor);
		SDL_GL_SwapWindow(win->sdl_win);
	}

	file_watch_close(notify->notify_fd);
	close_lib(game->lib_handle);
	close_lib(ren->lib_handle);
	close_lib(editor->lib_handle);
	close_lib(physics->lib_handle);
	return 0;
}
