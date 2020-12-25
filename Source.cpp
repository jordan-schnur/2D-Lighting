#include "glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <glm\glm.hpp>
#include <string>
#include <vector>
#include <map>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //Hide console
#endif

struct Program {
	GLuint vao, vbo, ebo, program_id, vbo2, vao2;
	std::map <std::string, GLuint> uniforms;
	void setupProgram()
	{
		glBindVertexArray(vao);
		glUseProgram(program_id);
	}
};

//Forward declaration
void initProgram();
GLFWwindow* window;
Program createSceneProgram();
Program createScreenProgram();
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void CompileProgram(GLuint& program_id, const char* vertexshader_filename, const char* fragmentshader_filename);
glm::vec3 GetIntersection(glm::vec2 Ray1, glm::vec2 Ray2, glm::vec2 Point1, glm::vec2 Point2);
void scroll_callback(GLFWwindow* window, double x, double y);
bool sortVec3(const glm::vec4& i, const glm::vec4& j) { return (i.z < j.z); }
void CreateShader(GLuint& shader_object, const char* filename, GLenum shadertype);




//Variables

glm::vec3 BasicColor = glm::vec3(0.5, 0.5, 0.50);
glm::vec3 ShadowColor = glm::vec3(1, 1, 1);
bool drawLines = false;
float width = 640.0; float height = 360.0;
double lightSize = 200;
float boxVertices[] = { 0,  1,1, 1,1, 0,1, 0,0, 0,0,  1, };
GLfloat quadVertices[] = { -1.0f,  1.0f,  0.0f, 1.0f,1.0f,  1.0f,  1.0f, 1.0f,1.0f, -1.0f,  1.0f, 0.0f,1.0f, -1.0f,  1.0f, 0.0f,-1.0f, -1.0f,  0.0f, 0.0f,-1.0f,  1.0f,  0.0f, 1.0f };
static GLfloat verts[] = { 0, 0, width, 0,width, 0, width, height ,width, height, 0, height ,0, height, 0, 0,100,150, 120,50,120,50, 200,80,200,80, 140,210,140,210, 100,150,100,200, 120,250,120,250, 60,300,60,300, 100,200,200,260, 220,150,220,150, 300,200,300,200, 350,320,350,320, 200,260,340,60, 360,40,360,40, 370,70,370,70, 340,60,450,190, 560,170,560,170, 540,270,540,270, 430,290,430,290, 450,190,400,95, 580,50,580,50, 480,150,480,150, 400,95 };
//Points to cast the rays to
GLfloat points[] = { 0, 0, 640, 0, 640, 360, 0, 360, 100, 150, 120, 50, 200, 80, 140, 210, 100, 200, 120, 250, 60, 300, 200, 260, 220, 150, 300, 200, 350, 320, 340, 60, 360, 40, 370, 70, 450, 190, 560, 170, 540, 270, 430, 290, 400, 95, 580, 50, 480, 150, };
GLfloat lines[100 * 6];

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main() {

	initProgram();
	glm::mat4 ortho = glm::ortho(0.0f, 640.0f, 360.0f, 0.0f, -1.0f, 1.0f); //Setup orographic matrix

	Program sceneProgram = createSceneProgram();
	Program screenProgram = createScreenProgram();

	//Setup framebuffer for post processing
	GLuint frameBuffer;
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);


	GLuint texColorBuffer;//Create multisampled texture
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texColorBuffer);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 8, GL_RGB, width, height, GL_TRUE);

	//Connect texture to multisampled framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texColorBuffer, 0);



	GLuint intermediateBuffer; //Create normal framebuffer so we can render scene
	glGenFramebuffers(1, &intermediateBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateBuffer);

	GLuint screenTexture; //Create texture for screen
	glGenTextures(1, &screenTexture);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	//Attach to framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(screenProgram.program_id);
	glUniform1i(glGetUniformLocation(screenProgram.program_id, "renderedTexture"), 0);



	glm::mat4 view = glm::mat4(1.0f);


	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

	while (!glfwWindowShouldClose(window)) //Begin main game loop
	{
		glViewport(0, 0, width, height);

		double mX, mY;
		glfwGetCursorPos(window, &mX, &mY); //Load cursor position

		glm::vec4 mousePositon_worldspace = glm::vec4(mX, mY, 0, 1); //Transform screen coordinates to world
		glm::mat4 inverse = glm::inverse(ortho); //Basically voodoo
		mousePositon_worldspace = view * mousePositon_worldspace;
		glm::vec2 dir = glm::normalize(glm::vec2(mousePositon_worldspace.x, mousePositon_worldspace.y) - glm::vec2(640.0 / 2, 360.0 / 2));


		float pS = (sizeof(verts) / sizeof(GLfloat)) / 4; //Number of walls

		std::vector<glm::vec4> rays = std::vector<glm::vec4>();
		std::vector<glm::vec2> angles = std::vector<glm::vec2>();


		for (int j = 0; j < (sizeof(points) / sizeof(GLfloat)) / 2; j++) { //Load all of the angles into a vector
			float angle = atan2(points[(j * 2) + 1] - mY, points[j * 2] - mX);
			angles.push_back(glm::vec2(angle - 0.001f, 1));
			angles.push_back(glm::vec2(angle, 0));
			angles.push_back(glm::vec2(angle + 0.001f, 1));
		}

		for (int j = 0; j < angles.size(); j++) { //Cast rays to every point and find intersections
			float angle = angles[j].x;

			float dX = cos(angle);
			float dY = sin(angle);

			glm::vec3 closest = glm::vec3(0, 0, -1337);
			for (int i = 0; i < pS; i++) {
				glm::vec3 intersect = GetIntersection(
					glm::vec2(mX, mY),
					glm::vec2(mX + dX, mY + dY),
					glm::vec2(verts[i * 4], verts[(i * 4) + 1]),
					glm::vec2(verts[(i * 4) + 2], verts[(i * 4) + 3]));

				if (intersect.z < 0) continue;
				if (closest.z < 0 || intersect.z < closest.z) {
					closest = intersect;
				}
			}
			closest.z = angle;
			rays.push_back(glm::vec4(closest, j));
		}

		std::vector<float> pointsV2 = std::vector<float>();
		std::sort(rays.begin(), rays.end(), sortVec3); //Sort the rays based on angle(Clockwise)
		int numTimes = 0;
		lines[0] = mX;
		lines[1] = mY;
		for (int i = 0; i < rays.size(); i++) {
			glm::vec4 ray = rays[i];
			int offset = 4 * numTimes; //Change offset based on if we're drawing lines or triangles
			offset += 2;

			glm::vec4 pRay;
			if (i == rays.size() - 1) {
				pRay = rays[i];
			}
			else {
				pRay = rays[i + 1];
			}


			if (i == rays.size() - 1) {
				pRay = rays[0];
			}


			if (drawLines) {
				lines[(offset + 0)] = ray.x;
				lines[(offset + 1)] = ray.y;
				lines[offset + 2] = mX;
				lines[offset + 3] = mY;
				pointsV2.push_back(ray.x);
				pointsV2.push_back(ray.y);
				numTimes++;
			}
			else { //Create triangles from the point
				lines[(offset + 0)] = ray.x;
				lines[(offset + 1)] = ray.y;
				lines[(offset + 2)] = pRay.x;
				lines[(offset + 3)] = pRay.y;
				numTimes++;
			}


		}

		angles.clear();
		glBindBuffer(GL_ARRAY_BUFFER, sceneProgram.vbo2);
		glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STREAM_DRAW); //Load the vertcies into the GPU
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		rays.clear();



		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear framebuffer


		//Set shader uniforms
		glUseProgram(sceneProgram.program_id);
		glBindVertexArray(sceneProgram.vao2);
		glUniform2f(sceneProgram.uniforms.at("mouseCoords"), mousePositon_worldspace.x, mousePositon_worldspace.y);
		glUniform3f(sceneProgram.uniforms.at("Color"), ShadowColor.r, ShadowColor.g, ShadowColor.b);
		glUniform1f(glGetUniformLocation(sceneProgram.program_id, "lightSize"), lightSize);


		if (mX > 0 && mY > 0 && mX < width && mY < height) {
			if (!drawLines) { //Draw shadow
				glUniform1i(glGetUniformLocation(sceneProgram.program_id, "isShadow"), 1);
				glDrawArrays(GL_TRIANGLE_FAN, 0, (numTimes * 2) + 1);
				glUniform1i(glGetUniformLocation(sceneProgram.program_id, "isShadow"), 0);
			}
			else { //Draw the lines
				glDrawArrays(GL_LINES, 0, numTimes * 2);
			}

		}




		//Write framebuffer into the intermediateBuffer for rendering. Bind default buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateBuffer);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);



		//Send uniforms for screen
		glBindVertexArray(screenProgram.vao);
		glUseProgram(screenProgram.program_id);
		glUniform2f(screenProgram.uniforms.at("mouseCoords"), mousePositon_worldspace.x, mousePositon_worldspace.y);
		glUniformMatrix4fv(glGetUniformLocation(screenProgram.program_id, "inverseMat"), 1, GL_FALSE, glm::value_ptr(inverse));
		if (pointsV2.size() != 0) {
			glUniform1i(glGetUniformLocation(screenProgram.program_id, "sizeofpoints"), pointsV2.size() / 2);
			glUniform2fv(glGetUniformLocation(screenProgram.program_id, "points"), pointsV2.size(), &pointsV2[0]);
		}


		if (drawLines) {
			glUniform1i(glGetUniformLocation(screenProgram.program_id, "showPoints"), 1);
		}
		else {
			glUniform1i(glGetUniformLocation(screenProgram.program_id, "showPoints"), 0);
		}
		//Send the texture from the framebuffer
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, screenTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6); //Draw the screen

		//Finish my drawing the walls
		glUseProgram(sceneProgram.program_id);
		glm::mat4 MVO = ortho * view * glm::mat4(1.0f);
		glUseProgram(sceneProgram.program_id);
		glUniformMatrix4fv(glGetUniformLocation(sceneProgram.program_id, "MVO"), 1, GL_FALSE, glm::value_ptr(MVO));
		glBindVertexArray(sceneProgram.vao);
		glUniform3f(sceneProgram.uniforms.at("Color"), BasicColor.r, BasicColor.g, BasicColor.b);
		glDrawArrays(GL_LINES, 8, (sizeof(verts) / sizeof(GLfloat)) - 36);


		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			break;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteFramebuffers(1, &frameBuffer);
	glDeleteFramebuffers(1, &intermediateBuffer);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}



