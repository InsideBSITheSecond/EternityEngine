#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragUv;

struct PointLight {
	vec4 position; //ignote w
	vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
	vec4 ambientLightColor;
	vec3 directionalLight;
	PointLight pointLights[10];
	int numLights;
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix; // projection * view * model
	mat4 normalMatrix;
} push;

const float AMBIENT = 0.05;
const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, -3.0, -1.0)); //sun

// MATRIX MULTIPLICATION ORDER MATTERS
void main() {
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * positionWorld;
	fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
	fragPosWorld = positionWorld.xyz;
	float lightIntensity = max(dot(fragNormalWorld, normalize(ubo.directionalLight)), 0);
	
	fragColor = lightIntensity * color;
	fragUv = uv;
}