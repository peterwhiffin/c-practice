#include "sys/inotify.h"
#include <errno.h>
#include <iso646.h>
#include <stddef.h>
#include <string.h>
#include <sys/poll.h>
#include <dlfcn.h>
#include "stdio.h"
#include "syscall.h"
#include "unistd.h"
#include "poll.h"
#include "renderer/load.h"
#include "types.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define MAX_RELOADS 64

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

	ren->load_functions = dlsym(ren->lib_handle, "load_functions");

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

	game->load_functions = dlsym(game->lib_handle, "load_functions");

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

	editor->load_functions = dlsym(editor->lib_handle, "load_functions");

	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "%s%s\n", "DLSYM::", error);
		exit(EXIT_FAILURE);
	}

	editor->load_functions(editor);
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
