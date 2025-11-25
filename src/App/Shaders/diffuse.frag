#version 330 core

in vec3 vert_col;

out vec4 out_col;

void main(){
	out_col = vec4(vert_col.rgb, 1.0);
}