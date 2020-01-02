#include "utils.h"
#include <fstream>
#include <iostream>
#include <string>
#include <limits>
#include <cmath>

using namespace std;

static void eatline(std::istream& input)
{
	input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void load_bunny_data(const char file[], std::vector<GLfloat>& vertices, std::vector<GLuint>& faces, std::vector<GLfloat>& normals)
{
	ifstream fin;
	fin.open(file);
	if (!fin.is_open()) {
		printf("Open `%s` failed\n", file);
		exit(1);
	}

	// 开始读取文件
	string token;
	float x, y, z;
	GLuint i1, i2, i3;
	while (!fin.eof()) {
		fin >> token;
		if (token == "#") {
			eatline(fin);
		} else if (token == "v") {
			fin >> x >> y >> z;
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);
			eatline(fin);
		} else if (token == "vn") {
			fin >> x >> y >> z;
			normals.push_back(x);
			normals.push_back(y);
			normals.push_back(z);
			eatline(fin);
		} else if (token == "f") {
			fin >> i1; fin.get(); fin.get(); fin >> i1;
			fin >> i2; fin.get(); fin.get(); fin >> i2;
			fin >> i3; fin.get(); fin.get(); fin >> i3;
			faces.push_back(i1 - 1);
			faces.push_back(i2 - 1);
			faces.push_back(i3 - 1);
			eatline(fin);
		}
	}
}

void drawSolidSphere(GLfloat radius, GLint slices, GLint stacks)
{
	float step_z = M_PI / slices;
	float step_xy = 2 * M_PI / stacks;
	float x[4], y[4], z[4];

	float angle_z = 0.0;
	float angle_xy = 0.0;
	int i = 0, j = 0;
	glBegin(GL_QUADS);
	for (i = 0; i < slices; i++) {
		angle_z = i * step_z;

		for (j = 0; j < stacks; j++) {
			angle_xy = j * step_xy;
			x[0] = radius * sin(angle_z) * cos(angle_xy);
			y[0] = radius * sin(angle_z) * sin(angle_xy);
			z[0] = radius * cos(angle_z);

			x[1] = radius * sin(angle_z + step_z) * cos(angle_xy);
			y[1] = radius * sin(angle_z + step_z) * sin(angle_xy);
			z[1] = radius * cos(angle_z + step_z);

			x[2] = radius * sin(angle_z + step_z) * cos(angle_xy + step_xy);
			y[2] = radius * sin(angle_z + step_z) * sin(angle_xy + step_xy);
			z[2] = radius * cos(angle_z + step_z);

			x[3] = radius * sin(angle_z) * cos(angle_xy + step_xy);
			y[3] = radius * sin(angle_z) * sin(angle_xy + step_xy);
			z[3] = radius * cos(angle_z);
			for (int k = 0; k < 4; k++) {
				glVertex3f(x[k], y[k], z[k]);
			}
		}
	}
	glEnd();
}
