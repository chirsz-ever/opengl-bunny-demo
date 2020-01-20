#version 120

varying vec3 N;                  // 归一化法向量
varying vec3 vertex_coord;       // 在观察坐标系下的顶点坐标

void main()
{
	N = normalize(gl_NormalMatrix * gl_Normal);

	vertex_coord = vec3(gl_ModelViewMatrix * gl_Vertex);

	// 使用默认投影
	gl_Position = ftransform();
}
