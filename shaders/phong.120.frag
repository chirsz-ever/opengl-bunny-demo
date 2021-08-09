#version 120

varying vec3 v_normal;
varying vec3 v_viewcoord;
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
	// 归一化法向量
	vec3 N = normalize(v_normal);

	// 归一化的入射光线向量
	vec3 light_in[LIGHTS];
	for (int i = 0; i < LIGHTS; ++i) {
		light_in[i] = normalize(v_viewcoord - vec3(view * lights[i].position));
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
		float ratio = max(dot(N, -light_in[i]), 0.0);
		diffuse += material.diffuse * lights[i].diffuse * ratio;
	}
	diffuse.a = 1.0;

	// Specular，镜面反射光，强度与反射光线与观察方向的夹角的余弦成正相关
	specular = vec4(0.0);
	vec3 V = normalize(-v_viewcoord);     // 观察方向
	for (int i = 0; i < LIGHTS; ++i) {
		vec3 R = reflect(light_in[i], N);  // 反射光线方向
		float RdotV = max(dot(R, V), 0.0);
		specular += material.specular * lights[i].specular * pow(RdotV, material.shininess);
	}
	specular.a = 1.0;

	gl_FragColor = ambient + diffuse + specular;
}
