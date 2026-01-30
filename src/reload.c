#include <errno.h>
#include <iso646.h>
#include <stddef.h>
#include <string.h>
#include "stdio.h"
#include "renderer/load.h"
#include "types.h"

#if defined(__linux__)
#include "poll.h"
#include "unistd.h"
#include "syscall.h"
#include <dlfcn.h>
#include <sys/poll.h>
#include "sys/inotify.h"
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#elif defined(_WIN32)
// #define EVENT_SIZE (sizeof(struct inotify_event))
// #define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define BUF_LEN (1024)
#include <windows.h>
#endif

enum reload_flags {
	RELOAD_NONE = 0,
	RELOAD_SHADERS = 1 << 0,
	RELOAD_RENDERER = 1 << 1,
	RELOAD_GAME = 1 << 2,
	RELOAD_EDITOR = 1 << 3
};

struct notify {
	int notify_fd;
	int shader_watch;
	int renderer_watch;
	int game_watch;
	int resource_watch;
	int editor_watch;
	char buffer[BUF_LEN];
	enum reload_flags flags;
	u32 num_reloads;
};

#if defined(__linux__)
void load_renderer(struct renderer *ren)
{
	char *error;

	const char *build_command = "../../src/renderer/build.sh";
	if (ren->lib_handle != NULL) {
		dlclose(ren->lib_handle);
	}

	//need to actually handle status.
	int status = system(build_command);
	ren->lib_handle = dlopen("./librendererlib.so", RTLD_LAZY);

	if (!ren->lib_handle) {
		printf("handle error\n");
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	ren->load_functions = dlsym(ren->lib_handle, "load_renderer_functions");

	if ((error = dlerror()) != NULL) {
		printf("dlsym error\n");
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}

	ren->load_functions(ren, (GLADloadproc)SDL_GL_GetProcAddress);
}

void load_game_lib(struct game *game)
{
	char *error;

	const char *build_command = "../../src/game/build.sh";
	if (game->lib_handle != NULL) {
		dlclose(game->lib_handle);
	}

	//need to actually handle status.
	int status = system(build_command);
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

	game->load_functions(game);
}

void load_editor_lib(struct editor *editor)
{
	char *error;

	const char *build_command = "../../src/editor/build.sh";
	if (editor->lib_handle != NULL) {
		dlclose(editor->lib_handle);
	}

	//need to actually handle status.
	int status = system(build_command);
	editor->lib_handle = dlopen("./libeditorlib.so", RTLD_LAZY);

	if (!editor->lib_handle) {
		fprintf(stderr, "%s%s\n", "DLOPEN::", dlerror());
		exit(EXIT_FAILURE);
	}

	editor->load_functions = dlsym(editor->lib_handle, "load_editor_functions");

	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "%s%s\n", "DLSYM::", error);
		exit(EXIT_FAILURE);
	}

	editor->load_functions(editor);
}

void load_physics_lib(struct physics *physics)
{
	char *error;

	const char *build_command = "../../src/physics/build.sh";
	if (physics->lib_handle != NULL) {
		dlclose(physics->lib_handle);
	}

	//need to actually handle status.
	int status = system(build_command);
	physics->lib_handle = dlopen("./libphysicslib.so", RTLD_LAZY);

	if (!physics->lib_handle) {
		fprintf(stderr, "%s%s\n", "DLOPEN::", dlerror());
		exit(EXIT_FAILURE);
	}

	physics->load_functions = dlsym(physics->lib_handle, "load_physics_functions");

	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "%s%s\n", "DLSYM::", error);
		exit(EXIT_FAILURE);
	}

	physics->load_functions(physics);
}

void file_watch_init(struct notify *notify)
{
	notify->notify_fd = inotify_init();
	notify->flags = 0;

	if (notify->notify_fd < 0) {
		printf("ERROR::INOTIFY_INIT::%s\n", strerror(errno));
		return;
	}

	notify->shader_watch = inotify_add_watch(notify->notify_fd, "../../src/shaders", IN_MODIFY);
	notify->renderer_watch = inotify_add_watch(notify->notify_fd, "../../src/renderer", IN_MODIFY);
	notify->game_watch = inotify_add_watch(notify->notify_fd, "../../src/game", IN_MODIFY);
	notify->resource_watch = inotify_add_watch(notify->notify_fd, "../../res/export/pete", IN_MODIFY);
	notify->editor_watch = inotify_add_watch(notify->notify_fd, "../../src/editor", IN_MODIFY);

	if (notify->shader_watch < 0) {
		printf("ERROR::INOTIFY_ADD_WATCH::%s\n", strerror(errno));
		return;
	}
}

void check_modified(struct notify *notify, struct game *game, struct renderer *ren, struct resources *res,
		    struct arena *main_arena, struct arena *render_arena, struct window *win, struct editor *editor)
{
	struct pollfd fdset[1];
	int nfds = 1;
	int num_fd;
	int length, i = 0;
	fdset[0].fd = notify->notify_fd;
	fdset[0].events = POLLIN;

	num_fd = poll(fdset, nfds, 0);

	if (num_fd < 0) {
		printf("ERROR::POLL::%s\n", strerror(errno));
	}

