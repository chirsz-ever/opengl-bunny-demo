#version 150

in vec3 position;
in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 v_normal;    // 法向量
out vec3 v_viewcoord; // 在观察坐标系下的顶点坐标

void main() {
    v_normal = transpose(inverse(mat3(view * model))) * normal;

    vec4 view_position = view * model * vec4(position, 1.0);

    v_viewcoord = vec3(view_position);

    gl_Position = proj * view_position;
}
