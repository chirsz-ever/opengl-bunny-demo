#version 120

attribute vec3 position;
attribute vec4 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

varying vec4 v_color;

void main() {
    v_color = color;
    gl_Position =  proj * view * model * vec4(position, 1.0);
}
