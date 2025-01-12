// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <stack>
#include <sstream>
#include <map>
// Include GLEW
#include <GL/glew.h>
// Include GLFW
#include <GLFW/glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;
// Include AntTweakBar
// #include <AntTweakBar.h>
#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "common/stb_image.h"
const int window_width = 1024, window_height = 768;
struct HalfEdge;
typedef struct Vertex {
	float Position[4];
	float Color[4];
	float Normal[3];
	float TexCoord[2];
	std::vector<int> neighbors;
	HalfEdge* edge;
	int index;
	Vertex() {
		std::fill(std::begin(Position), std::end(Position), 0.0f);
		std::fill(std::begin(Color), std::end(Color), 0.0f);
		std::fill(std::begin(Normal), std::end(Normal), 0.0f);
		std::fill(std::begin(TexCoord), std::end(TexCoord), 0.0f);
	}
	Vertex(const float* pos) {
		Position[0] = pos[0];
		Position[1] = pos[1];
		Position[2] = pos[2];
		Position[3] = 1.0f;
		std::fill(std::begin(Color), std::end(Color), 0.0f);
		std::fill(std::begin(Normal), std::end(Normal), 0.0f);
		std::fill(std::begin(TexCoord), std::end(TexCoord), 0.0f);
	}
	Vertex(const glm::vec3& pos) {
		Position[0] = pos.x;
		Position[1] = pos.y;
		Position[2] = pos.z;
		Position[3] = 1.0f;
		std::fill(std::begin(Color), std::end(Color), 0.0f);
		std::fill(std::begin(Normal), std::end(Normal), 0.0f);
		std::fill(std::begin(TexCoord), std::end(TexCoord), 0.0f);
	}
	void SetPosition(float* coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetColor(float* color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
	void SetNormal(float* coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
	void SetTexCoord(float* coords) {
		TexCoord[0] = coords[0];
		TexCoord[1] = coords[1];
	}
	bool operator<(const Vertex& other) const {
		for (int i = 0; i < 4; i++) {
			if (Position[i] != other.Position[i])
				return Position[i] < other.Position[i];
		}
		for (int i = 0; i < 4; i++) {
			if (Color[i] != other.Color[i])
				return Color[i] < other.Color[i];
		}
		for (int i = 0; i < 3; i++) {
			if (Normal[i] != other.Normal[i])
				return Normal[i] < other.Normal[i];
		}
		for (int i = 0; i < 2; i++) {
			if (TexCoord[i] != other.TexCoord[i])
				return TexCoord[i] < other.TexCoord[i];
		}
		return false;
	}
};
struct Edge {
	int v1, v2;
	Edge(int vertex1, int vertex2)
		: v1(std::min(vertex1, vertex2)), v2(std::max(vertex1, vertex2)) {
	}
	Edge() : v1(0), v2(0) {}
	bool operator<(const Edge& other) const {
		return std::tie(v1, v2) < std::tie(other.v1, other.v2);
	}
};
struct Face {
	int v1, v2, v3, v4;
	HalfEdge* edge;
	Face() : v1(-1), v2(-1), v3(-1), v4(-1), edge(nullptr) {}
	Face(int vertex1, int vertex2, int vertex3, int vertex4 = -1)
		: v1(vertex1), v2(vertex2), v3(vertex3), v4(vertex4),
		edge(nullptr) {
	}
};
struct HalfEdge {
	Vertex* origin;
	HalfEdge* twin;
	HalfEdge* next;
	HalfEdge* prev;
	struct Face* face;
	HalfEdge() : origin(nullptr), twin(nullptr), next(nullptr), prev(nullptr),
		face(nullptr) {
	}
};
std::vector<Vertex> vertices;
std::vector<Face> faces;
std::map<Edge, int> edges;
// function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], GLushort[], int);
void loadObject(char*, glm::vec4, Vertex*&, GLushort*&, int);
void createObjects(void);
void pickObject(void);
void renderScene(void);
void cleanup(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);
// GLOBAL VARIABLES
GLFWwindow* window;
glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;
GLuint gPickedIndex = -1;
std::string gMessage;
GLuint programID;
GLuint pickingProgramID;
float horizAngle = 3.14f / 2.0f;
float vertAngle = 0.0f;
float radius = 20.0f;
float cameraSpeed = 0.05f;
glm::vec3 cameraPosition;
glm::vec3 upVector(0.0f, 1.0f, 0.0f);
bool isWireframe = false;
bool showTexture = false;
bool showSubdivided = false;
glm::vec3 lightPos1 = glm::vec3(-2.0f, 5.0f, 5.0f);
glm::vec3 lightDiffuseColor1 = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 lightAmbientColor1 = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 lightSpecularColor1 = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 lightPos2 = glm::vec3(2.0f, 5.0f, -5.0f);
glm::vec3 lightDiffuseColor2 = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 lightAmbientColor2 = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 lightSpecularColor2 = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 objectColor = glm::vec3(0.7f, 0.7f, 0.7f);
glm::vec3 materialDiffuse = objectColor;
glm::vec3 materialAmbient = objectColor;
glm::vec3 materialSpecular = materialDiffuse * 0.1f;
float materialShininess = 20.0f;
std::vector<glm::vec3> normals;
std::vector<glm::vec2> uvs;
const GLuint NumObjects = 9; // ATTN: THIS NEEDS TO CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];
// TL
size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumIdcs[NumObjects];
size_t NumVerts[NumObjects];
GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ViewMatrixID;
GLuint ProjMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint LightID;
GLuint faceObjectID = 2;
GLuint faceTextObjectID = 3;
GLuint textureID = 4;
GLuint controlNetID = 5;
// Declare global objects
// TL
const size_t CoordVertsCount = 6;
Vertex CoordVerts[CoordVertsCount];
int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //FOR MAC
		// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height,
		"Bosworth, Kinnara (71760772)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible.Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}
	// Initialize the GUI
	/*TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar* GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING,
	&gMessage, NULL);*/
	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	return 0;
}
void initOpenGL(void) {
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);
	// Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit < -> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); //In world coordinates
		// Camera matrix
	gViewMatrix = glm::lookAt(glm::vec3(10.0, 10.0, 10.0f), // eye
		glm::vec3(0.0, 0.0, 0.0), // center
		glm::vec3(0.0, 1.0, 0.0)); // up
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("StandardShading.vertexshader",
		"StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("Picking.vertexshader",
		"Picking.fragmentshader");
	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");
	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID,
		"PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID,
		"LightPosition_worldspace");
	// TL
	// Define objects
	createObjects();
	// ATTN: create VAOs for each of the newly created objects here:
	VertexBufferSize[0] = sizeof(CoordVerts);
	NumVerts[0] = CoordVertsCount;
	createVAOs(CoordVerts, NULL, 0);
}
void createVAOs(Vertex Vertices[], GLushort Indices[], int ObjectId) {
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices,
		GL_STATIC_DRAW);
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
			IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize,
		(GLvoid*)0); // Position
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize,
		(GLvoid*)sizeof(Vertex::Position)); // Color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)
		(sizeof(Vertex::Position) + sizeof(Vertex::Color))); // Normal
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)
		(sizeof(Vertex::Position) + sizeof(Vertex::Color) + sizeof(Vertex::Normal))); // TexCoord
	glEnableVertexAttribArray(0); // Position
	glEnableVertexAttribArray(1); // Color
	glEnableVertexAttribArray(2); // Normal
	glEnableVertexAttribArray(3); // TexCoord
	glBindVertexArray(0);
	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR) {
		fprintf(stderr, "ERROR: Could not create a VBO: %s\n",
			gluErrorString(ErrorCheckValue));
	}
}
void indexVBO(
	std::vector<glm::vec3>& in_vertices,
	std::vector<glm::vec3>& in_normals,
	std::vector<glm::vec2>& in_uvs,
	std::vector<unsigned short>& out_indices,
	std::vector<Vertex>& out_vertices
) {
	std::map<Vertex, unsigned short> vertexToIndex;
	for (size_t i = 0; i < in_vertices.size(); i++) {
		Vertex vertex;
		vertex.SetPosition(&in_vertices[i].x);
		vertex.SetNormal(&in_normals[i].x);
		vertex.SetTexCoord(&in_uvs[i].x);
		if (vertexToIndex.find(vertex) != vertexToIndex.end()) {
			out_indices.push_back(vertexToIndex[vertex]);
		}
		else {
			unsigned short newIndex = static_cast<unsigned
				short>(out_vertices.size());
			out_vertices.push_back(vertex);
			vertexToIndex[vertex] = newIndex;
			out_indices.push_back(newIndex);
		}
	}
}
// Ensure your .obj files are in the correct format and properly loaded by looking at the following function
void loadObject(char* file, glm::vec4 color, Vertex*& out_Vertices,
	GLushort*& out_Indices, int ObjectId) {
	// Temporary vectors to process data
	std::vector<glm::vec3> tempVertices;
	std::vector<glm::vec3> tempNormals;
	std::vector<glm::vec2> tempUVs;
	bool res = loadOBJ(file, tempVertices, tempNormals, tempUVs);
	if (!res) {
		printf("Failed to load OBJ file: %s\n", file);
		return;
	}
	printf("OBJ Loaded: %zu vertices, %zu normals, %zu UVs\n", tempVertices.size(), tempNormals.size(), tempUVs.size());
	// Convert to indexed format
	std::vector<unsigned short> indices;
	std::vector<Vertex> indexedVertices;
	indexVBO(tempVertices, tempNormals, tempUVs, indices, indexedVertices);
	printf("Indexed data: %zu vertices, %zu indices\n", indexedVertices.size(), indices.size());
	// Allocate and transfer data to output pointers
	size_t vertCount = indexedVertices.size();
	size_t idxCount = indices.size();
	out_Vertices = new Vertex[vertCount];
	out_Indices = new GLushort[idxCount];
	for (size_t i = 0; i < vertCount; ++i) {
		out_Vertices[i] = indexedVertices[i];
	}
	for (size_t i = 0; i < idxCount; ++i) {
		out_Indices[i] = indices[i];
	}
	// Store buffer sizes
	VertexBufferSize[ObjectId] = sizeof(Vertex) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;
	NumIdcs[ObjectId] = idxCount;
}
void addEdge(int v1, int v2) {
	if (v1 > v2) std::swap(v1, v2);
	Edge edgeKey(v1, v2);
	if (edges.find(edgeKey) == edges.end()) {
		edges[edgeKey] = -1;
	}
}
GLuint loadTexture(const char* filepath) {
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filepath, &width, &height,
		&nrChannels, 0);
	GLenum format = GL_RGB;
	if (nrChannels == 1) {
		format = GL_RED;
	}
	else if (nrChannels == 3) {
		format = GL_RGB;
	}
	else if (nrChannels == 4) {
		format = GL_RGBA;
	}
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
		GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
	stbi_image_free(data);
	return textureID;
}
void createObjects(void) {
	//-- COORDINATE AXES --//
	/*CoordVerts[0] = { { 0.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0,
	0.0, 1.0 } };
	CoordVerts[1] = { { 5.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0,
	0.0, 1.0 } };
	CoordVerts[2] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0,
	0.0, 1.0 } };
	CoordVerts[3] = { { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0,
	0.0, 1.0 } };
	CoordVerts[4] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0,
	0.0, 1.0 } };
	CoordVerts[5] = { { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0,
	0.0, 1.0 } };*/
	//-- GRID --//
	// ATTN: Create your grid vertices here!
	//-- .OBJs --//
	// ATTN: Load your models here through .obj files -- example of how to do so is as shown
		// Vertex* Verts;
		// GLushort* Idcs;
		// loadObject("models/base.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), Verts, Idcs, ObjectID);
		// createVAOs(Verts, Idcs, ObjectID);
	Vertex* faceVerts;
	GLushort* faceIndices;
	loadObject("../common/newHead3.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), faceVerts, faceIndices, faceObjectID);
	vertices.assign(faceVerts, faceVerts + (VertexBufferSize[faceObjectID] / sizeof(Vertex)));

	for (size_t i = 0; i < NumIdcs[faceObjectID]; i += 3) {
		faces.push_back({ faceIndices[i], faceIndices[i + 1], faceIndices[i
		+ 2] });
	}
	createVAOs(faceVerts, faceIndices, faceObjectID);
	Vertex* faceTextVerts;
	GLushort* faceTextIndices;
	loadObject("../common/headWithTexture.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), faceTextVerts, faceTextIndices, faceTextObjectID);
	createVAOs(faceTextVerts, faceTextIndices, faceTextObjectID);
	printf("num verts: %zu\n", NumVerts[faceObjectID]);
	stbi_set_flip_vertically_on_load(true);
	textureID = loadTexture("../common/faceImage.jpg");
	std::vector<GLushort> controlNetIndices;
	for (const auto& face : faces) {
		controlNetIndices.push_back(face.v1);
		controlNetIndices.push_back(face.v2);
		controlNetIndices.push_back(face.v2);
		controlNetIndices.push_back(face.v3);
		controlNetIndices.push_back(face.v3);
		controlNetIndices.push_back(face.v1);
	}
	VertexBufferSize[controlNetID] = sizeof(Vertex) * vertices.size();
	IndexBufferSize[controlNetID] = sizeof(GLushort) * controlNetIndices.size();
	NumIdcs[controlNetID] = controlNetIndices.size();
	createVAOs(faceVerts, faceIndices, controlNetID);
}
bool isBoundaryEdge(const Edge& edge, const std::map<Edge,
	std::vector<int>>&adjacentTriangles) {
	return adjacentTriangles.at(edge).size() == 1;
}
bool isBoundaryVertex(int vertexIndex, const std::map<Edge,
	std::vector<int>>&adjacentTriangles) {
	for (auto it = adjacentTriangles.begin(); it != adjacentTriangles.end(); ++it) {
		const Edge& edge = it->first;
		const std::vector<int>& triangles = it->second;
		if ((edge.v1 == vertexIndex || edge.v2 == vertexIndex) &&
			triangles.size() == 1) {
			return true;
		}
	}
	return false;
}
glm::vec3 updateBoundaryVertex(
	const Vertex& vertex,
	const std::map<Edge, glm::vec3>& edgePoints,
	const std::map<Edge, std::vector<int>>& adjacentTriangles
) {
	glm::vec3 updatedPosition = glm::vec3(vertex.Position[0],
		vertex.Position[1], vertex.Position[2]);
	glm::vec3 edgeSum = glm::vec3(0.0f);
	int count = 0;
	for (auto it = edgePoints.begin(); it != edgePoints.end(); ++it) {
		const Edge& edge = it->first;
		const glm::vec3& edgePoint = it->second;
		if (edge.v1 == vertex.index || edge.v2 == vertex.index) {
			auto adjacentIt = adjacentTriangles.find(edge);
			if (adjacentIt != adjacentTriangles.end() && adjacentIt->second.size() == 1) {
				edgeSum += edgePoint;
				count++;
			}
		}
	}
	if (count > 0) {
		updatedPosition = 0.75f * updatedPosition + 0.25f * (edgeSum /
			float(count));
	}
	return updatedPosition;
}
std::map<Edge, std::vector<int>> buildAdjacentTriangles(const
	std::vector<Face>& faces) {
	std::map<Edge, std::vector<int>> adjacentTriangles;
	for (size_t i = 0; i < faces.size(); ++i) {
		const Face& face = faces[i];
		adjacentTriangles[Edge(face.v1, face.v2)].push_back(i);
		adjacentTriangles[Edge(face.v2, face.v3)].push_back(i);
		adjacentTriangles[Edge(face.v3, face.v1)].push_back(i);
	}
	return adjacentTriangles;
}
void subdivideTriangle(
	const Face& triangle,
	int depth,
	const std::vector<Vertex>& vertices,
	std::vector<Vertex>& newVertices,
	std::map<Edge, int>& edgeCache,
	std::vector<Face>& newFaces
) {
	if (depth == 0) {
		newFaces.push_back(triangle);
		return;
	}
	Edge edge1(std::min(triangle.v1, triangle.v2), std::max(triangle.v1,
		triangle.v2));
	Edge edge2(std::min(triangle.v2, triangle.v3), std::max(triangle.v2,
		triangle.v3));
	Edge edge3(std::min(triangle.v3, triangle.v1), std::max(triangle.v3,
		triangle.v1));
	int m1, m2, m3;
	if (edgeCache.find(edge1) == edgeCache.end()) {
		Vertex midpoint;
		for (int i = 0; i < 3; i++) {
			midpoint.Position[i] = (vertices[triangle.v1].Position[i] +
				vertices[triangle.v2].Position[i]) / 2.0f;
			midpoint.Normal[i] = (vertices[triangle.v1].Normal[i] +
				vertices[triangle.v2].Normal[i]) / 2.0f;
		}
		midpoint.TexCoord[0] = (vertices[triangle.v1].TexCoord[0] +
			vertices[triangle.v2].TexCoord[0]) / 2.0f;
		midpoint.TexCoord[1] = (vertices[triangle.v1].TexCoord[1] +
			vertices[triangle.v2].TexCoord[1]) / 2.0f;
		m1 = newVertices.size();
		edgeCache[edge1] = newVertices.size();
		newVertices.push_back(midpoint);
	}
	else {
		m1 = edgeCache[edge1];
	}
	if (edgeCache.find(edge2) == edgeCache.end()) {
		Vertex midpoint;
		for (int i = 0; i < 3; i++) {
			midpoint.Position[i] = (vertices[triangle.v2].Position[i] +
				vertices[triangle.v3].Position[i]) / 2.0f;
			midpoint.Normal[i] = (vertices[triangle.v2].Normal[i] +
				vertices[triangle.v3].Normal[i]) / 2.0f;
		}
		midpoint.TexCoord[0] = (vertices[triangle.v2].TexCoord[0] +
			vertices[triangle.v3].TexCoord[0]) / 2.0f;
		midpoint.TexCoord[1] = (vertices[triangle.v2].TexCoord[1] +
			vertices[triangle.v3].TexCoord[1]) / 2.0f;
		m2 = newVertices.size();
		edgeCache[edge2] = newVertices.size();
		newVertices.push_back(midpoint);
	}
	else {
		m2 = edgeCache[edge2];
	}
	if (edgeCache.find(edge3) == edgeCache.end()) {
		Vertex midpoint;
		for (int i = 0; i < 3; i++) {
			midpoint.Position[i] = (vertices[triangle.v3].Position[i] +
				vertices[triangle.v1].Position[i]) / 2.0f;
			midpoint.Normal[i] = (vertices[triangle.v3].Normal[i] +
				vertices[triangle.v1].Normal[i]) / 2.0f;
		}
		midpoint.TexCoord[0] = (vertices[triangle.v3].TexCoord[0] +
			vertices[triangle.v1].TexCoord[0]) / 2.0f;
		midpoint.TexCoord[1] = (vertices[triangle.v3].TexCoord[1] +
			vertices[triangle.v1].TexCoord[1]) / 2.0f;
		m3 = newVertices.size();
		edgeCache[edge3] = newVertices.size();
		newVertices.push_back(midpoint);
	}
	else {
		m3 = edgeCache[edge3];
	}
	Face t1 = { triangle.v1, m1, m3 };
	Face t2 = { m1, triangle.v2, m2 };
	Face t3 = { m3, m2, triangle.v3 };
	Face t4 = { m1, m2, m3 };
	subdivideTriangle(t1, depth - 1, vertices, newVertices, edgeCache,
		newFaces);
	subdivideTriangle(t2, depth - 1, vertices, newVertices, edgeCache,
		newFaces);
	subdivideTriangle(t3, depth - 1, vertices, newVertices, edgeCache,
		newFaces);
	subdivideTriangle(t4, depth - 1, vertices, newVertices, edgeCache,
		newFaces);
}
std::vector<glm::vec3> computeFacePoints(const std::vector<Vertex>&
	vertices, const std::vector<Face>& faces) {
	std::vector<glm::vec3> facePoints(faces.size());
	for (size_t i = 0; i < faces.size(); ++i) {
		const auto& face = faces[i];
		glm::vec3 sum = glm::vec3(0.0f);
		sum += glm::vec3(vertices[face.v1].Position[0],
			vertices[face.v1].Position[1], vertices[face.v1].Position[2]);
		sum += glm::vec3(vertices[face.v2].Position[0],
			vertices[face.v2].Position[1], vertices[face.v2].Position[2]);
		sum += glm::vec3(vertices[face.v3].Position[0],
			vertices[face.v3].Position[1], vertices[face.v3].Position[2]);
		facePoints[i] = sum / 3.0f;
	}
	return facePoints;
}
std::map<Edge, glm::vec3> computeEdgePoints(
	const std::vector<Vertex>& vertices,
	const std::vector<Face>& faces,
	const std::map<Edge, std::vector<int>>& adjacentTriangles
) {
	std::map<Edge, glm::vec3> edgePoints;
	for (auto it = adjacentTriangles.begin(); it != adjacentTriangles.end(); ++it) {
		const Edge& edge = it->first;
		const std::vector<int>& triangles = it->second;
		glm::vec3 v1 = glm::vec3(vertices[edge.v1].Position[0],
			vertices[edge.v1].Position[1], vertices[edge.v1].Position[2]);
		glm::vec3 v2 = glm::vec3(vertices[edge.v2].Position[0],
			vertices[edge.v2].Position[1], vertices[edge.v2].Position[2]);
		if (triangles.size() == 1) {
			edgePoints[edge] = 0.5f * (v1 + v2);
		}
		else if (triangles.size() == 2) {
			int t1 = triangles[0];
			int t2 = triangles[1];
			int opposite1 = (faces[t1].v1 != edge.v1 && faces[t1].v1 != edge.v2) ? faces[t1].v1 : (faces[t1].v2 != edge.v1 && faces[t1].v2 != edge.v2) ? faces[t1].v2 : faces[t1].v3;
			int opposite2 = (faces[t2].v1 != edge.v1 && faces[t2].v1 != edge.v2) ? faces[t2].v1 : (faces[t2].v2 != edge.v1 && faces[t2].v2 != edge.v2) ? faces[t2].v2 : faces[t2].v3;
			glm::vec3 n1 = glm::vec3(vertices[opposite1].Position[0], vertices[opposite1].Position[1], vertices[opposite1].Position[2]);
			glm::vec3 n2 = glm::vec3(vertices[opposite2].Position[0], vertices[opposite2].Position[1], vertices[opposite2].Position[2]);
			edgePoints[edge] = (3.0f / 8.0f) * (v1 + v2) + (1.0f / 8.0f) * (n1 + n2);
		}
	}
	return edgePoints;
}
std::vector<glm::vec3> computeVertexPoints(
	const std::vector<Vertex>& vertices,
	const std::map<Edge, glm::vec3>& edgePoints,
	const std::map<Edge, std::vector<int>>& adjacentTriangles
) {
	std::vector<glm::vec3> updatedVertices(vertices.size(),
		glm::vec3(0.0f));
	std::vector<int> valence(vertices.size(), 0);
	for (auto it = edgePoints.begin(); it != edgePoints.end(); ++it) {
		const Edge& edge = it->first;
		const glm::vec3& edgePoint = it->second;
		updatedVertices[edge.v1] += edgePoint;
		updatedVertices[edge.v2] += edgePoint;
		valence[edge.v1]++;
		valence[edge.v2]++;
	}
	for (size_t i = 0; i < vertices.size(); ++i) {
		if (isBoundaryVertex(i, adjacentTriangles)) {
			updatedVertices[i] = updateBoundaryVertex(vertices[i],
				edgePoints, adjacentTriangles);
		}
		else {
			glm::vec3 v = glm::vec3(vertices[i].Position[0],
				vertices[i].Position[1], vertices[i].Position[2]);
			float beta = (valence[i] == 3) ? (5.0f / 16.0f) : (5.0f / (8.0f *
				valence[i]));
			updatedVertices[i] = (1.0f - valence[i] * beta) * v + beta *
				updatedVertices[i];
		}
	}
	return updatedVertices;
}
void addEdgePointsToVertices(
	const std::map<Edge, glm::vec3>& edgePoints,
	std::vector<Vertex>& newVertices,
	std::map<Edge, int>& edgePointIndices,
	int& index
) {
	for (auto it = edgePoints.begin(); it != edgePoints.end(); ++it) {
		const Edge& edge = it->first;
		const glm::vec3& edgePoint = it->second;
		Vertex edgeVertex = {};
		edgeVertex.SetPosition(new float[3] { edgePoint.x, edgePoint.y,
			edgePoint.z });
		newVertices.push_back(edgeVertex);
		edgePointIndices[edge] = index++;
	}
}
void recursiveSubdivideMesh(
	const std::vector<Vertex>& vertices,
	const std::vector<Face>& faces,
	int depth,
	std::vector<Vertex>& newVertices,
	std::vector<Face>& newFaces
) {
	std::map<Edge, int> edgeCache;
	newVertices = vertices;
	for (const auto& face : faces) {
		subdivideTriangle(face, depth, vertices, newVertices, edgeCache,
			newFaces);
	}
}
void pickObject(void) {
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		// ATTN: DRAW YOUR PICKING SCENE HERE. REMEMBER TO SEND IN A DIFFERENT PICKING COLOR FOR EACH OBJECT BEFOREHAND
		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow !
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFlush();
	glFinish();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel,
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with(0, 0) on top
	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);
	if (gPickedIndex == 255) { // Full white, must be the background !
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "point " << gPickedIndex;
		gMessage = oss.str();
	}
	// Uncomment these lines to see the picking shader in effect
	//glfwSwapBuffers(window);
	//continue; // skips the normal rendering
}
void updateCamera() {
	cameraPosition = glm::vec3(
		radius * cos(vertAngle) * sin(horizAngle), // x
		radius * sin(vertAngle), // y
		radius * cos(vertAngle) * cos(horizAngle) // z
	);
	gViewMatrix = glm::lookAt(
		cameraPosition,
		glm::vec3(0.0f, 0.0f, 0.0f),
		upVector
	);
}
void renderScene(void) {
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(programID);
	{
		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glm::mat4x4 ModelMatrix = glm::mat4(1.0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0]
			[0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE,
			&gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0]
			[0]);
		// light 1
		glUniform3f(glGetUniformLocation(programID, "lightPos1"), lightPos1.x, lightPos1.y, lightPos1.z);
		glUniform3f(glGetUniformLocation(programID, "lightDiffuse1"), lightDiffuseColor1.x, lightDiffuseColor1.y, lightDiffuseColor1.z);
		glUniform3f(glGetUniformLocation(programID, "lightAmbient1"), lightAmbientColor1.x, lightAmbientColor1.y, lightAmbientColor1.z);
		glUniform3f(glGetUniformLocation(programID, "lightSpecular1"), lightSpecularColor1.x, lightSpecularColor1.y, lightSpecularColor1.z);
		// light 2
		glUniform3f(glGetUniformLocation(programID, "lightPos2"), lightPos2.x, lightPos2.y, lightPos2.z);
		glUniform3f(glGetUniformLocation(programID, "lightDiffuse2"), lightDiffuseColor2.x, lightDiffuseColor2.y, lightDiffuseColor2.z);
		glUniform3f(glGetUniformLocation(programID, "lightAmbient2"), lightAmbientColor2.x, lightAmbientColor2.y, lightAmbientColor2.z);
		glUniform3f(glGetUniformLocation(programID, "lightSpecular2"), lightSpecularColor2.x, lightSpecularColor2.y, lightSpecularColor2.z);
		// material
		glUniform3f(glGetUniformLocation(programID, "materialDiffuse"), materialDiffuse.x, materialDiffuse.y, materialDiffuse.z);
		glUniform3f(glGetUniformLocation(programID, "materialAmbient"), materialAmbient.x, materialAmbient.y, materialAmbient.z);
		glUniform3f(glGetUniformLocation(programID, "materialSpecular"), materialSpecular.x, materialSpecular.y, materialSpecular.z);
		glUniform1f(glGetUniformLocation(programID, "materialShininess"), materialShininess);
		glUniform3f(glGetUniformLocation(programID, "viewPosition"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
		glBindVertexArray(VertexArrayId[0]);
		glUniform1i(glGetUniformLocation(programID, "useLighting"), true);
		glDrawArrays(GL_LINES, 0, NumVerts[0]);
		glBindVertexArray(0);
		glDrawArrays(GL_LINES, 0, NumVerts[0]);
		glBindVertexArray(0);
		// draw face
		if (showTexture) {
			glUniform1i(glGetUniformLocation(programID, "useTexture"), 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glUniform1i(glGetUniformLocation(programID, "texture1"), 0);
			glBindVertexArray(VertexArrayId[faceTextObjectID]);
			glDrawElements(GL_TRIANGLES, NumIdcs[faceTextObjectID], GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
		}
		//if (showSubdivided) {
		// glUniform1i(glGetUniformLocation(programID, "useLighting"), true);
		// glUniform1i(glGetUniformLocation(programID, "useTexture"), 0);
		// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// glBindVertexArray(VertexArrayId[faceObjectID]);
		// glDrawElements(GL_TRIANGLES, NumIdcs[faceObjectID], GL_UNSIGNED_SHORT, 0);
		// glBindVertexArray(0);
		// // Render original control net as wireframe
		// //glUniform1i(glGetUniformLocation(programID, "useLighting"), true);
		// //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
			// //glUniform3f(glGetUniformLocation(programID, "materialDiffuse"), 1.0f, 0.0f, 0.0f); // Set wireframe color
			// //glBindVertexArray(VertexArrayId[controlNetID]);
			// //glDrawElements(GL_TRIANGLES, NumIdcs[controlNetID], GL_UNSIGNED_SHORT, 0);
			// //glBindVertexArray(0);
			//}
		else {
			glUniform1i(glGetUniformLocation(programID, "useLighting"), true);
			glUniform1i(glGetUniformLocation(programID, "useTexture"), 0);
			glBindVertexArray(VertexArrayId[faceObjectID]);
			glDrawElements(GL_TRIANGLES, NumIdcs[faceObjectID], GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
		}
	}
	glUseProgram(0);
	// Draw GUI
	//TwDraw();
	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}
void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}
glm::vec3 computeFaceNormal(const glm::vec3& v1, const glm::vec3& v2,
	const glm::vec3& v3) {
	glm::vec3 edge1 = v2 - v1;
	glm::vec3 edge2 = v3 - v1;
	return glm::normalize(glm::cross(edge1, edge2));
}
void recomputeNormals(std::vector<Vertex>& vertices, const
	std::vector<Face>& faces) {
	for (auto& vertex : vertices) {
		vertex.Normal[0] = 0.0f;
		vertex.Normal[1] = 0.0f;
		vertex.Normal[2] = 0.0f;
	}
	for (const auto& face : faces) {
		glm::vec3 v1(vertices[face.v1].Position[0],
			vertices[face.v1].Position[1], vertices[face.v1].Position[2]);
		glm::vec3 v2(vertices[face.v2].Position[0],
			vertices[face.v2].Position[1], vertices[face.v2].Position[2]);
		glm::vec3 v3(vertices[face.v3].Position[0],
			vertices[face.v3].Position[1], vertices[face.v3].Position[2]);
		glm::vec3 normal = computeFaceNormal(v1, v2, v3);
		for (int i : {face.v1, face.v2, face.v3}) {
			vertices[i].Normal[0] += normal.x;
			vertices[i].Normal[1] += normal.y;
			vertices[i].Normal[2] += normal.z;
		}
	}
	for (auto& vertex : vertices) {
		glm::vec3 normal(vertex.Normal[0], vertex.Normal[1],
			vertex.Normal[2]);
		normal = glm::normalize(normal);
		vertex.Normal[0] = normal.x;
		vertex.Normal[1] = normal.y;
		vertex.Normal[2] = normal.z;
	}
}
// Alternative way of triggering functions on keyboard events
static void keyCallback(GLFWwindow* window, int key, int scancode, int
	action, int mods) {
	// ATTN: MODIFY AS APPROPRIATE
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key)
		{
		case GLFW_KEY_F: // toggle wireframe mode
			isWireframe = !isWireframe;
			if (isWireframe) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			break;
		case GLFW_KEY_S: {
			std::map<Edge, std::vector<int>> adjacentTriangles;
			for (size_t i = 0; i < faces.size(); ++i) {
				const auto& face = faces[i];
				adjacentTriangles[Edge(face.v1,
					face.v2)].push_back(static_cast<int>(i));
				adjacentTriangles[Edge(face.v2,
					face.v3)].push_back(static_cast<int>(i));
				adjacentTriangles[Edge(face.v3,
					face.v1)].push_back(static_cast<int>(i));
			}
			std::map<Edge, glm::vec3> edgePoints =
				computeEdgePoints(vertices, faces, adjacentTriangles);
			auto updatedVertices = computeVertexPoints(vertices,
				edgePoints, adjacentTriangles);
			std::vector<Vertex> newVertices;
			std::vector<Face> newFaces;
			for (const auto& vertex : updatedVertices) {
				Vertex updatedVertex = {};
				updatedVertex.SetPosition(new float[3] { vertex.x,
					vertex.y, vertex.z });
				newVertices.push_back(updatedVertex);
			}
			std::map<Edge, int> edgePointIndices;
			int index = vertices.size();
			addEdgePointsToVertices(edgePoints, newVertices,
				edgePointIndices, index);
			for (const auto& face : faces) {
				int e1 = edgePointIndices[Edge(face.v1, face.v2)];
				int e2 = edgePointIndices[Edge(face.v2, face.v3)];
				int e3 = edgePointIndices[Edge(face.v3, face.v1)];
				assert(e1 != e2 && e2 != e3 && e3 != e1);
				newFaces.push_back({ face.v1, e1, e3 });
				newFaces.push_back({ e1, face.v2, e2 });
				newFaces.push_back({ e3, e2, face.v3 });
				newFaces.push_back({ e1, e2, e3 });
			}
			vertices = newVertices;
			faces = newFaces;
			for (auto& face : faces) {
				glm::vec3 v0(vertices[face.v1].Position[0],
					vertices[face.v1].Position[1], vertices[face.v1].Position[2]);
				glm::vec3 v1(vertices[face.v2].Position[0],
					vertices[face.v2].Position[1], vertices[face.v2].Position[2]);
				glm::vec3 v2(vertices[face.v3].Position[0],
					vertices[face.v3].Position[1], vertices[face.v3].Position[2]);
				glm::vec3 normal = glm::normalize(glm::cross(v1 -
					v0, v2 - v0));
				vertices[face.v1].Normal[0] += normal.x;
				vertices[face.v1].Normal[1] += normal.y;
				vertices[face.v1].Normal[2] += normal.z;
				vertices[face.v2].Normal[0] += normal.x;
				vertices[face.v2].Normal[1] += normal.y;
				vertices[face.v2].Normal[2] += normal.z;
				vertices[face.v3].Normal[0] += normal.x;
				vertices[face.v3].Normal[1] += normal.y;
				vertices[face.v3].Normal[2] += normal.z;
			}
			for (auto& vertex : vertices) {
				glm::vec3 normal(vertex.Normal[0],
					vertex.Normal[1], vertex.Normal[2]);
				normal = glm::normalize(normal);
				vertex.Normal[0] = normal.x;
				vertex.Normal[1] = normal.y;
				vertex.Normal[2] = normal.z;
			}
			glDeleteBuffers(1, &VertexBufferId[faceObjectID]);
			glDeleteBuffers(1, &IndexBufferId[faceObjectID]);
			glDeleteVertexArrays(1, &VertexArrayId[faceObjectID]);
			std::vector<GLushort> indices;
			for (const auto& face : faces) {
				indices.push_back(face.v1);
				indices.push_back(face.v2);
				indices.push_back(face.v3);
			}
			VertexBufferSize[faceObjectID] = sizeof(Vertex) *
				vertices.size();
			IndexBufferSize[faceObjectID] = sizeof(GLushort) *
				indices.size();
			NumIdcs[faceObjectID] = indices.size();
			glGenVertexArrays(1, &VertexArrayId[faceObjectID]);
			glBindVertexArray(VertexArrayId[faceObjectID]);
			glGenBuffers(1, &VertexBufferId[faceObjectID]);
			glBindBuffer(GL_ARRAY_BUFFER,
				VertexBufferId[faceObjectID]);
			glBufferData(GL_ARRAY_BUFFER,
				VertexBufferSize[faceObjectID], vertices.data(), GL_STATIC_DRAW);
			glGenBuffers(1, &IndexBufferId[faceObjectID]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
				IndexBufferId[faceObjectID]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				IndexBufferSize[faceObjectID], indices.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
				sizeof(Vertex), (void*)0); // Position
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
				sizeof(Vertex), (void*)sizeof(Vertex::Position)); // Color
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
				sizeof(Vertex),
				(void*)(sizeof(Vertex::Position) +
					sizeof(Vertex::Color))); // Normal
			glEnableVertexAttribArray(2);
			glBindVertexArray(0);
			showSubdivided = true;
			break;
		}
		case GLFW_KEY_T: // toggle texture
			showTexture = !showTexture;
			break;
		case GLFW_KEY_LEFT:
			horizAngle -= cameraSpeed;
			break;
		case GLFW_KEY_RIGHT:
			horizAngle += cameraSpeed;
			break;
		case GLFW_KEY_UP:
			if (vertAngle < glm::radians(89.0f)) vertAngle +=
				cameraSpeed;
			break;
		case GLFW_KEY_DOWN:
			if (vertAngle > glm::radians(-89.0f)) vertAngle -=
				cameraSpeed;
			break;
		default:
			break;
		}
		updateCamera();
	}
}
// Alternative way of triggering functions on mouse click events
static void mouseCallback(GLFWwindow* window, int button, int action, int
	mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action ==
		GLFW_PRESS) {
		pickObject();
	}
}
int main(void) {
	// TL
	// ATTN: Refer to https://learnopengl.com/Getting-started/Transformations, https://learnopengl.com/Getting-started/Coordinate-Systems,
	// and https://learnopengl.com/Getting-started/Camera to familiarize yourself with implementing the camera movement
	// ATTN (Project 3 only): Refer to https://learnopengl.com/Gettingstarted/ Textures to familiarize yourself with mapping a texture
	// to a given mesh
	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;
	// Initialize OpenGL pipeline
	initOpenGL();
	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1sec ago
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}
		// DRAWING POINTS
		renderScene();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);
	cleanup();
	return 0;
}