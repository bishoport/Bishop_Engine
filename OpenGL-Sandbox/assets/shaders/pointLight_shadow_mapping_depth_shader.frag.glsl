#version 330 core
in vec4 FragPos;

uniform int numPointLights;
uniform vec3 lightPos[16];
uniform float far_plane[16];

void main()
{
    for (int i = 0; i < numPointLights; ++i)
    {
        float lightDistance = length(FragPos.xyz - lightPos[i]);

        // map to [0;1] range by dividing by far_plane
        lightDistance = lightDistance / far_plane[i];

        // write this as modified depth
        gl_FragDepth = lightDistance;
    }
}