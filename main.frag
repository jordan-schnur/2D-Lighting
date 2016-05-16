
out vec4 outColor;
uniform vec3 Color;
uniform  vec2 mouseCoords;
uniform bool isShadow;
uniform float lightSize;

in vec2 position;

void main() {
	
	
	outColor = vec4(Color,1.0);
	if(isShadow) {
			float dis= distance(mouseCoords,position);
		if(dis<lightSize) {
			outColor = mix(vec4(Color, 1.0),vec4(0.05,0.05, 0.05, 1.0),dis/lightSize);
		} else  {
			outColor =vec4(0.05,0.05, 0.05, 1.0);
		}
	}
}