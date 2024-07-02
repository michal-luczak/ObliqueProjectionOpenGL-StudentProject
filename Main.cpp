#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<vector>

#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Camera.h"

const unsigned int width = 1280;
const unsigned int height = 720;
const unsigned int stacks = 30;
const unsigned int sectors = 30;
float* radius = new float(0.1);
const float PI = 3.14159;

const unsigned short spheresCount = 2;
int vertexIndex = 0;
int index = 0;

void generateVerticesAndIndices(GLfloat* vertices, GLuint* indices) {
	const float sectorStep = 2 * PI / sectors;
	const float stackStep = PI / stacks;

	for (int i = 0; i <= stacks; ++i) {
		float stackAngle = PI / 2 - i * stackStep;
		float xy = *radius * cosf(stackAngle);
		float z = *radius * sinf(stackAngle);

		for (int j = 0; j <= sectors; ++j) {
			float sectorAngle = j * sectorStep;
			float x = xy * cosf(sectorAngle);
			float y = xy * sinf(sectorAngle);
			vertices[vertexIndex++] = x;
			vertices[vertexIndex++] = y;
			vertices[vertexIndex++] = z;
		}
	}

	for (int i = 0; i < stacks; ++i) {
		int k1 = i * (sectors + 1);
		int k2 = k1 + sectors + 1;

		for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
			if (i != 0) {
				indices[index++] = k1;
				indices[index++] = k2;
				indices[index++] = k1 + 1;
			}
			if (i != (stacks - 1)) {
				indices[index++] = k1 + 1;
				indices[index++] = k2;
				indices[index++] = k2 + 1;
			}
		}
	}
}

double lastFrame = 0.0;

double calculateFPS(GLFWwindow* window) {
	// Pobranie aktualnego czasu
	double currentFrame = glfwGetTime();
	// Obliczenie czasu od poprzedniej klatki
	double deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	return 1.0 / deltaTime;
}

struct State {
	float x, y, z;
};

float x(double time, float v0, float alfa) {
	return v0 * time * cos(alfa);
}

float z(double time, float v0, float alfa) {
	return v0 * time * cos(alfa);
}

float y(double time, float v0, float alfa, float g) {
	return v0 * time * sin(alfa) - g * time * time / 2;
}

State projectileEquations(double& time, float v0, float alfa, float dt, float g) {
	float k1x = x(time, v0, alfa);
	float k1y = y(time, v0, alfa, g);
	float k1z = z(time, v0, alfa);

	float k2x = x(time + dt / 2, v0, alfa) + k1x / 2;
	float k2y = y(time + dt / 2, v0, alfa, g) + k1y / 2;
	float k2z = z(time + dt / 2, v0, alfa) + k1z / 2;

	float k3x = x(time + dt / 2, v0, alfa) + k2x / 2;
	float k3y = y(time + dt / 2, v0, alfa, g) + k2y / 2;
	float k3z = z(time + dt / 2, v0, alfa) + k2z / 2;

	float k4x = x(time + dt, v0, alfa);
	float k4y = y(time + dt, v0, alfa, g);
	float k4z = z(time + dt, v0, alfa);

	State state;
	// Aktualizacja pozycji i pr�dko�ci
	state.x = dt / 6.0f * (k1x + 2.0f * k2x + 2.0f * k3x + k4x);
	state.y = dt / 6.0f * (k1y + 2.0f * k2y + 2.0f * k3y + k4y);
	//state.z = dt / 6.0f * (k1z + 2.0f * k2z + 2.0f * k3z + k4z);
	time += dt;
	return state;
}

