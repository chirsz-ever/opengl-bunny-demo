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

void load_bunny_data(const char file[], std::vector<GLfloat>& vertices, std::vector<GLuint>& faces)
{
	ifstream fin;
	fin.open(file);
	if (!fin.is_open()) {
		printf("Open `%s` failed\n", file);
		exit(1);
	}

	// 开始读取文件
	size_t vertexNum = 0, faceNum = 0;
	string token;
	while (!fin.eof()) {
		fin >> token;
		if (token == "element") {
			fin >> token;
			if (token == "vertex") {
				fin >> vertexNum;
				vertices.reserve(vertexNum * 3);
			} else if (token == "face") {
				fin >> faceNum;
				faces.reserve(faceNum * 3);
			}
			eatline(fin);
		} else if (token == "end_header") {
			eatline(fin);
			GLfloat x, y, z;
			GLfloat confidence;
			GLfloat intensity;

			// 添加点信息到数组
			for (size_t i = 0; i < vertexNum; ++i) {
				fin >> x;
				fin >> y;
				fin >> z;

				vertices.push_back(x);
				vertices.push_back(y);
				vertices.push_back(z);

				fin >> confidence;
				fin >> intensity;

			}

			GLuint idx1, idx2, idx3;
			size_t nums;

			// 添加面信息到数组
			for (size_t i = 0; i < faceNum; ++i) {
				fin >> nums;
				if (nums == 3) {
					fin >> idx1;
					fin >> idx2;
					fin >> idx3;

					faces.push_back(idx1);
					faces.push_back(idx2);
					faces.push_back(idx3);
				}
			}
			return;
		}
	}
}
