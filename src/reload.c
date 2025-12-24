#include "sys/inotify.h"
#include <errno.h>
#include <iso646.h>
#include <stddef.h>
#include <string.h>
#include <sys/poll.h>
#include "stdio.h"
#include "syscall.h"
#include "unistd.h"
#include "poll.h"
#include "load.h"
#include "types.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define MAX_RELOADS 64

enum reload_flags { RELOAD_NONE = 0, RELOAD_SHADERS = 1 << 0, RELOAD_SOURCE = 1 << 1 };

struct reload_func {
	void (*func)(void *params);
	void *args;
};

struct notify {
	int notify_fd;
	int watch;
	char buffer[BUF_LEN];
	enum reload_flags flags;
	struct reload_func shader_func;
	struct reload_func *reloads;
	u32 num_reloads;
};

void notify_init(struct notify *notify, struct game *game, struct renderer *ren)
{
	notify->notify_fd = inotify_init();
	notify->flags = 0;
	//don't really need this. flags are enough for now.
	// notify->num_reloads = 0;
	// notify->shader_func.func = (void *)game->reload_shaders;
	// notify->shader_func.args = (void *)ren;
	// notify->reloads = malloc(sizeof(struct reload_func) * MAX_RELOADS);

	if (notify->notify_fd < 0) {
		printf("ERROR::INOTIFY_INIT::%s\n", strerror(errno));
		return;
	}

	notify->watch = inotify_add_watch(notify->notify_fd, "../../src/shaders", IN_MODIFY);

	if (notify->watch < 0) {
		printf("ERROR::INOTIFY_ADD_WATCH::%s\n", strerror(errno));
		return;
	}
}

void check_modified(struct notify *notify, struct game *game, struct renderer *ren)
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
			if (event->len) {
				if (event->mask & IN_MODIFY) {
					if (notify->flags & RELOAD_SHADERS)
						continue;

					game->reload_shaders(ren);

					// notify->flags |= RELOAD_SHADERS;
					// notify->reloads[notify->num_reloads] = notify->shader_func;
					// notify->num_reloads++;
				}
			}

			i += EVENT_SIZE + event->len;
		}
	}

	// for (int i = 0; i < notify->num_reloads; i++) {
	// 	struct reload_func *reload = &notify->reloads[i];
	// 	reload->func(reload->args);
	// }
	//
	// notify->flags = 0;
	// notify->num_reloads = 0;
}
