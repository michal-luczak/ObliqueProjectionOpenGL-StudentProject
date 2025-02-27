#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;


// Inputs the color from the Vertex Shader
uniform vec4 color;


void main() {
	FragColor = color;
}