#version 330 core

in vec2 tex_coord;

uniform sampler2D colorTex;

out vec4 out_col;

void main() {
    out_col = vec4(texture(colorTex, tex_coord).rgb, 1.0);
}