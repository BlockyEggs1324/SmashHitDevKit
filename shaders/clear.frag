uniform mat4 uMvpMatrix;
uniform vec4 uLowerFog;
uniform vec4 uUpperFog;

varying vec4 vColor;

void main(void) 
{
	gl_FragColor = vColor;
}

