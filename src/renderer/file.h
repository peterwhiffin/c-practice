#pragma once
#include "../types.h"

const char *read_file(char *filename);
void get_extension(char *ext, char *path);
void get_filename(char *name, const char *path);
void get_filename_no_ext(char *name, const char *path);
void get_resource_files(char *path, struct file_info *model_files, struct file_info *texture_files, size_t *model_count,
			size_t *texture_count);
