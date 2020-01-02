#ifndef UTILS_H__
#define UTILS_H__

#include <vector>
#include <GL/gl.h>

void load_bunny_data(const char file[], std::vector<GLfloat>& vertices, std::vector<GLuint>& faces, std::vector<GLfloat>& normals);
void drawSolidSphere(GLfloat radius, GLint slices, GLint stacks);

#endif
