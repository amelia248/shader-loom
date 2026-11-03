#version 330
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec4 vertexColour;
layout (location = 2) in vec2 textureCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragmentPosition;
out vec4 fragmentColour;
out vec2 fragmentCoord;

void main() {
	mat4 mvp = projection*view;

	gl_Position = mvp*vec4(vertexPosition,1);
	fragmentPosition = gl_Position.xyz;
	fragmentColour   = vertexColour;
	fragmentCoord    = textureCoord;
}