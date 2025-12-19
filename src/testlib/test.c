#include "test.h"
char *get_test_string()
{
	return "help me\0";
}

int get_color_(float *color)
{
	color[0] = 0.0f;
	color[1] = 0.0f;
	color[2] = 0.0f;
	color[3] = 1.0f;
	return 33;
}
