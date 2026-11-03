#version 330

precision highp float;

in vec3 fragmentPosition;
in vec4 fragmentColour;
in vec2 fragmentCoord;

uniform sampler2D uGlyphAtlas;
uniform sampler2D uBackgroundTexture;

out vec4 finalColour;

void main() {
	vec3 glyphValue = texture(uGlyphAtlas, fragmentCoord).rgb;
	vec4 background = texture(uBackgroundTexture, fragmentPosition.xy*.5+.5);

	if (glyphValue.r+glyphValue.g+glyphValue.b == 0) discard;
	
	finalColour = vec4(
		mix(background.rgb, fragmentColour.rgb, glyphValue),
		fragmentColour.a
	);

	// finalColour = fragmentColour; // vec4(1,0,1,1);
	// finalColour.rgb = vec3(fragmentCoord, 1);
}