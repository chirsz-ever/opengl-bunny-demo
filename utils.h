#ifndef UTILS_H__
#define UTILS_H__

//#include <GL/gl.h>

#include <memory_resource>
#include <string_view>

#include <vector>

inline namespace glss {

template <typename coord = float, typename index = std::uint32_t>
struct Mesh {
    std::pmr::vector<coord> vertices;
    std::pmr::vector<index> indices;
    std::pmr::vector<coord> normals;
};

Mesh<> load_bunny_data(std::string_view obj_filename);

Mesh<> genSolidSphere(GLfloat radius, GLint slices, GLint stacks);

GLuint load_program(std::string_view vertex_shader_file, std::string_view fragment_shader_file);

} // namespace glss

#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif

#endif
