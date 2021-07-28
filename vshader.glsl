#version 120

attribute vec3 position;
attribute vec3 normal;

varying vec3 N;                  // 归一化法向量
varying vec3 vertex_coord;       // 在观察坐标系下的顶点坐标

void main()
{
	N = gl_NormalMatrix * normal;

	vec4 view_position = gl_ModelViewMatrix * vec4(position, 1.0);

	vertex_coord = vec3(view_position);

	gl_Position =  gl_ProjectionMatrix * view_position;
}
