#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include "types.h"

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
	input->lock_mouse = SDL_SetWindowRelativeMouseMode;
	input->del.state = CANCELED;
	input->space.state = CANCELED;
	input->movement.state = CANCELED;
	input->movement.value_type = COMPOSITE;
	input->arrows.state = CANCELED;
	input->arrows.value_type = COMPOSITE;
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

void set_key_state(struct key_state *key, float *value)
{
	float activated = 0;

	switch (key->value_type) {
	case BUTTON:
		key->value.raw[0] = value[0];
		break;
	case AXIS:
		break;
	case COMPOSITE:
		key->value.x = value[0] + value[1];
		key->value.y = value[2] + value[3];
		break;
	}

	activated = key->value.x || key->value.y;

	switch (key->state) {
	case STARTED:
		if (!activated) {
			key->state = CANCELED;
		} else {
			key->state = ACTIVE;
		}
		break;
	case ACTIVE:
		if (!activated)
			key->state = CANCELED;
		break;
	case CANCELED:
		if (activated)
			key->state = STARTED;
		break;
	}
}

void check_input(struct input *input)
{
	SDL_MouseButtonFlags mouseButtonMask =
		SDL_GetGlobalMouseState(&input->cursorPosition.x, &input->cursorPosition.y);

	input->lookX = input->cursorPosition.x - input->oldX;
	input->lookY = input->oldY - input->cursorPosition.y;
	input->oldX = input->cursorPosition.x;
	input->oldY = input->cursorPosition.y;

	set_key_state(&input->mouse0, (float[1]){ mouseButtonMask & SDL_BUTTON_LMASK });
	set_key_state(&input->mouse1, (float[1]){ mouseButtonMask & SDL_BUTTON_RMASK });
	set_key_state(&input->del, (float[1]){ input->keyStates[SDL_SCANCODE_DELETE] });
	set_key_state(&input->space, (float[1]){ input->keyStates[SDL_SCANCODE_SPACE] });
	set_key_state(&input->movement, (float[4]){
						input->keyStates[SDL_SCANCODE_A],
						-input->keyStates[SDL_SCANCODE_D],
						-input->keyStates[SDL_SCANCODE_S],
						input->keyStates[SDL_SCANCODE_W],
					});
	set_key_state(&input->arrows, (float[4]){
					      input->keyStates[SDL_SCANCODE_LEFT],
					      -input->keyStates[SDL_SCANCODE_RIGHT],
					      -input->keyStates[SDL_SCANCODE_DOWN],
					      input->keyStates[SDL_SCANCODE_UP],
				      });
}

void load_lib(struct game *game)
{
	char *error;
	const char *compile_command =
		"clang -g -fPIC -c ../../src/game.c -I../../../cglm/include -I../../../glad/include -I../../../cglm/include -I../../../SDL/include  -o game.o";
	const char *link_command = "clang -shared -lSDL3 -lm -o libgamelib.so game.o";

	if (game->lib_handle != NULL) {
		dlclose(game->lib_handle);
	}

	//need to actually handle status.
	int status = system(compile_command);
	status = system(link_command);
	game->lib_handle = dlopen("./libgamelib.so", RTLD_LAZY);

	if (!game->lib_handle) {
		printf("handle error\n");
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	game->load_functions = dlsym(game->lib_handle, "load_game_functions");

	if ((error = dlerror()) != NULL) {
		printf("dlsym error\n");
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}

	game->load_functions(game, (GLADloadproc)SDL_GL_GetProcAddress);
}

void check_modified(struct game *game, struct window *win, struct input *input, struct renderer *ren)
{
	//need to figure out inotify/readdirectorychangesW???
	struct stat file_stat;
	stat("../../src/game.c", &file_stat);

	if (file_stat.st_mtim.tv_sec > game->last_lib_time) {
		game->last_lib_time = file_stat.st_mtim.tv_sec;
		printf("lib modified\n");
		load_lib(game);
	}

	stat("../../src/shaders/default.frag", &file_stat);

	if (file_stat.st_mtim.tv_sec > game->last_frag_time) {
		game->last_frag_time = file_stat.st_mtim.tv_sec;
		printf("fragment shader modified\n");
		game->reload_shaders(ren);
	}

	stat("../../src/shaders/default.vert", &file_stat);

	if (file_stat.st_mtim.tv_sec > game->last_vert_time) {
		game->last_vert_time = file_stat.st_mtim.tv_sec;
		printf("fragment shader modified\n");
		game->reload_shaders(ren);
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
	struct game *game = malloc(sizeof(*game));
	struct window *win = malloc(sizeof(*win));
	struct input *input = malloc(sizeof(*input));
	struct resources *res = malloc(sizeof(*res));
	struct renderer *renderer = malloc(sizeof(*renderer));
	struct scene *scene = malloc(sizeof(*scene));

	game->lib_handle = NULL;
	scene->entities = malloc(sizeof(struct entity) * 4096);
	scene->cameras = malloc(sizeof(struct camera) * 32);
	scene->transforms = malloc(sizeof(struct transform) * 4096);
	scene->renderers = malloc(sizeof(struct mesh_renderer) * 4096);

	window_init(win, input);
	load_lib(game);

	game->init_renderer(renderer);
	game->load_resources(res, renderer);
	game->init_scene(scene, res);

	while (!win->should_close) {
		check_modified(game, win, input, renderer);
		update_time(scene);
		poll_events(win);
		check_input(input);
		game->update(scene, input, res, renderer, win);
		game->draw_scene(renderer, res, scene, win);
		SDL_GL_SwapWindow(win->sdl_win);
	}

	dlclose(game->lib_handle);
	return 0;
}
