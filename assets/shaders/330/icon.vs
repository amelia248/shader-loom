#version 330
precision highp float;
in vec3 vertexPosition;
in vec2 textureCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragmentPosition;
out vec2 fragmentCoord;

void main() {
	mat4 mvp = projection*view*model;

	gl_Position = mvp*vec4(vertexPosition,1);
	fragmentPosition = gl_Position.xyz;
	fragmentCoord = floor(textureCoord);
}