﻿#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "meshes.h"
#include "camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Way Final Project"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Texture
	GLuint gTextureIdFloor;
	GLuint gTextureIdLamp;
	GLuint gTextureIdLampTop;
	GLuint gTextureIdAmp;
	GLuint gTextureIdCatToy;
	GLuint gTextureIdHeat;
	GLuint gTextureIdGuitar1;
	GLuint gTextureIdNeck;
	GLuint gTextureIdHead;
	glm::vec2 gUVScale(1.0f, 1.0f);
	GLint gTexWrapMode = GL_REPEAT;

	// Shader program
	GLuint gProgramId;
	GLuint gLightProgramId;

	//Shape Meshes from Professor Brian
	Meshes meshes;

	// camera
	Camera gCamera(glm::vec3(0.0f, 5.0f, 15.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;

	int viewProj = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Vertex Shader Source Code*/
const GLchar* surfaceVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
	out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
	out vec2 vertexTextureCoordinate;

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

		vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

		vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
		vertexTextureCoordinate = textureCoordinate;
	}
);

/* Surface Fragment Shader Source Code*/
const GLchar* surfaceFragmentShaderSource = GLSL(440,

	in vec3 vertexFragmentNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor; // For outgoing cube color to the GPU

	// Uniform / Global variables for object color, light color, light position, and camera/view position
	uniform vec4 objectColor;
	uniform vec3 ambientColor;
	uniform vec3 light1Color;
	uniform vec3 light1Position;
	uniform vec3 light2Color;
	uniform vec3 light2Position;
	uniform vec3 viewPosition;
	uniform sampler2D uTexture; // Useful when working with multiple textures
	uniform vec2 uvScale;
	uniform bool ubHasTexture;
	uniform float ambientStrength = 0.8f; // Set ambient or global lighting strength
	uniform float specularIntensity1 = 1.0f;
	uniform float highlightSize1 = 16.0f;
	uniform float specularIntensity2 = 0.1f;
	uniform float highlightSize2 = 16.0f;

	void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient lighting
		vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

		//**Calculate Diffuse lighting**
		vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
		vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse1 = impact1 * light1Color; // Generate diffuse light color
		vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse2 = impact2 * light2Color; // Generate diffuse light color

		//**Calculate Specular lighting**
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
		vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
		vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;
		vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
		vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

		//**Calculate phong result**
		//Texture holds the color to be used for all three components
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
		vec3 phong1;
		vec3 phong2;

		if (ubHasTexture == true)
		{
			phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz;
			phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz;
		}
		else
		{
			phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
			phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
		}

		fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
		//fragmentColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
);

