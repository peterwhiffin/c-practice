#include "file.h"
#include "stdio.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

const char *read_file(char *filename)
{
	long len;
	FILE *f = fopen(filename, "r");
	char *str;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	str = malloc(len + 1);
	fread(str, sizeof(char), len, f);
	str[len] = '\0';
	fclose(f);

	return str;
}

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
	char full_path[256];
	char extension[128];
	char file_name[128];
	char name[128];

	DIR *dir_stream;
	struct file_info *file_info;
	struct dirent *dir;

	dir_stream = opendir(path);

	while ((dir = readdir(dir_stream)) != NULL) {
		sprintf(full_path, "%s%s", path, dir->d_name);

		if (dir->d_type == DT_DIR) {
			if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
				sprintf(full_path, "%s/", full_path);
				get_resource_files(full_path, model_files, texture_files, model_count, texture_count);
			}

		} else if (dir->d_type == DT_REG) {
			sprintf(full_path, "%s", full_path);
			get_extension(extension, full_path);
			get_filename(file_name, full_path);
			get_filename_no_ext(name, full_path);

			if (strcmp(extension, "fbx") == 0) {
				file_info = &model_files[*model_count];
				*model_count = *model_count + 1;
			} else if (strcmp(extension, "png") == 0) {
				file_info = &texture_files[*texture_count];
				*texture_count = *texture_count + 1;
			} else {
				continue;
			}

			snprintf(file_info->path, 256, "%s", full_path);
			snprintf(file_info->extension, 128, "%s", extension);
			snprintf(file_info->filename, 128, "%s", file_name);
			snprintf(file_info->name, 128, "%s", name);
		}
	}

	closedir(dir_stream);
}
