#ifndef UTILS_H__
#define UTILS_H__

//#include <GL/gl.h>

#include <vector>

// 2021/08: Apple clang does not support <memory_source>, WTF!
#if defined(__APPLE__) && defined(__clang__)
namespace glss {
    template<typename T>
    using Vector = std::vector<T>;
};
#else
#include <memory_resource>
namespace glss {
    template<typename T>
    using Vector = std::pmr::vector<T>;
};
#endif // __APPLE__

#include <string_view>

namespace glss {

template <typename coord = float, typename index = std::uint32_t>
struct Mesh {
    Vector<coord> vertices;
    Vector<index> indices;
    Vector<coord> normals;
};

Mesh<> load_bunny_data(std::string_view obj_filename);

Mesh<> genSolidSphere(GLfloat radius, GLint slices, GLint stacks);

GLuint load_program(std::string_view vertex_shader_file, std::string_view fragment_shader_file);

} // namespace glss

using glss::Mesh;
using glss::load_bunny_data;
using glss::genSolidSphere;
using glss::load_program;

#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif

#endif