	if (fdset[0].revents & POLLIN) {
		length = read(notify->notify_fd, notify->buffer, BUF_LEN);

		if (length < 0) {
			printf("ERROR::READ_NOTIFY_FD::%s\n", strerror(errno));
		}

		while (i < length) {
			struct inotify_event *event = (struct inotify_event *)&notify->buffer[i];
			printf("event name: %s\n", event->name);
			if (event->mask & IN_MODIFY) {
				if (event->wd == notify->shader_watch) {
					if (!(notify->flags & RELOAD_SHADERS)) {
						notify->flags |= RELOAD_SHADERS;
						ren->reload_shaders(ren);
					}
				} else if (event->wd == notify->renderer_watch) {
					if (!(notify->flags & RELOAD_RENDERER)) {
						notify->flags |= RELOAD_RENDERER;
						load_renderer(ren);
						ren->reload_renderer(ren, res, render_arena, win);
					}
				} else if (event->wd == notify->game_watch) {
					if (!(notify->flags & RELOAD_GAME)) {
						notify->flags |= RELOAD_GAME;
						load_game_lib(game);
					}
				} else if (event->wd == notify->editor_watch) {
					if (!(notify->flags & RELOAD_EDITOR)) {
						notify->flags |= RELOAD_EDITOR;
						load_editor_lib(editor);
						editor->init_editor(win, editor);
					}
				} else if (event->wd == notify->resource_watch) {
					printf("resource changed: %s\n", event->name);
					for (int i = 0; i < res->num_models; i++) {
						struct model_import *model = &res->model_imports[i];
						printf("model name: %s - event name: %s\n", model->file_info.filename,
						       event->name);

						if (strcmp(model->file_info.filename, event->name) == 0) {
							ren->reload_model(res, ren, model);
						}
					}
				}
			}

			i += EVENT_SIZE + event->len;
		}
	}

	notify->flags = 0;
}

void close_lib(void *handle)
{
	dlclose(handle);
}

void file_watch_close(int handle)
{
	close(handle);
}

#elif defined(_WIN32)
void load_renderer(struct renderer *ren)
{
	if (ren->lib_handle)
		FreeLibrary(ren->lib_handle);

	int success = system("..\\..\\src\\renderer\\build.bat");

	printf("renderer build: %u\n", success);
	ren->lib_handle = LoadLibrary(TEXT("renderer.dll"));

	if (ren->lib_handle != NULL) {
		ren->load_functions =
			(typeof(ren->load_functions))GetProcAddress(ren->lib_handle, "load_renderer_functions");

		if (ren->load_functions != NULL) {
			ren->load_functions(ren, (GLADloadproc)SDL_GL_GetProcAddress);
		} else {
			printf("couldn't load renderer functions\n");
		}
	} else {
		DWORD error_code = GetLastError();
		printf("couldn't load renderer dll. Error code: %lu\n", error_code);
	}
}

void load_game_lib(struct game *game)
{
	if (game->lib_handle)
		FreeLibrary(game->lib_handle);

	int success = system("..\\..\\src\\game\\build.bat");
	printf("game build: %u\n", success);

	game->lib_handle = LoadLibrary(TEXT("game.dll"));

	if (game->lib_handle != NULL) {
		game->load_functions =
			(typeof(game->load_functions))GetProcAddress(game->lib_handle, "load_game_functions");

		if (game->load_functions != NULL) {
			game->load_functions(game);
		} else {
			printf("couldn't load game functions\n");
		}
	} else {
		printf("couldn't load game dll\n");
	}
}

void load_editor_lib(struct editor *editor)
{
	if (editor->lib_handle)
		FreeLibrary(editor->lib_handle);

	int success = system("..\\..\\src\\editor\\build.bat");
	printf("editor build: %u\n", success);

	editor->lib_handle = LoadLibrary(TEXT("editor.dll"));

	if (editor->lib_handle != NULL) {
		editor->load_functions =
			(typeof(editor->load_functions))GetProcAddress(editor->lib_handle, "load_editor_functions");

		if (editor->load_functions != NULL) {
			editor->load_functions(editor);
		} else {
			printf("couldn't load editor functions\n");
		}
	} else {
		printf("couldn't load editor dll\n");
	}
}

void load_physics_lib(struct physics *physics)
{
	if (physics->lib_handle)
		FreeLibrary(physics->lib_handle);

	int success = system("..\\..\\src\\physics\\build.bat");
	printf("physics build: %u\n", success);

	physics->lib_handle = LoadLibrary(TEXT("physics.dll"));

	if (physics->lib_handle != NULL) {
		physics->load_functions =
			(typeof(physics->load_functions))GetProcAddress(physics->lib_handle, "load_physics_functions");

		if (physics->load_functions != NULL) {
			physics->load_functions(physics);
		} else {
			printf("couldn't load physics functions\n");
		}
	} else {
		printf("couldn't load physics dll\n");
	}
}

void file_watch_init(struct notify *notify)
{
}

void check_modified(struct notify *notify, struct game *game, struct renderer *ren, struct resources *res,
		    struct arena *main_arena, struct arena *render_arena, struct window *win, struct editor *editor)
{
}

void file_watch_close(int handle)
{
}

void close_lib(void *lib_handle)
{
	FreeLibrary(lib_handle);
}
#endif
