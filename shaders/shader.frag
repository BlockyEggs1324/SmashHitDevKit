#version 330 core

in vec3 vColor;
in vec2 vTexCoord;

uniform sampler2D uTexture0;  // Renamed to match C++ code
uniform bool isSelected;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture0, vTexCoord);
    
    if (isSelected) {
        texColor.rgb = mix(texColor.rgb, vec3(1.0, 1.0, 1.0), 0.5);
    }

    FragColor = texColor * vec4(vColor, 1.0);
}