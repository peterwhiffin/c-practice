#include "transform.h"
#include "cglm/struct/quat.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "../types.h"

float vec3_magnitude(vec3s v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3s vec3_normalize(vec3s v)
{
	float mag = vec3_magnitude(v);
	v.x = v.y / mag;
	v.y = v.y / mag;
	v.z = v.z / mag;
	return v;
}
vec3s quat_by_vec3(versors rotation, vec3s point)
{
	vec3s vector;
	float num = rotation.x * 2.0f;
	float num2 = rotation.y * 2.0f;
	float num3 = rotation.z * 2.0f;
	float num4 = rotation.x * num;
	float num5 = rotation.y * num2;
	float num6 = rotation.z * num3;
	float num7 = rotation.x * num2;
	float num8 = rotation.x * num3;
	float num9 = rotation.y * num3;
	float num10 = rotation.w * num;
	float num11 = rotation.w * num2;
	float num12 = rotation.w * num3;
	vector.x = (((1.0f - (num5 + num6)) * point.x) + ((num7 - num12) * point.y)) + ((num8 + num11) * point.z);
	vector.y = (((num7 + num12) * point.x) + ((1.0f - (num4 + num6)) * point.y)) + ((num9 - num10) * point.z);
	vector.z = (((num8 - num11) * point.x) + ((num9 + num10) * point.y)) + ((1.0f - (num4 + num5)) * point.z);
	return vector;
}

vec3s get_right(struct transform *t)
{
	return glms_vec3_normalize(quat_by_vec3(t->rot, (vec3s){ 1.0f, 0.0f, 0.0f }));
	// return glms_vec3_normalize(glms_quat_rotatev(glms_quat_normalize(t->rot), (vec3s){ 1.0f, 0.0f, 0.0f }));
}

vec3s get_up(struct transform *t)
{
	return glms_vec3_normalize(quat_by_vec3(t->rot, (vec3s){ 0.0f, 1.0f, 0.0f }));
	// return glms_vec3_normalize(glms_quat_rotatev(glms_quat_normalize(t->rot), (vec3s){ 0.0f, 1.0f, 0.0f }));
}

vec3s get_forward(struct transform *t)
{
	return glms_vec3_normalize(quat_by_vec3(t->rot, (vec3s){ 0.0f, 0.0f, 1.0f }));
	// return glms_vec3_normalize(glms_quat_rotatev(glms_quat_normalize(t->rot), (vec3s){ 0.0f, 0.0f, 1.0f }));
}

static void update_transform_matrices(struct transform *t)
{
	mat4s T = glms_translate(GLMS_MAT4_IDENTITY, t->pos);
	mat4s R = glms_quat_mat4(t->rot);
	mat4s S = glms_scale(GLMS_MAT4_IDENTITY, t->scale);
	t->world_transform = glms_mat4_mul(T, glms_mat4_mul(R, S));
}

void set_position(struct transform *t, vec3s pos)
{
	t->pos = pos;
	update_transform_matrices(t);
}

void set_rotation(struct transform *t, versors rot)
{
	t->rot = glms_quat_normalize(rot);
	update_transform_matrices(t);
}

void set_scale(struct transform *t, vec3s scale)
{
	t->scale = scale;
	update_transform_matrices(t);
}
