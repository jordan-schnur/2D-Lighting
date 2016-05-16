layout(location = 0) in vec2 position_screenspace;
layout(location = 1) in vec2 tex_screenspace;


uniform mat4 inverseMat;
out vec2 Texcoord;
out vec2 position;
void main() {
	
	Texcoord = tex_screenspace;
	position = (inverseMat * vec4(position_screenspace, 0.0, 1.0)).xy; //Screenspace to world space
    gl_Position = vec4(position_screenspace, 0.0, 1.0);

}