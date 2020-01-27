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

// Model-view and projection matrices uniform location
GLuint ModelView, ModelViewInverseTranspose, Projection;
GLuint Time;
GLboolean UseLighting;

FluidCell* activeCell;
FluidSimulator* activeSimulator;

//----------------------------------------------------------------------------

// OpenGL initialization
void init() {
	//create a new fluid cell
	activeCell = new FluidCell(0.2f, 0.0f, 0.00001f);
	activeSimulator = new FluidSimulator(activeCell, 16);

	// create the height field vertices (a 2D grid in the x-z plane)
	int Index = 0;
	int FaceIndex = 0;
	float dx = 2.0f / N, dy = 2.0f / N;

	for (int i = 0; i < N; i++) {
		float y = i * dy - 1.0f;
		for (int j = 0; j < N; j++) {
			float x = j * dx - 1.0f;
			vertices[Index++] = point4(x, y, 1.0, 1.0); //point4(x, y, (Index - 0.0f) / NumVertices, 1.0);
			vertices[Index++] = point4(x + dx, y, 1.0, 1.0); //point4(x + dx, y, (Index - 1.0f) / NumVertices, 1.0);
			vertices[Index++] = point4(x + dx, y + dy, 1.0, 1.0); //point4(x + dx, y + dy, (Index - 2.0f) / NumVertices, 1.0);
			vertices[Index++] = point4(x, y + dy, 1.0, 1.0); //point4(x, y + dy, (Index - 3.0f) / NumVertices, 1.0);
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

	// Initialize shader lighting parameters
	point4 light_position(0.0, 0.0, -1.0, 0.0);
	color4 light_ambient(0.2, 0.2, 0.2, 1.0);
	color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
	color4 light_specular(1.0, 1.0, 1.0, 1.0);

	color4 material_ambient(0.0215, 0.1745, 0.0215, 1.0);
	color4 material_diffuse(0.07568, 0.61424, 0.07568, 1.0);
	color4 material_specular(0.633, 0.727811, 0.633, 1.0);
	float  material_shininess = 60.0;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, glm::value_ptr(ambient_product));
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, glm::value_ptr(diffuse_product));
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, glm::value_ptr(specular_product));

	glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, glm::value_ptr(light_position));

	glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);

	glUniform1f(glGetUniformLocation(program, "Distance"), 1.0f / N);

	// Retrieve transformation uniform variable locations
	ModelView = glGetUniformLocation(program, "ModelView");
	ModelViewInverseTranspose = glGetUniformLocation(program, "ModelViewInverseTranspose");
	Projection = glGetUniformLocation(program, "Projection");
	Time = glGetUniformLocation(program, "Time");
	UseLighting = glGetUniformLocation(program, "UseLighting");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_FLAT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//  Generate the model-view matrix
	// TODO: Support the mdoel-view + lighting/materials
	const glm::vec3 viewer_pos(0.0, 0.0, 1.2f);
	const glm::vec3 model_trans(-0.0, 0.0, 0.0);
	glm::mat4 trans, rot, model_view;
	trans = glm::translate(trans, -viewer_pos);
	model_view = trans;

	long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	glUniform1f(Time, (ms % 1000000) / 1000.0f);
	glUniformMatrix4fv(ModelView, 1, GL_FALSE, glm::value_ptr(model_view));
	glUniformMatrix4fv(ModelViewInverseTranspose, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(model_view))));

	glUniform1f(UseLighting, true);
	glScalef(2.0, 2.0, 1.0);
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
	// TO-DO: Add more features
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
	glViewport(0, 0, width, height);

	GLfloat aspect = GLfloat(width) / height;
	glm::mat4  projection = glm::ortho(glm::radians(45.0f), aspect, 0.5f, 3.0f);
	glUniformMatrix4fv(Projection, 1, GL_FALSE, glm::value_ptr(projection));
}