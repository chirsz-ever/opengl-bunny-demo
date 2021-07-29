#version 120

attribute vec3 position;
attribute vec3 normal;

varying vec3 N;                  // 归一化法向量
varying vec3 vertex_coord;       // 在观察坐标系下的顶点坐标

#if __VERSION__ < 150
mat3 inverse(mat3 m) {
    float Determinant =
          m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
        - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
        + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
    
    mat3 Inverse;
    Inverse[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
    Inverse[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]);
    Inverse[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
    Inverse[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]);
    Inverse[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
    Inverse[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]);
    Inverse[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
    Inverse[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]);
    Inverse[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
    Inverse /= Determinant;
    
    return Inverse;
}
#endif

void main()
{
	N = transpose(inverse(mat3(gl_ModelViewMatrix))) * normal;

	vec4 view_position = gl_ModelViewMatrix * vec4(position, 1.0);

	vertex_coord = vec3(view_position);

	gl_Position =  gl_ProjectionMatrix * view_position;
}