/* Light Object Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(330,
	layout(location = 0) in vec3 aPos;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(aPos, 1.0);
	}
);

/* Light Object Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(330,
	out vec4 FragColor;

	void main()
	{
		FragColor = vec4(1.0); // set all 4 vector values to 1.0
	}
);

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}


int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	//UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
	meshes.CreateMeshes();

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);

	// Create the shader program
	if (!UCreateShaderProgram(surfaceVertexShaderSource, surfaceFragmentShaderSource, gProgramId))
		return EXIT_FAILURE;

	if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
		return EXIT_FAILURE;
	
	const char* texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/woodfloor.jpg";
	if (!UCreateTexture(texFilename, gTextureIdFloor))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/frostedglass.jpg";
	if (!UCreateTexture(texFilename, gTextureIdLampTop))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/lamppost.jpg";
	if (!UCreateTexture(texFilename, gTextureIdLamp))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/amplifier.jpg";
	if (!UCreateTexture(texFilename, gTextureIdAmp))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/cattoy.png";
	if (!UCreateTexture(texFilename, gTextureIdCatToy))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/heater.jpg";
	if (!UCreateTexture(texFilename, gTextureIdHeat))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/guitarbody.jpg";
	if (!UCreateTexture(texFilename, gTextureIdGuitar1))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/neck.jpg";
	if (!UCreateTexture(texFilename, gTextureIdNeck))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "C:/Users/jway2/source/repos/CS330 Final/Resources/Textures/head.jpg";
	if (!UCreateTexture(texFilename, gTextureIdHead))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	//copy texture data for gTextureIdFloor into slot 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdFloor);

	//copy texture data for gTextureIdLampTop into slot 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gTextureIdLampTop);

	//copy texture data for gTextureIdLamp into slot 2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gTextureIdLamp);

	//copy texture data for gTextureIdAmp into slot 3
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gTextureIdAmp);

	//copy texture data for gTextureIdCatToy into slot 4
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, gTextureIdCatToy);

	//copy texture data for gTextureIdHeat into slot 5
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, gTextureIdHeat);

	//copy texture data for gTextureIdGuitar1 into slot 6
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, gTextureIdGuitar1);

	//copy texture data for gTextureIdNeck into slot 7
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, gTextureIdNeck);

	//copy texture data for gTextureIdHead into slot 8
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, gTextureIdHead);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	meshes.DestroyMeshes();

	// Destroy texture
	UDestroyTexture(gTextureIdFloor);
	UDestroyTexture(gTextureIdLamp);
	UDestroyTexture(gTextureIdLampTop);
	UDestroyTexture(gTextureIdAmp);
	UDestroyTexture(gTextureIdCatToy);
	UDestroyTexture(gTextureIdHeat);
	UDestroyTexture(gTextureIdGuitar1);

	// Release shader program
	UDestroyShaderProgram(gProgramId);
	UDestroyShaderProgram(gLightProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
		gCamera.Position = glm::vec3(0.0f, 0.0f, 15.0f);
		gCamera.Front = glm::vec3(0.0f, 5.0f, 0.0f);
		gCamera.Up = glm::vec3(0.0f, 0.0f, 0.0f);

	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		gCamera.Position = glm::vec3(0.0f, 5.0f, 15.0f);
		gCamera.Front = glm::vec3(0.0f, 0.0f, 0.0f);
		gCamera.Up = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	gCamera.ProcessMouseScroll(yoffset);
}


// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}


// Functioned called to render a frame
void URender() {
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 translation;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint objectColorLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint specInt1Loc;
	GLint highlghtSz1Loc;
	GLint specInt2Loc;
	GLint highlghtSz2Loc;
	GLint uHasTextureLoc;
	bool ubHasTextureVal;

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Transforms the camera
	view = gCamera.GetViewMatrix();

	// Creates a orthographic projection
	//projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);

	// Creates a perspective projection
	projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);


	// Set the shader to be used
	glUseProgram(gProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	viewPosLoc = glGetUniformLocation(gProgramId, "viewPosition");
	ambStrLoc = glGetUniformLocation(gProgramId, "ambientStrength");
	ambColLoc = glGetUniformLocation(gProgramId, "ambientColor");
	light1ColLoc = glGetUniformLocation(gProgramId, "light1Color");
	light1PosLoc = glGetUniformLocation(gProgramId, "light1Position");
	light2ColLoc = glGetUniformLocation(gProgramId, "light2Color");
	light2PosLoc = glGetUniformLocation(gProgramId, "light2Position");
	objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
	specInt1Loc = glGetUniformLocation(gProgramId, "specularIntensity1");
	highlghtSz1Loc = glGetUniformLocation(gProgramId, "highlightSize1");
	specInt2Loc = glGetUniformLocation(gProgramId, "specularIntensity2");
	highlghtSz2Loc = glGetUniformLocation(gProgramId, "highlightSize2");
	uHasTextureLoc = glGetUniformLocation(gProgramId, "ubHasTexture");
	uHasTextureLoc = glGetUniformLocation(gProgramId, "ubHasTexture");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set ambient lighting strength
	glUniform1f(ambStrLoc, 0.5f);
	//set ambient color
	glUniform3f(ambColLoc, 0.5f, 0.5f, 0.5f);
	glUniform3f(light1ColLoc, 0.8f, 0.8f, 0.3f);
	glUniform3f(light1PosLoc, -1.5f, 10.0f, -5.0f);
	glUniform3f(light2ColLoc, 0.2f, 0.2f, 0.2f);
	glUniform3f(light2PosLoc, 0.0f, 5.0f, 3.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, 1.0f);
	glUniform1f(specInt2Loc, 0.1f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 10.0f);

	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));


	///////////////////////////////////////////////////////////////////////////////
	// Floor
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(6.0f, 1.0f, 6.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 0.0f, 0.0f, 1.0f);

	//reference texture slot 0 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Lamp
	// Lamp Base
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.0f, 0.2f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-1.5f, 0.01f, -5.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Lamp Shaft
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 9.0f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-1.5f, 0.01f, -5.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Lamp Top
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gConeMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.2f, 1.2f, 1.2f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(180.0f), glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-1.5f, 10.0f, -5.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//reference texture slot 1 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);

	//glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 0.0f, 1.0f, 1.0f);
	glUniform4f(objectColorLoc, 1.0f, 0.0f, 0.0f, 1.0f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_STRIP, 36, 108);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Large Amp
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(4.0f, 2.5f, 2.2f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(2.0f, 1.27f, -4.8f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

	//reference texture slot 3 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);

	glUniform3f(light1ColLoc, 0.6f, 0.6f, 0.3f);
	glUniform3f(light1PosLoc, 2.0f, 5.0f, -4.8f);
	//set specular intensity
	glUniform1f(specInt1Loc, 0.1f);
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 10.0f);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Small Amp
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.6f, 1.8f, 1.5f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(3.25f, 0.91f, -2.8f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

	//reference texture slot 3 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Space Heater
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.45f, 2.0f, 0.45f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(3.5f, 0.01f, -0.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 5 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 5);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Cat toy
	// Base
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.8f, 0.8f, 1.5f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.15f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);

	//reference texture slot 4 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Middle
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.7f, 0.7f, 1.5f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.45f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);

	//reference texture slot 4 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Top
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.6f, 0.6f, 1.5f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.7f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);

	//reference texture slot 4 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Guitar Stand
	// Right leg
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 1.5f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(80.0f), glm::vec3(0.0, 0.2f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-2.5f, 0.1f, -2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Left leg
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 1.5f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(80.0f), glm::vec3(-1.2f, 0.3f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.8f, 0.1f, -0.4f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Back leg
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 0.7f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(60.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.5f, 0.1f, -2.2f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Main post
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 3.0f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.05f, 0.4f, -1.7f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottom Holder Connection
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 0.4f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.05f, 0.5f, -1.7f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottom Holder Back
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 1.3f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.3f, 0.5f, -1.8f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottom Holder Right
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 1.0f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.4f, 0.5f, -1.8f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Bottom Holder Left
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 1.0f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.2f, 0.5f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	
	// Top Holder Connection
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 0.4f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.05f, 3.3f, -1.7f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Top Holder Back
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 0.5f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.55f, 3.3f, -1.55f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Top Holder Right
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 0.5f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.55f, 3.3f, -1.65f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Top Holder Left
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.1f, 0.5f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.0f, 3.3f, -1.2f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// Guitar
	// Lower Body
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.8f, 0.25f, 0.8f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.6f, 1.18f, -1.2f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 6);

	glUniform3f(light1ColLoc, 0.6f, 0.6f, 0.3f);
	glUniform3f(light1PosLoc, -3.1f, 1.18f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, 0.1f);

	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 0.5f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// Upper Body
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.6f, 0.23f, 0.6f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, -1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.59f, 2.2f, -1.19f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	//reference texture slot 2 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 6);

	glUniform3f(light1ColLoc, 0.6f, 0.6f, 0.3f);
	glUniform3f(light1PosLoc, -3.1f, 2.2f, -1.0f);
	//set specular intensity
	glUniform1f(specInt1Loc, 0.1f);

	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 0.5f);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// Neck
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.25f, 2.0f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.5f, 3.6f, -1.1f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

	//reference texture slot 7 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 7);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Head
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.35f, 0.5f, 0.08f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-3.5f, 4.8f, -1.1f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

	//reference texture slot 7 before drawing
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 8);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	///////////////////////////////////////////////////////////////////////////////


	// Set the shader to be used
	glUseProgram(gLightProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gLightProgramId, "model");
	viewLoc = glGetUniformLocation(gLightProgramId, "view");
	projLoc = glGetUniformLocation(gLightProgramId, "projection");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glUseProgram(0);
	 
	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId) {
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}
	
	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}


// Plane
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gPlaneMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(6.0f, 1.0f, 6.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 0.0f, 0.0f, 1.0f);

// Draws the triangles
glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Box
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gBoxMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(-3.5f, 1.0f, -3.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

// Draws the triangles
glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// 3-Sided Pyramid
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gPyramid3Mesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(-2.0f, 1.0f, -3.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 0.5f, 0.5f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid3Mesh.nVertices);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// 4-Sided Pyramid
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gPyramid4Mesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(0.0f, 1.0f, -3.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.0f, 0.5f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid4Mesh.nVertices);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Prism
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gPrismMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(2.0f, 1.0f, -3.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 0.0f, 0.5f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPrismMesh.nVertices);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Cone
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gConeMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(4.0f, 0.5f, -3.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 0.0f, 1.0f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
glDrawArrays(GL_TRIANGLE_STRIP, 36, 108);	//sides

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Cylinder
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gCylinderMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(-3.5f, 1.0f, 5.5f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Tapered Cylinder
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gTaperedCylinderMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(-1.2f, 1.0f, 5.5f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 1.0f, 1.0f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Torus
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gTorusMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(1.2f, 1.3f, 5.0f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);

// Draws the triangles
glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/

// Sphere
/*// Activate the VBOs contained within the mesh's VAO
glBindVertexArray(meshes.gSphereMesh.vao);

// 1. Scales the object
scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
// 2. Rotate the object
rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
// 3. Position the object
translation = glm::translate(glm::vec3(4.0f, 1.0f, 4.3f));
// Model matrix: transformations are applied right-to-left order
model = translation * rotation * scale;
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 1.0f, 0.0f, 1.0f);

// Draws the triangles
glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

// Deactivate the Vertex Array Object
glBindVertexArray(0);*/