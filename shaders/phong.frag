#version 120

varying vec3 N;
varying vec3 vertex_coord;
const int LIGHTS = 2;

struct Material {
	//vec4 emission;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct LightSource {
	vec4  ambient;
	vec4  diffuse;
	vec4  specular;
	vec4  position;
	// vec4  halfVector;
	// vec3  spotDirection;
	// float spotExponent;
	// float spotCutoff;
	// float spotCosCutoff;
	// float constantAttenuation; // K0
	// float linearAttenuation;   // K1
	// float quadraticAttenuation;// K2
};

uniform Material material;
uniform LightSource lights[LIGHTS];
uniform vec4 global_ambient;
uniform mat4 view;

void main()
{
	// 归一化的入射光线向量
	vec3 light_in[LIGHTS];
	for (int i = 0; i < LIGHTS; ++i) {
		light_in[i] = normalize(vertex_coord - vec3(view * lights[i].position));
	}

	vec4 ambient, diffuse, specular;

	// Ambient，环境光
	ambient = global_ambient * material.ambient;
	for (int i = 0; i < LIGHTS; ++i) {
		ambient += lights[i].ambient * material.ambient;
	}

	// Diffuse，漫反射光，强度与入射角余弦成正比
	diffuse = vec4(0.0);
	for (int i = 0; i < LIGHTS; ++i) {
		float ratio = max(dot(normalize(N), -light_in[i]), 0.0);
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
			specular += material.specular * lights[i].specular * pow(RdotV, material.shininess);
		}
	}
	specular.a = 1.0;

	gl_FragColor = ambient + diffuse + specular;
}
