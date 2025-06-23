#version 120
uniform mat4 uMvpMatrix;
uniform vec4 uColor;
uniform vec4 uLowerFog;
uniform vec4 uUpperFog;

attribute vec3 aPosition;
attribute vec2 aTexCoord;
attribute vec4 aColor;

varying vec4 vColor;
varying vec2 vTexCoord;
varying float vShadow;
varying vec4 vFog;

void main(void)
{
    gl_Position = uMvpMatrix * vec4(aPosition, 1.0);

    float nearPlane = 0.4;
    float t = gl_Position.y / (gl_Position.z + nearPlane) * 0.5 + 0.5;
    vec4 fogColor = mix(uLowerFog, uUpperFog, t);
    float fog = clamp(0.05 * (-5.0 + gl_Position.z), 0.0, 1.0);
    vColor =  vec4(aColor.rgb, 0.5) * (2.0 * (1.0 - fog));
    vFog = fogColor * fog;

    vShadow = 1.0 - aColor.a;
    vTexCoord = aTexCoord;
}
