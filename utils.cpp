#include <GL/glew.h>

#include "utils.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

using namespace std;

namespace glss {

static void eatline(std::istream &input) {
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

Mesh<> load_bunny_data(std::string_view obj_filename) {
    ifstream fin;
    fin.open(filesystem::path(obj_filename));
    if (!fin.is_open()) {
        cerr << "Open `" << obj_filename << "` failed" << endl;
        exit(1);
    }

    Mesh<> mesh;
    auto &vertices = mesh.vertices;
    auto &faces    = mesh.indices;
    auto &normals  = mesh.normals;

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
            fin >> i1;
            fin.get();
            fin.get();
            fin >> i1;
            fin >> i2;
            fin.get();
            fin.get();
            fin >> i2;
            fin >> i3;
            fin.get();
            fin.get();
            fin >> i3;
            faces.push_back(i1 - 1);
            faces.push_back(i2 - 1);
            faces.push_back(i3 - 1);
            eatline(fin);
        }
    }

    return mesh;
}

template <typename T, typename A>
static void push3(std::vector<T, A> &vertices, T x, T y, T z) {
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
}

Mesh<> genSolidSphere(GLfloat radius, GLint slices, GLint stacks) {
    GLfloat step_la = M_PI / stacks;
    GLfloat step_lo = 2.0f * M_PI / slices;

    std::pmr::vector<GLfloat> vertices, normals;
    vertices.reserve(slices * (stacks - 1) * 3 + 6);
    normals.reserve(slices * (stacks - 1) * 3 + 6);

    GLfloat latitude = M_PI / 2.0f;
    for (int i = 1; i < stacks; ++i) {
        latitude -= step_la;
        GLfloat longtitude = 0.0f;
        for (int j = 0; j < slices; ++j, longtitude += step_lo) {
            GLfloat x = cos(latitude) * cos(longtitude);
            GLfloat y = cos(latitude) * sin(longtitude);
            GLfloat z = sin(latitude);
            push3(normals, x, y, z);
            push3(vertices, radius * x, radius * y, radius * z);
        }
    }

    // North Pole
    push3(vertices, 0.0f, 0.0f, radius);
    push3(normals, 0.0f, 0.0f, 1.0f);

    // South Pole
    push3(vertices, 0.0f, 0.0f, -radius);
    push3(normals, 0.0f, 0.0f, -1.0f);

    std::pmr::vector<GLuint> indices;
    indices.reserve(slices * stacks * 3);

    for (int i = 0; i < stacks - 2; ++i) {
        latitude -= step_la;
        for (int j = 0; j < slices - 1; ++j) {
            /*
             *      v1 -- v2
             *      |     |
             *      v3 -- v4
             */
            GLuint v1 = i * slices + j;
            GLuint v2 = v1 + 1;
            GLuint v3 = (i + 1) * slices + j;
            GLuint v4 = v3 + 1;
            push3(indices, v1, v3, v4);
            push3(indices, v1, v4, v2);
        }

        // the triangles on the tail and head
        GLuint v1 = i * slices + slices - 1;
        GLuint v2 = i * slices;
        GLuint v3 = (i + 1) * slices + slices - 1;
        GLuint v4 = (i + 1) * slices;
        push3(indices, v1, v3, v4);
        push3(indices, v1, v4, v2);
    }

    // North Pole
    GLuint vnorth = vertices.size() / 3 - 2;
    for (int j = 0; j < slices - 1; ++j) {
        push3<GLuint>(indices, vnorth, j, j + 1);
    }
    push3<GLuint>(indices, vnorth, slices - 1, 0);

    // South Pole
    GLuint vsouth = vertices.size() / 3 - 1;
    for (int j = slices * (stacks - 2); j < slices * (stacks - 1) - 1; ++j) {
        push3<GLuint>(indices, vsouth, j + 1, j);
    }
    push3<GLuint>(indices, vsouth, slices * (stacks - 2), slices * (stacks - 1) - 1);

    return {vertices, indices, normals};
}

static GLuint load_shader(std::string_view shader_file, GLenum shader_type) {
    ifstream fin;
    fin.open(std::filesystem::path(shader_file));
    GLint file_len;
    GLchar *source;

    fin.seekg(0, ios_base::end);
    file_len = fin.tellg();
    source   = new GLchar[file_len];
    fin.seekg(0, ios_base::beg);
    fin.read(source, file_len);

    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, &file_len);
    delete[] source;
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << shader_file << " failed to compile:" << std::endl;
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
        char *logMsg = new char[logSize];
        glGetShaderInfoLog(shader, logSize, NULL, logMsg);
        std::cerr << logMsg << std::endl;
        delete[] logMsg;
        exit(EXIT_FAILURE);
    }

    return shader;
}

GLuint load_program(std::string_view vertex_shader_file, std::string_view fragment_shader_file) {
    GLuint vertex_shader   = load_shader(vertex_shader_file, GL_VERTEX_SHADER);
    GLuint fragment_shader = load_shader(fragment_shader_file, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        std::cerr << "Shader program failed to link" << std::endl;
        GLint logSize;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
        char *logMsg = new char[logSize];
        glGetProgramInfoLog(program, logSize, NULL, logMsg);
        std::cerr << logMsg << std::endl;
        delete[] logMsg;
        exit(EXIT_FAILURE);
    }

    return program;
}

}; // namespace glss
