#version 120
uniform sampler2D uTexture0;

varying vec4 vColor;
varying vec2 vTexCoord;
varying float vShadow;
varying vec4 vFog;

void main(void) 
{
    float light = (1.0 - vShadow * vShadow);
    gl_FragColor = texture2D(uTexture0, vTexCoord) * vColor * light + vFog;
}
