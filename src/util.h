#ifndef _util_h_
#define _util_h_

#include "config.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define PI 3.14159265359
#define DEGREES(radians) ((radians)*180 / PI)
#define RADIANS(degrees) ((degrees)*PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

GLuint gen_buffer(GLsizei size, GLfloat *data);
void del_buffer(GLuint buffer);
GLfloat *malloc_faces(int components, int faces);
GLuint gen_faces(int components, int faces, GLfloat *data);
GLuint make_shader(GLenum type, const char *source);
GLuint load_shader(GLenum type, const char *path);
GLuint make_program(GLuint shader1, GLuint shader2);
GLuint load_program(const char *path1, const char *path2);
void load_png_texture(const char *file_name);
void normalize(float *x, float *y, float *z);
void mat_identity(float *matrix);
void mat_translate(float *matrix, float dx, float dy, float dz);
void mat_rotate(float *matrix, float x, float y, float z, float angle);
void mat_vec_multiply(float *vector, float *a, float *b);
void mat_multiply(float *matrix, float *a, float *b);
void mat_apply(float *data, float *matrix, int count, int offset, int stride);
void frustum_planes(float planes[6][4], int radius, float *matrix);
void mat_frustum(float *matrix, float left, float right, float bottom,
                 float top, float znear, float zfar);
void mat_perspective(float *matrix, float fov, float aspect, float near,
                     float far);
void mat_ortho(float *matrix, float left, float right, float bottom, float top,
               float near, float far);
void set_matrix_2d(float *matrix, int width, int height);
void set_matrix_3d(float *matrix, int width, int height, float x, float y,
                   float z, float rx, float ry, float fov, int ortho,
                   int radius);
void set_matrix_item(float *matrix, int width, int height, int scale);
int chunked(float x);

#endif
