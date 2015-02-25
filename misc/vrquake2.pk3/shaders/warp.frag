#version 110
uniform sampler2D texImage;
uniform sampler2D texDistort;
uniform vec4 rgbscale;
uniform int fogmodel;

varying vec4 color;
varying vec4 coords;

void main() {
	vec2 offset;
	vec4 dist;
	offset = texture2D(texDistort, coords.zw).zw * 0.5;
	dist = texture2D(texImage, coords.xy + offset);
	vec4 c = color * dist * rgbscale;

	if (fogmodel == 1) {
			float fogFactor = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
			fogFactor = clamp(fogFactor, 0.0, 1.0);
			c = mix(gl_Fog.color, c, fogFactor);
        } else if (fogmodel == 2) { 
                        float fogFactor = exp(-gl_Fog.density * gl_FogFragCoord);
                        fogFactor = clamp(fogFactor, 0.0, 1.0);
                        c = mix(gl_Fog.color, c, fogFactor);
	} else if (fogmodel == 3) {
                        float fogFactor = exp(-pow((gl_Fog.density * gl_FogFragCoord), 2.0));
                        fogFactor = clamp(fogFactor, 0.0, 1.0);
                        c = mix(gl_Fog.color, c, fogFactor);
	}
	gl_FragColor = c;
}
