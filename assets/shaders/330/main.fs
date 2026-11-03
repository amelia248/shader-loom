#version 330

in vec3 fragmentPosition;
in vec2 fragmentCoord;

uniform sampler2D uTexture;
uniform vec4      uColour;

out vec4 finalColour;

void main() {
	finalColour = texture(uTexture, fragmentCoord) * uColour;
}