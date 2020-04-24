#include "common.h"
#include "fluid.h"
#include "simulator.h"
#include <iostream>
#include <chrono>
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* WINDOW_TITLE = "Fluid Simulator";
const double FRAME_RATE_MS = 1000.0 / 60.0;

typedef glm::vec4 color4;
typedef glm::vec4 point4;
typedef glm::vec3 vec3;

const int window_size = 512;
const int N = SIZE;
const int NumVertices = N * N * 4;
const int NumIndices = N * N * 6;
point4 vertices[NumVertices];
const int FillIndices = (2 + N * 2) * (N - 1);
const int EdgeIndices = N * N * 2;
GLuint indices[NumIndices];
int prev_mouseX = 0;
int prev_mouseY = 0;
bool isLeftClicked = false;

void fade();
void renderFluid();

FluidCell* activeCell;
FluidSimulator* activeSimulator;

//----------------------------------------------------------------------------

// OpenGL initialization
void init() {
	//create a new fluid cell
	activeCell = new FluidCell(0.2f, 0.01f, 0.000005f);
	activeSimulator = new FluidSimulator(activeCell, 16);

	// create the height field vertices (a 2D grid in the x-z plane)
	int Index = 0;
	int FaceIndex = 0;
	float dx = 2.0f / N, dy = 2.0f / N;

	for (int i = 0; i < N; i++) {
		float y = i * dy - 1.0f;
		for (int j = 0; j < N; j++) {
			float x = j * dx - 1.0f;
			vertices[Index++] = point4(x, y, 1.0, 1.0); 
			vertices[Index++] = point4(x + dx, y, 1.0, 1.0); 
			vertices[Index++] = point4(x + dx, y + dy, 1.0, 1.0); 
			vertices[Index++] = point4(x, y + dy, 1.0, 1.0); 
			indices[FaceIndex++] = Index - 4;
			indices[FaceIndex++] = Index - 3;
			indices[FaceIndex++] = Index - 1;
			indices[FaceIndex++] = Index - 3;
			indices[FaceIndex++] = Index - 2;
			indices[FaceIndex++] = Index - 1;
		}
	}
	assert(Index == NumVertices);
	assert(FaceIndex == NumIndices);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// Index buffer
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Load shaders and use the resulting shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// set up vertex arrays
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_FLAT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	activeSimulator->step();
	renderFluid();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glDrawElements(GL_TRIANGLES, NumVertices * 3 / 2, GL_UNSIGNED_INT, 0);
	glutSwapBuffers();
}

//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y) {
	if (state == GLUT_DOWN) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			prev_mouseX = x;
			prev_mouseY = y;
			isLeftClicked = true;
			break;
		}
	}
	if (state == GLUT_UP)
	{
		switch (button) {
		case GLUT_LEFT_BUTTON:
			isLeftClicked = false;
			break;
		}
	}
}

//----------------------------------------------------------------------------

void update(void) {
}

//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 033: // Escape Key
	case 'q': case 'Q':
		exit(EXIT_SUCCESS);
		break;
	}
}

void mouseDrag(int x, int y)
{
	if (x < window_size && y < window_size && x > 0 && y > 0)
	{
		int x_adjCoord = (x * N / window_size);
		int y_adjCoord = (window_size - y) * N / window_size;
		activeSimulator->addDye(y_adjCoord, x_adjCoord, 50.0f);
		activeSimulator->addVelocity(y_adjCoord, x_adjCoord, -(y - prev_mouseY) * 10000, (x - prev_mouseX) * 10000);
		prev_mouseX = x;
		prev_mouseY = y;
	}
}

void renderFluid()
{
	int Index = 0;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			float transferVal = activeCell->density[activeSimulator->GenerateIndex(i, j)];

			// all four vertices of this cell should have the same value
			vertices[Index++].z = glm::min(transferVal, 0.99f);
			vertices[Index++].z = glm::min(transferVal, 0.99f);
			vertices[Index++].z = glm::min(transferVal, 0.99f);
			vertices[Index++].z = glm::min(transferVal, 0.99f);
		}
	}
}

void fade()
{
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			activeCell->density[activeSimulator->GenerateIndex(i, j)] = glm::max(activeCell->density[activeSimulator->GenerateIndex(i, j)] - 0.05f, 0.0f);
		}
	}
}
//----------------------------------------------------------------------------

void reshape(int width, int height) {
}