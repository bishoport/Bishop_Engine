#version 330 core
layout(location = 0) out vec3 fragmentdepth;

void main() { 
	fragmentdepth = vec3(gl_FragCoord.z); 
}
