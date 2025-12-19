#pragma once

char *get_test_string();
int get_color_(float *color);

#ifdef HOT
int (*get_color)(float *);
#else
int (*get_color)(float *) = get_color_;
#endif