int main() {
	const int numVertices = (sectors + 1) * (stacks + 1) * 3;
	const int numIndices = stacks * sectors * 6;
	// Tworzymy tablice na wierzcho�ki i indeksy
	GLfloat vertices[numVertices];
	GLuint indices[numIndices];
	generateVerticesAndIndices(vertices, indices);
	
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	double time[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

	GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL){
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);

	// Generates Shader object using shaders default.vert and default.frag
	Shader shaderProgram("default.vert", "default.frag");

	VAO VAO1;
	VAO1.Bind();
	// Generates Vertex Buffer Object and links it to vertices
	VBO VBO1(vertices, sizeof(vertices));
	// Generates Element Buffer Object and links it to indices
	EBO EBO1(indices, sizeof(indices));

	// Links VBO attributes such as coordinates and colors to VAO
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), (void*)0);
	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);

	// Creates camera object
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 12.0f));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	float color[4] = { 0.8f, 0.3f, 0.02f, 1.0f };
	float* fov = new float(90);
	// Main while loop
	while (!glfwWindowShouldClose(window)) {
		// Specify the color of the background
		glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		int* width = new int;
		int* height = new int;
		glfwGetWindowSize(window, width, height);
		glViewport(0, 0, *width, *height);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Tell OpenGL which Shader Program we want to use
		shaderProgram.Activate();

		// Handles camera inputs
		camera.Inputs(window);
		// Updates and exports the camera matrix to the Vertex Shader

		State state1 = projectileEquations(time[0], 120, 45, 0.005, 10);
		State state2 = projectileEquations(time[0], 150, 120, 0.005, 10);
		glm::vec3 translationA(state1.x, state1.y, state1.z);
		glm::vec3 translationB(state2.x + 1, state2.y, state2.z);
		camera.Matrix(*fov, 0.1f, 100.0f, shaderProgram, "camMatrix", translationA);
		VAO1.Bind();
		EBO1.Bind();
		glDrawElements(GL_TRIANGLE_STRIP, numIndices, GL_UNSIGNED_INT, 0);
		
		camera.Matrix(*fov, 0.1f, 100.0f, shaderProgram, "camMatrix", translationB);
		VAO1.Bind();
		EBO1.Bind();
		glDrawElements(GL_TRIANGLE_STRIP, numIndices, GL_UNSIGNED_INT, 0);

		float fps = calculateFPS(window);

		ImGui::Begin("My name is window, ImGUI window");
		ImGui::Text("FPS: ");
		ImGui::SameLine();
		ImGui::Text("%.0f", fps);

		if (ImGui::CollapsingHeader("Info")) {
			ImGui::Text("Window Dimensions:");
			ImGui::SameLine();
			ImGui::Text("%d x %d", *width, *height);

			std::string imgWindowPos = std::to_string(
				static_cast<int>(ImGui::GetWindowPos().x)) + 
				"/" + std::to_string(static_cast<int>(ImGui::GetWindowPos().y)
			);
			ImGui::Text("ImGui Window Position:");
			ImGui::SameLine();
			ImGui::Text("%s", imgWindowPos.c_str());
		}

		if (ImGui::CollapsingHeader("Timers")) {
			ImGui::Text("Frame Time:");
			ImGui::SameLine();
			ImGui::Text("%f", glfwGetTime() - lastFrame);
			ImGui::SameLine();
			ImGui::Text("ms");
		}

		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::Text("Camera Position:");
			ImGui::SameLine();
			ImGui::Text("x: %f, y: %f, z: %f", camera.Position.x, camera.Position.y, camera.Position.z);

			ImGui::Text("Field of View");
			ImGui::SameLine();
			ImGui::SliderFloat("##FOV", fov, 40, 120);
		}

		if (ImGui::CollapsingHeader("Spheres")) {
			ImGui::Text("Spheres:");
			ImGui::SameLine();
			ImGui::Text("%d", spheresCount);
			ImGui::ColorEdit4("Color", color);
		}

		ImGui::End();

		shaderProgram.Activate();
		glUniform1f(glGetUniformLocation(shaderProgram.ID, "size"), *radius);
		glUniform4f(glGetUniformLocation(shaderProgram.ID, "color"), color[0], color[1], color[2], color[3]);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Delete all the objects we've created
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();
	delete[] vertices;
	delete[] indices;
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}