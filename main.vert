
layout(location = 0) in vec2 vertices_worldspace;

uniform mat4 MVO;
out vec2 position;

void main() {
	position = vertices_worldspace;
	gl_Position = MVO * vec4(vertices_worldspace,0.0,1.0);
}