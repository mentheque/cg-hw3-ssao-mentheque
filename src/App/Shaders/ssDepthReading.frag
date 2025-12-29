#version 330 core

in vec2 tex_coord;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

layout (location = 0) out vec4 out_col;

void main() {
    out_col = vec4(texture(depthTex, tex_coord).r, 0.0, 0.0, 1.0);
}