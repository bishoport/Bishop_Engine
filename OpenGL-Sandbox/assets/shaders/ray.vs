#version 460 core

layout(location = 0) in vec3 inPosition;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

void main()
{
    gl_Position = u_ViewProjection * u_Model * vec4(inPosition, 1.0);
}