void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		drawLines = !drawLines;
		if (drawLines) {
			glClearColor(1.0, 1.0, 1.0, 1.0);
			ShadowColor = glm::vec3(1.0, 0.0, 0.0);
		}
		else {
			glClearColor(0.0, 0.0, 0.0, 1.0);
			ShadowColor = glm::vec3(1.0, 1.0, 1.0);
		}
	}
}

void scroll_callback(GLFWwindow* window, double x, double y) {
	lightSize += y * 10;
}

glm::vec3 GetIntersection(glm::vec2 Ray1, glm::vec2 Ray2, glm::vec2 Point1, glm::vec2 Point2) { //Do even more voodoo for rays! Sacrificing a chicken would have worked, but I didn't want to do that.
	float rayPX = Ray1.x;
	float rayPY = Ray1.y;
	float rayDX = Ray2.x - Ray1.x;
	float rayDY = Ray2.y - Ray1.y;

	float linePX = Point1.x;
	float linePY = Point1.y;
	float lineDX = Point2.x - Point1.x;
	float lineDY = Point2.y - Point1.y;

	float r_mag = sqrt(rayDX * rayDX + rayDY * rayDY);
	float s_mag = sqrt(lineDX * lineDX + lineDY * lineDY);
	if (rayDX / r_mag == lineDX / s_mag && rayDY / r_mag == lineDY / s_mag) {
		// Unit vectors are the same.
		return glm::vec3(0, 0, -1337);
	}

	float T2 = (rayDX * (linePY - rayPY) + rayDY * (rayPX - linePX)) / (lineDX * rayDY - lineDY * rayDX);

	float T1;

	if (rayDX != 0) {
		T1 = (linePX + lineDX * T2 - rayPX) / rayDX;
	}
	else {
		T1 = (linePY + lineDY * T2 - rayPY) / rayDY;
	}
	if (T1 < 0) return glm::vec3(0, 0, -1337);
	if (T2 < 0 || T2>1) return glm::vec3(0, 0, -1337);

	return glm::vec3(
		rayPX + rayDX * T1,
		rayPY + rayDY * T1,
		T1);


}

