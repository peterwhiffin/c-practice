#include "string.h"
#include "stdio.h"
#include "../types.h"

void get_extension(char *ext, char *path)
{
	char *dot = strrchr(path, '.');
	snprintf(ext, 256, "%s", dot + 1);
}

void get_filename(char *name, const char *path)
{
	char *slash = strrchr(path, '/');
	snprintf(name, 256, "%s", slash + 1);
}

void get_filename_no_ext(char *name, const char *path)
{
	char file_name[256];
	get_filename(file_name, path);

	char *dot = strrchr(file_name, '.');
	*dot = '\0';

	char *under = strrchr(file_name, '_');

	if (under && strcmp(under + 1, "TOM") == 0) {
		*under = '\0';
	}

	snprintf(name, 256, "%s", file_name);
}

void get_resource_files(char *path, struct file_info *model_files, struct file_info *texture_files, size_t *model_count,
			size_t *texture_count)
{
}
