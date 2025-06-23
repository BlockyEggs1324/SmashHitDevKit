uniform mat4 uMvpMatrix;
uniform sampler2D uTexture0;
uniform vec4 uLowerFog;
uniform vec4 uUpperFog;
uniform bool uIsSelected;

varying vec4 vColor;
varying vec2 vTexCoord;
varying vec4 vFog;

void main(void) 
{
	vec4 red = vec4(1.0, 0.0, 0.0, 1.0); 

	if (uIsSelected) {
		gl_FragColor = red * vColor + vFog;
	} else {
		gl_FragColor = texture2D(uTexture0, vTexCoord) * (vColor / 4) + vFog;
	}
}
