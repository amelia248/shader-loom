#version 330

in vec3 fragmentPosition;
in vec2 fragmentCoord;

uniform sampler2D uTexture;
uniform vec4      uColour;
uniform vec4      uBorderColour;
uniform vec4      uBurnColour;


uniform vec2 uSize; // width, height

//                            0---1
uniform vec4 uCurvature; //   |   |
//                            2---3

//                            ┌-0-┐
uniform vec4 uBorderWidth; // 1   2
//                            └-3-┘

//                            ┌--------┐
//                            | xy--┐  |
uniform vec4 uUVRectangle; // |  |  |  |
//                            |  └--wh |
//                            └--------┘

out vec4 finalColour;

float func(vec2 p, float r, vec2 b) {
	if (r <= 0.0) return max(p.x - b.x, p.y - b.y);

	b = min(b, vec2(r - 0.001));

	return length(p*r / (r-b)) - r;
}
#define SMOOTHING_RADIUS .75
void main() {
	vec2 uv = (fragmentCoord-.5) * uSize;
	
	vec4 uvRec = uUVRectangle;
	uvRec.xy /= textureSize(uTexture, 0);
	uvRec.zw /= textureSize(uTexture, 0);
	vec2 coord = fragmentCoord * uvRec.zw + uvRec.xy;
	vec4 backgroundColour = uColour * texture(uTexture, coord);


	float radius;
	vec2  border;
	
	vec4 r = uCurvature;
	r.xz   = (uv.x>=0.0)?r.yw : r.xz;
	radius = (uv.y<=0.0)?r.x  : r.z;
	
	border.x = uv.x<=0.0 ? uBorderWidth.y : uBorderWidth.z;
	border.y = uv.y>=0.0 ? uBorderWidth.w : uBorderWidth.x;

	uv = abs(uv) - uSize*.5 + radius;
	float sdf = length(uv) - radius;
	if (uv.x > 0.0 && uv.y > 0.0) {
		finalColour = mix(
			vec4(uBorderColour.rgb, 0), uBorderColour,
			smoothstep(-SMOOTHING_RADIUS, SMOOTHING_RADIUS, -sdf) //clamp(-sdf, 0.0, 1.0)
		);
		if (radius > 0.0 && border.x < radius && border.y < radius) {
			vec2 b = min(border, vec2(radius - 0.001));
			sdf = length(uv*radius / (radius-b)) - radius;
			finalColour = mix(
				finalColour, backgroundColour,
				smoothstep(-SMOOTHING_RADIUS, SMOOTHING_RADIUS, -sdf) //clamp(-sdf, 0.0, 1.0)
			);
		}
	}
	else if (uv.x > radius-border.x || uv.y > radius-border.y) {
		finalColour = vec4(1);
		finalColour = uBorderColour;
	}
	else {
		finalColour = vec4(uv/radius, 0, 1);
		finalColour = backgroundColour;
	}

	finalColour.rgb = mix(
		finalColour.rgb,
		1.0 - ((1.0 - finalColour.rgb) / uBurnColour.rgb),
		uBurnColour.a
	);
}