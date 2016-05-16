in vec2 Texcoord;
out vec4 outColor;

uniform sampler2D renderedTexture;
uniform  vec2 mouseCoords;
uniform vec2 points[100];
uniform int sizeofpoints;
uniform bool showPoints;

in vec2 position;


void main() {
	
	outColor = texture(renderedTexture, Texcoord); //Render out texture
	
	if(showPoints) { //If we want to show the points
		for (int i = 0; i < sizeofpoints; i++) {
			float dis2 = distance(points[i],position);
			if(dis2<4) {
				outColor = mix(vec4(1,0, 0, 1.0),texture(renderedTexture, Texcoord) ,dis2/4); //Show point
			}
		}
	}
} 