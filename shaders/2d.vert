uniform mat4 uMvpMatrix;
uniform vec4 uColor;

attribute vec3 aPosition;

void main(void)
{
	gl_Position = uMvpMatrix * vec4(aPosition.xyz, 1.0);
}