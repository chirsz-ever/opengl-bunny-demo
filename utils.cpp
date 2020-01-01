#include "utils.h"
#include <fstream>
#include <iostream>
#include <string>
#include <limits>

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
			fin >> i1;fin.get();fin.get();fin >> i1;
			fin >> i2;fin.get();fin.get();fin >> i2;
			fin >> i3;fin.get();fin.get();fin >> i3;
			faces.push_back(i1-1);
			faces.push_back(i2-1);
			faces.push_back(i3-1);
			eatline(fin);
		}
	}
}
