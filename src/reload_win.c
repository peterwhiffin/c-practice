#include "types.h"
#include "windows.h"
#include <stdio.h>

enum reload_flags { RELOAD_NONE = 0, RELOAD_SHADERS = 1 << 0, RELOAD_RENDERER = 1 << 1, RELOAD_GAME = 1 << 2 };

struct notify {
	int notify_fd;
	int shader_watch;
	int renderer_watch;
	int game_watch;
	int resource_watch;
	enum reload_flags flags;
	u32 num_reloads;
};

void load_renderer(struct renderer *ren)
{
	// if (ren->lib_handle)
	// 	FreeLibrary(ren->lib_handle);

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
	// if (game->lib_handle)
	// 	FreeLibrary(game->lib_handle);

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
	// if (editor->lib_handle)
	// 	FreeLibrary(editor->lib_handle);

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
	// if (physics->lib_handle)
	// 	FreeLibrary(physics->lib_handle);

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
