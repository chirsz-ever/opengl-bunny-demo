#version 120

varying vec3 N;
varying vec3 vertex_coord;
const int LIGHTS = 2;

void main()
{
	gl_MaterialParameters material = gl_FrontMaterial;

	// 归一化的入射光线向量
	vec3 light_in[LIGHTS];
	for (int i = 0; i < LIGHTS; ++i) {
		light_in[i] = normalize(vertex_coord - gl_LightSource[i].position.xyz);
	}

	vec4 ambient, diffuse, specular;

	// Ambient，环境光
	ambient = gl_LightModel.ambient * material.ambient;
	for (int i = 0; i < LIGHTS; ++i) {
		ambient += gl_LightSource[i].ambient * material.ambient;
	}

	// Diffuse，漫反射光，强度与入射角余弦成正比
	diffuse = vec4(0.0);
	for (int i = 0; i < LIGHTS; ++i) {
		float ratio = max(dot(N, -light_in[i]), 0.0);
		diffuse += material.diffuse * gl_LightSource[i].diffuse * ratio;
	}
	diffuse.a = 1.0;

	// Specular，镜面反射光，强度与反射光线与观察方向的夹角的余弦成正相关
	specular = vec4(0.0);
	vec3 V = normalize(-vertex_coord);     // 观察方向
	for (int i = 0; i < LIGHTS; ++i) {
		vec3 R = reflect(light_in[i], N);  // 反射光线方向
		float RdotV = dot(R, V);
		if (RdotV > 0) {
			specular += material.specular * gl_LightSource[i].specular * pow(RdotV, material.shininess);
		}
	}
	specular.a = 1.0;

	gl_FragColor = ambient + diffuse + specular;
}
