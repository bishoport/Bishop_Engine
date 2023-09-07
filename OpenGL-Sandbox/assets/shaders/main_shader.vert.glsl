#version 460 core
//#INCLUDE_COMMON

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in mat4 aInstanceTransform;

uniform DirLight dirLight;

out vec3 FragPos;
out vec2 TexCoords;

out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 viewMat;
uniform vec3 viewPos;
uniform mat4 projMat;
uniform mat4 modelMat;


void main(void)
{
	vec4 worldPos4 = modelMat * vec4(aPos, 1.);
	FragPos = worldPos4.xyz;
	TexCoords = vec2(aTexCoords.x, 1 - aTexCoords.y);

    mat3 normalMatrix = transpose(inverse(mat3(modelMat)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    TangentLightPos = TBN * dirLight.lightPos;
    TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * FragPos;

    gl_Position = projMat * viewMat * aInstanceTransform * worldPos4;
}
