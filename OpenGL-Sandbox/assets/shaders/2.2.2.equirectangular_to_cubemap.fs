#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularDayLightMap;
uniform sampler2D equirectangularNightLightMap;

uniform float mixFactor; // Factor de mezcla

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 colorDay = texture(equirectangularDayLightMap, uv).rgb;
    vec3 colorNight = texture(equirectangularNightLightMap, uv).rgb;
    
    vec3 color = mix(colorNight, colorDay, mixFactor); // Mezcla de los colores basándose en mixFactor

    FragColor = vec4(color, 1.0);
}