Program createSceneProgram() {
	Program program;
	CompileProgram(program.program_id, "main.vert", "main.frag");
	glGenVertexArrays(1, &program.vao);
	glGenVertexArrays(1, &program.vao2);

	glGenBuffers(1, &program.vbo);
	glGenBuffers(1, &program.vbo2);

	glBindBuffer(GL_ARRAY_BUFFER, program.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);



	glUseProgram(program.program_id);
	program.uniforms.insert(std::pair<std::string, GLuint>("MVO", glGetUniformLocation(program.program_id, "MVO")));
	program.uniforms.insert(std::pair<std::string, GLuint>("Color", glGetUniformLocation(program.program_id, "Color")));
	program.uniforms.insert(std::pair<std::string, GLuint>("mouseCoords", glGetUniformLocation(program.program_id, "mouseCoords")));
	glUniform2f(program.uniforms.at("mouseCoords"), 0.0, 0.0);

	glBindVertexArray(program.vao);
	glBindBuffer(GL_ARRAY_BUFFER, program.vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	glBindVertexArray(program.vao2);
	glBindBuffer(GL_ARRAY_BUFFER, program.vbo2);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glBindVertexArray(0);


	return program;
}



Program createScreenProgram() {
	Program program;
	CompileProgram(program.program_id, "post.vert", "post.frag");
	glGenVertexArrays(1, &program.vao);
	glBindVertexArray(program.vao);
	glGenBuffers(1, &program.vbo);

	glBindBuffer(GL_ARRAY_BUFFER, program.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);


	glBindVertexArray(program.vao);
	glBindBuffer(GL_ARRAY_BUFFER, program.vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

	glUseProgram(program.program_id);
	program.uniforms.insert(std::pair<std::string, GLuint>("mouseCoords", glGetUniformLocation(program.program_id, "mouseCoords")));
	glUniform2f(program.uniforms.at("mouseCoords"), 0.0, 0.0);
	glBindVertexArray(0);
	return program;
}

std::string LoadFile(const char* filename) {
	//TODO: Make more efficient.
	std::string text = "";
	std::string line = "";
	std::ifstream f(filename);
	if (f.is_open())
	{

		while (std::getline(f, line))
		{
			text += line + "\n";
		}
		f.close();
		return text;
	}
	else std::cout << "Failed to load file: " << filename << std::endl; return "";
}


void initProgram() {

	GLuint vertex_buffer, vertex_shader, fragment_shader, program;
	GLint mvp_location, vpos_location, vcol_location;

	//glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //Request a specific OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //Request a specific OpenGL version
	glfwWindowHint(GLFW_SAMPLES, 16); //Request 4x antialiasing


	window = glfwCreateWindow(640, 360, "2D Lighting", NULL, NULL);


	if (!window) {
		fprintf(stderr, "Failed to open GLFW window.\n");
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glEnable(GL_MULTISAMPLE);
}


void CompileProgram(GLuint& program_id, const char* vertexshader_filename, const char* fragmentshader_filename) {
	GLuint vertexshader_object, fragmentshader_object;

	CreateShader(vertexshader_object, vertexshader_filename, GL_VERTEX_SHADER);
	CreateShader(fragmentshader_object, fragmentshader_filename, GL_FRAGMENT_SHADER);

	program_id = glCreateProgram();

	glAttachShader(program_id, vertexshader_object);
	glAttachShader(program_id, fragmentshader_object);

	glLinkProgram(program_id);
	glUseProgram(program_id);

	glDeleteShader(vertexshader_object);
	glDeleteShader(fragmentshader_object);
}


void CreateShader(GLuint& shader_object, const char* filename, GLenum shadertype) {
	shader_object = glCreateShader(shadertype);
	std::string fileText = LoadFile(filename);
	std::string shaderVersion = "#version 330";
	std::string s = shaderVersion + "\n" + fileText;
	char const* source = s.c_str();
	glShaderSource(shader_object, 1, &source, NULL);
	glCompileShader(shader_object);


	GLint status;
	glGetShaderiv(shader_object, GL_COMPILE_STATUS, &status);
	if (!status) {
		int InfoLogLength;
		glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &InfoLogLength);
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
		glGetShaderInfoLog(shader_object, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		fprintf(stdout, "Error %s: %s\n", filename, &FragmentShaderErrorMessage[0]);
		return;
	}
}