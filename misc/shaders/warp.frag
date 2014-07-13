#version 110
uniform sampler2D texImage;
uniform sampler2D texDistort;
uniform vec4 rgbscale;
varying vec4 color;
varying vec4 coords;

void main() {
	vec2 offset;
	vec4 dist;
	offset = texture2D(texDistort, coords.zw).zw * 0.5;
	dist = texture2D(texImage, coords.xy + offset);
	gl_FragColor = color * dist * rgbscale;
}