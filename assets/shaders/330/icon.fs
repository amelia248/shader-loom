#version 330

precision highp float;

in vec3 fragmentPosition;
in vec2 fragmentCoord;

uniform vec4 uColour;
uniform float uTime;
uniform sampler2D uTexture;
uniform vec2 uIcon;

out vec4 finalColour;

void main() {
	vec2 uv = fragmentCoord;
	uv /= 16.0;
	uv += uIcon / 16.0;
	finalColour = texture(uTexture, uv) * uColour;
}