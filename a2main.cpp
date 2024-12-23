/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2021
 *
 *  Project: lab1
 *  File: main.cpp
 *
 *  Description:
 *      Gives the starting point to draw a ground plane with a 
 *		pass through color shader.  Also renders text to the screen
 *
 *  Author:
 *      Dr. Jeffrey Paone, Colorado School of Mines
 *  
 *  Notes:
 *
 */

//**********************************************************************************************************************

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdlib>
#include <cstdio>

#include <deque>
#include <string>
#include <vector>
#include <fstream>

#include <CSCI441/OpenGLUtils.hpp>
#include <CSCI441/ShaderProgram.hpp>
#include <CSCI441/ShaderProgramPipeline.hpp>

#include <unordered_map>
#include <string>

//**********************************************************************************************************************
// Structure definitions

struct Vertex {
	GLfloat px, py, pz;	// point location x,y,z
	GLfloat r, g, b;	// color r,g,b
};

struct CharacterInfo {
  GLfloat advanceX; // advance.x
  GLfloat advanceY; // advance.y

  GLfloat bitmapWidth; // bitmap.width;
  GLfloat bitmapHeight; // bitmap.rows;

  GLfloat bitmapLeft; // bitmap_left;
  GLfloat bitmapTop; // bitmap_top;

  GLfloat texCoordOffsetX; // x offset of glyph in texture coordinates
} fontCharacters[128];

//**********************************************************************************************************************
// Global Parameters

// fix our window to a specific size
const GLint WINDOW_WIDTH = 640, WINDOW_HEIGHT = 640;
GLint windowWidth, windowHeight;

// keep track of our mouse information
GLboolean isShiftDown;                          // if the control key was pressed when the mouse was pressed
GLboolean isLeftMouseDown;                      // if the mouse left button is currently pressed
glm::vec2 mousePosition;                        // current moust vPos

bool showWireframe = true;
bool perlinActive = false;

// keep track of all our camera information
struct CameraParameters {
    glm::vec3 cameraAngles;                     // cameraAngles --> x = thera, y = phi, z = radius
    glm::vec3 camDir;                           // direction to the camera
    glm::vec3 eyePoint;                         // camera vPos
    glm::vec3 lookAtPoint;                      // location of our object of interest
    glm::vec3 upVector;                         // the upVector of our camera
} arcballCam;

CSCI441::ShaderProgramPipeline *wireframeShaderProgram = nullptr;
CSCI441::ShaderProgramPipeline *tessShaderProgram = nullptr;
CSCI441::ShaderProgramPipeline *controlShaderProgram = nullptr;
CSCI441::ShaderProgramPipeline *noiseShaderProgram = nullptr;


struct WorldInfo {
	GLuint worldInfoUbod;
	GLubyte *worldInfoBuffer = NULL;
	GLint mvpMtx;
	GLint time;
	GLint cameraPos;
	GLint size;
};

struct ControlProgram {
  WorldInfo worldInfo;
  CSCI441::ShaderProgram *controlProgram;
} controlProgram;

struct Lights {
	GLubyte* lightsBuffer;
	GLuint lightsInfoUbod;
	GLint position;
	GLint ambient;
	GLint diffuse;
	GLint specular;
	GLint size;
};

struct PermutationTable {
	GLuint permutationTableUbod;
	GLubyte *permutationTableBuffer = NULL;
	GLint permutations;
	GLint size;
};

struct JustGround {
  WorldInfo worldInfo;
  CSCI441::ShaderProgram *justGroundProgram;
  GLint modelView;
  GLint normal;
} justGroundProgram;

struct ControlFirst {
  WorldInfo worldInfo;
  CSCI441::ShaderProgram *controlFirst;
  GLint modelView;
  GLint normal;
} controlFirstHalfProgram;

struct PhongProgram {
  WorldInfo worldInfo;
  Lights lights;
  CSCI441::ShaderProgram *phongProgram;
  GLint viewportMtx;
  GLint width;
  GLint color;
  GLint showWireframe;
  GLint modelViewMatrix;
} phongProgram;

struct NoiseProgram {
  WorldInfo worldInfo;
  Lights lights;
  PermutationTable permutationTable;
  CSCI441::ShaderProgram *noiseProgram;
  GLint viewportMtx;
  GLint width;
  GLint color;
  GLint showWireframe;
  GLint modelViewMatrix;
} noiseProgram;

/* CSCI441::ShaderProgram **shaderProgram = &wireframeShaderProgram; */
const char* gText = "Wireframe and control points visible";
const char* pText = "Wireframe and control points invisible";
const char* activeText = pText;

const char* bpText = "Perlin noise active";
const char* piText = "Phong shading active";
const char* activeIText = piText;

// all drawing information
const GLuint NUM_VAOS = 2;
const struct VAOIDs {
    const GLuint CUBE = 0;
    const GLuint GROUND = 1;
} VAOS;
struct IBOCounts {
    GLuint cube;                                // number of vertices that make up the cube
    GLuint ground;                              // number of vertices thaat make up the ground
} iboCounts;
GLuint vaods[NUM_VAOS];                         // an array of our VAO descriptors
GLuint vbods[NUM_VAOS];                         // an array of our VBO descriptors
GLuint ibods[NUM_VAOS];                         // an array of our IBO descriptors

// FPS information
GLdouble currentTime, lastTime;                 // used to compute elapsed time
GLuint nbFrames = 0;                            // number of frames rendered
const GLdouble FPS_SPAN = 0.33;                 // frequency to measure FPS
const GLdouble FPS_WINDOW = 5.0f;               // length of time to average FPS over
const GLuint FPS_COUNT = ceil(FPS_WINDOW / FPS_SPAN);
std::deque<GLdouble> fpsAvgs;                   // store previous FPS calculations

// Font information
GLuint fontTexture, fontVAO, fontVBO;           // stores the font texture and VAO for rendering
GLint atlasWidth, atlasHeight;                  // size of all characters in a row

CSCI441::ShaderProgram *textShaderProgram = nullptr;

struct TextShaderUniforms {
	GLint tex;                                  // Texture Map for font to apply
	GLint color;                                // Color to apply to text
} textShaderUniforms;

struct TextShaderAttributeLocations {
	GLint coord;                                // coordiante represented as (x, y, s, t)
} textShaderAttribLocs;

//**********************************************************************************************************************
// Helper Funcs

// updateCameraDirection() /////////////////////////////////////////////////////
/// \desc
/// This function updates the camera's vPos in cartesian coordinates based
///  on its vPos in spherical coordinates. Should be called every time
///  cameraAngles is updated.
///
// /////////////////////////////////////////////////////////////////////////////
void updateCameraDirection() {
    // ensure the camera does not flip upside down at either pole
    if( arcballCam.cameraAngles.y < 0 )     arcballCam.cameraAngles.y = 0.0f + 0.001f;
    if( arcballCam.cameraAngles.y >= M_PI ) arcballCam.cameraAngles.y = M_PI - 0.001f;

    // do not let our camera get too close or too far away
    if( arcballCam.cameraAngles.z <= 2.0f )  arcballCam.cameraAngles.z = 2.0f;
    if( arcballCam.cameraAngles.z >= 35.0f ) arcballCam.cameraAngles.z = 35.0f;

    // update the new direction to the camera
    arcballCam.camDir.x =  sinf( arcballCam.cameraAngles.x ) * sinf( arcballCam.cameraAngles.y );
    arcballCam.camDir.y = -cosf( arcballCam.cameraAngles.y );
    arcballCam.camDir.z = -cosf( arcballCam.cameraAngles.x ) * sinf( arcballCam.cameraAngles.y );

    // normalize this direction
    arcballCam.camDir = glm::normalize(arcballCam.camDir);
}

// calculateFPS() //////////////////////////////////////////////////////////////
/// \desc
/// This function queries the current time, increments the number of frames
///     rendered, and measures if the target time span elapsed.  If yes, then
///     calculates the Frames Per Second value and adds it to the averages
///     array.
///
// /////////////////////////////////////////////////////////////////////////////
void calculateFPS() {
    currentTime = glfwGetTime();            // query the current time
    nbFrames++;                             // add one to the number of frames rendered

    // measure if the target amount of time has elapsed
    if ( currentTime - lastTime >= FPS_SPAN ) {
        // calculate the FPS over the corresponding time span
        GLdouble currFPS = GLdouble(nbFrames)/(currentTime - lastTime);
        // add this value to the array of prior FPS values
        fpsAvgs.emplace_back( currFPS );
        // only store the last FPS_COUNT worth of values to compute average
        if(fpsAvgs.size() > FPS_COUNT) fpsAvgs.pop_front();

        // reset our FPS counters
        lastTime = currentTime;
        nbFrames = 0;
    }
}

//**********************************************************************************************************************
// GLFW Event Callbacks

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's error callback.
///	When an error within GLFW occurs, GLFW will tell us by calling
///	this function.  We can then print this info to the terminal to
///	alert the user.
///
// /////////////////////////////////////////////////////////////////////////////
static void error_callback(int error, const char* description) {
	fprintf(stderr, "[ERROR]: %s\n", description);
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's keypress callback.
///	Responds to key presses and key releases
///
// /////////////////////////////////////////////////////////////////////////////
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;

            case GLFW_KEY_W:
            		showWireframe = !showWireframe;
                activeText = showWireframe ? gText : pText;
            break;
            case GLFW_KEY_P:
            		perlinActive = !perlinActive;
            		activeIText = perlinActive ? bpText : piText;
            break;
            default:
                break;
        }
    }
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's mouse button callback.
///	Responds to mouse button presses and mouse button releases.  Keeps track if
///	the control key was pressed when a left mouse click occurs to allow
///	zooming of our arcball camera.
///
// /////////////////////////////////////////////////////////////////////////////
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if( button == GLFW_MOUSE_BUTTON_LEFT ) {
        if( action == GLFW_PRESS ) {
            isLeftMouseDown = GL_TRUE;
            isShiftDown = (mods & GLFW_MOD_SHIFT);
        } else {
            isLeftMouseDown = GL_FALSE;
            isShiftDown = GL_FALSE;
            mousePosition = glm::vec2(-9999.0f, -9999.0f);
        }
    }
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's cursor movement callback.
///	Responds to mouse movement.  When active motion is used with the left
///	mouse button an arcball camera model is followed.
///
// /////////////////////////////////////////////////////////////////////////////
static void cursor_callback( GLFWwindow* window, double xPos, double yPos ) {
    // make sure movement is in bounds of the window
    // glfw captures mouse movement on entire screen
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if( xPos > 0 && xPos < width ) {
        if( yPos > 0 && yPos < height ) {
            // active motion
            if( isLeftMouseDown ) {
                if( (mousePosition.x - -9999.0f) > 0.001f ) {
                    if( !isShiftDown ) {
                        // if shift is not held down, update our camera angles theta & phi
                        arcballCam.cameraAngles.x += (xPos - mousePosition.x) * 0.005f;
                        arcballCam.cameraAngles.y += (mousePosition.y - yPos) * 0.005f;
                    } else {
                        // otherwise shift was held down, update our camera radius
                        double totChgSq = (xPos - mousePosition.x) + (yPos - mousePosition.y);
                        arcballCam.cameraAngles.z += totChgSq*0.01f;
                    }
                    // recompute our camera direction
                    updateCameraDirection();
                }
                // update the last mouse vPos
                mousePosition = glm::vec2(xPos, yPos);
            }
            // passive motion
            else {

            }
        }
    }
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's scroll wheel callback.
///	Responds to movement of the scroll where.  Allows zooming of the arcball
///	camera.
///
// /////////////////////////////////////////////////////////////////////////////
static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset ) {
    double totChgSq = yOffset;
    arcballCam.cameraAngles.z += totChgSq*0.2f;
    updateCameraDirection();
}

//**********************************************************************************************************************
// Setup Funcs

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		Used to setup everything GLFW related.  This includes the OpenGL context
///	and our window.
///
// /////////////////////////////////////////////////////////////////////////////
GLFWwindow* setupGLFW() {
    // set what function to use when registering errors
    // this is the ONLY GLFW function that can be called BEFORE GLFW is initialized
    // all other GLFW calls must be performed after GLFW has been initialized
    glfwSetErrorCallback(error_callback);

    // initialize GLFW
    if (!glfwInit()) {
        fprintf( stderr, "[ERROR]: Could not initialize GLFW\n" );
        exit(EXIT_FAILURE);
    } else {
        fprintf( stdout, "[INFO]: GLFW initialized\n" );
    }

    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );						// request forward compatible OpenGL context
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );	        // request OpenGL Core Profile context
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );		                // request OpenGL 4.X context
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );		                // request OpenGL X.1 context
    glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );                             // request double buffering
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );                               // do not allow the window to be resized

    // create a window for a given size, with a given title
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab2: Shader Subroutines", nullptr, nullptr );
    if( !window ) {						                                        // if the window could not be created, NULL is returned
        fprintf( stderr, "[ERROR]: GLFW Window could not be created\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    } else {
        fprintf( stdout, "[INFO]: GLFW Window created\n" );
    }

    glfwMakeContextCurrent(	window );	                                        // make the created window the current window
    glfwSwapInterval( 1 );				                                // update our screen after at least 1 screen refresh

    glfwSetKeyCallback(         window, key_callback		  );            	// set our keyboard callback function
    glfwSetMouseButtonCallback( window, mouse_button_callback );	            // set our mouse button callback function
    glfwSetCursorPosCallback(	window, cursor_callback  	  );	            // set our cursor vPos callback function
    glfwSetScrollCallback(		window, scroll_callback		  );	            // set our scroll wheel callback function

    return window;										                        // return the window that was created
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Used to setup everything OpenGL related.
///
// /////////////////////////////////////////////////////////////////////////////
void setupOpenGL() {
    glEnable( GL_DEPTH_TEST );					                    // enable depth testing
    glDepthFunc( GL_LESS );							                // use less than depth test

    glFrontFace( GL_CCW );                                          // front faces are counter clockwise

    glEnable( GL_BLEND );									        // enable blending
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	        // use one minus blending equation

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// clear the frame buffer to black

    glPatchParameteri(GL_PATCH_VERTICES, 16);
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Used to initialize GLEW
///
// /////////////////////////////////////////////////////////////////////////////
void setupGLEW() {
    glewExperimental = GL_TRUE;
    GLenum glewResult = glewInit();

    // check for an error
    if( glewResult != GLEW_OK ) {
        fprintf( stderr, "[ERROR]: Error initializing GLEW\n");
        fprintf( stderr, "[ERROR]: %s\n", glewGetErrorString(glewResult) );
        exit(EXIT_FAILURE);
    } else {
        fprintf( stdout, "\n[INFO]: GLEW initialized\n" );
        fprintf( stdout, "[INFO]: Using GLEW %s\n", glewGetString(GLEW_VERSION) );
    }
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Registers our Shader Programs and query locations
///          of uniform/attribute inputs
///
// /////////////////////////////////////////////////////////////////////////////
void setupShaders() {
	// Create the shader programs
	const char* justGround[1] = {"shaders/wireframe.v.glsl"};
	justGroundProgram.justGroundProgram = new CSCI441::ShaderProgram(justGround, true, false, false, false, true);
	const char* controlFirstHalf[3] = {"shaders/wireframe.v.glsl", "shaders/bezierPatch.tc.glsl", "shaders/bezierPatch.te.glsl"};
	controlFirstHalfProgram.controlFirst = new CSCI441::ShaderProgram(controlFirstHalf, true, true, false, false, true);
	const char* phongShaders[2] = {"shaders/wireframe.g.glsl", "shaders/wireframe.f.glsl"};
	phongProgram.phongProgram = new CSCI441::ShaderProgram(phongShaders, false, false, true, true, true);
	const char* noiseShaders[2] = {"shaders/wireframe.g.glsl", "shaders/procedural.f.glsl"};
	noiseProgram.noiseProgram = new CSCI441::ShaderProgram(noiseShaders, false, false, true, true, true);
	const char* controlShaders[3] = { "shaders/control.v.glsl", "shaders/control.g.glsl", "shaders/control.f.glsl" };
	controlProgram.controlProgram = new CSCI441::ShaderProgram(controlShaders, true, false, true, true, true);

	// Create the shader pipeline programs
	tessShaderProgram        		    = new CSCI441::ShaderProgramPipeline();
	tessShaderProgram->useProgramStages(GL_VERTEX_SHADER_BIT | GL_TESS_CONTROL_SHADER_BIT | GL_TESS_EVALUATION_SHADER_BIT, controlFirstHalfProgram.controlFirst);
	tessShaderProgram->useProgramStages(GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, phongProgram.phongProgram);

	wireframeShaderProgram        		    = new CSCI441::ShaderProgramPipeline();
	wireframeShaderProgram->useProgramStages(GL_VERTEX_SHADER_BIT, justGroundProgram.justGroundProgram);
	wireframeShaderProgram->useProgramStages(GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, phongProgram.phongProgram);

  controlShaderProgram = new CSCI441::ShaderProgramPipeline();
  controlShaderProgram->useProgramStages(GL_VERTEX_SHADER_BIT | GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, controlProgram.controlProgram);

  noiseShaderProgram = new CSCI441::ShaderProgramPipeline();
	noiseShaderProgram->useProgramStages(GL_VERTEX_SHADER_BIT | GL_TESS_CONTROL_SHADER_BIT | GL_TESS_EVALUATION_SHADER_BIT, controlFirstHalfProgram.controlFirst);
	noiseShaderProgram->useProgramStages(GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, noiseProgram.noiseProgram);


	// Setup a ton of uniforms
	justGroundProgram.worldInfo.worldInfoBuffer     = justGroundProgram.justGroundProgram->getUniformBlockBuffer( "WorldInfo");
	justGroundProgram.worldInfo.size = justGroundProgram.justGroundProgram->getUniformBlockSize("WorldInfo");
	const GLchar* names[3] = {"mvpMtx", "time", "cameraPos"};
	justGroundProgram.worldInfo.mvpMtx          = justGroundProgram.justGroundProgram->getUniformBlockOffsets("WorldInfo", names)[0];
	justGroundProgram.worldInfo.time          = justGroundProgram.justGroundProgram->getUniformBlockOffsets("WorldInfo", names)[1];
	justGroundProgram.worldInfo.cameraPos          = justGroundProgram.justGroundProgram->getUniformBlockOffsets("WorldInfo", names)[2];
	justGroundProgram.modelView          = justGroundProgram.justGroundProgram->getUniformLocation("ModelViewMatrix");
	justGroundProgram.normal          = justGroundProgram.justGroundProgram->getUniformLocation("NormalMatrix");
	glGenBuffers(1, &justGroundProgram.worldInfo.worldInfoUbod);


	controlFirstHalfProgram.worldInfo.worldInfoBuffer     = controlFirstHalfProgram.controlFirst->getUniformBlockBuffer( "WorldInfo");
	controlFirstHalfProgram.worldInfo.size = controlFirstHalfProgram.controlFirst->getUniformBlockSize("WorldInfo");
	controlFirstHalfProgram.worldInfo.mvpMtx          = controlFirstHalfProgram.controlFirst->getUniformBlockOffsets("WorldInfo", names)[0];
	controlFirstHalfProgram.worldInfo.time          = controlFirstHalfProgram.controlFirst->getUniformBlockOffsets("WorldInfo", names)[1];
	controlFirstHalfProgram.worldInfo.cameraPos          = controlFirstHalfProgram.controlFirst->getUniformBlockOffsets("WorldInfo", names)[2];
	controlFirstHalfProgram.modelView          = controlFirstHalfProgram.controlFirst->getUniformLocation("ModelViewMatrix");
	controlFirstHalfProgram.normal          = controlFirstHalfProgram.controlFirst->getUniformLocation("NormalMatrix");
	glGenBuffers(1, &controlFirstHalfProgram.worldInfo.worldInfoUbod);


	phongProgram.lights.lightsBuffer     = phongProgram.phongProgram->getUniformBlockBuffer( "Lights");
	phongProgram.lights.size = phongProgram.phongProgram->getUniformBlockSize("Lights");
	const GLchar* names_l[4] = {"position", "ambient", "diffuse", "specular"};
	phongProgram.lights.position          = phongProgram.phongProgram->getUniformBlockOffsets("Lights", names_l)[0];
	phongProgram.lights.ambient          = phongProgram.phongProgram->getUniformBlockOffsets("Lights", names_l)[1];
	phongProgram.lights.diffuse          = phongProgram.phongProgram->getUniformBlockOffsets("Lights", names_l)[2];
	phongProgram.lights.specular          = phongProgram.phongProgram->getUniformBlockOffsets("Lights", names_l)[3];
	glGenBuffers(1, &phongProgram.lights.lightsInfoUbod);

	phongProgram.viewportMtx          = phongProgram.phongProgram->getUniformLocation("ViewportMatrix");
	phongProgram.width          = phongProgram.phongProgram->getUniformLocation("Width");
	phongProgram.color          = phongProgram.phongProgram->getUniformLocation("Color");
	phongProgram.showWireframe          = phongProgram.phongProgram->getUniformLocation("showWireframe");
	phongProgram.modelViewMatrix          = phongProgram.phongProgram->getUniformLocation("ModelViewMatrix");

	phongProgram.worldInfo.worldInfoBuffer     = phongProgram.phongProgram->getUniformBlockBuffer("WorldInfo");
	phongProgram.worldInfo.size = phongProgram.phongProgram->getUniformBlockSize("WorldInfo");
	phongProgram.worldInfo.mvpMtx          = phongProgram.phongProgram->getUniformBlockOffsets("WorldInfo", names)[0];
	phongProgram.worldInfo.time          = phongProgram.phongProgram->getUniformBlockOffsets("WorldInfo", names)[1];
	phongProgram.worldInfo.cameraPos          = phongProgram.phongProgram->getUniformBlockOffsets("WorldInfo", names)[2];
	glGenBuffers(1, &phongProgram.worldInfo.worldInfoUbod);


	noiseProgram.viewportMtx          = noiseProgram.noiseProgram->getUniformLocation("ViewportMatrix");
	noiseProgram.width          = noiseProgram.noiseProgram->getUniformLocation("Width");
	noiseProgram.color          = noiseProgram.noiseProgram->getUniformLocation("Color");
	noiseProgram.showWireframe          = noiseProgram.noiseProgram->getUniformLocation("showWireframe");
	noiseProgram.modelViewMatrix          = noiseProgram.noiseProgram->getUniformLocation("ModelViewMatrix");

	noiseProgram.lights.lightsBuffer     = noiseProgram.noiseProgram->getUniformBlockBuffer( "Lights");
	noiseProgram.lights.size = noiseProgram.noiseProgram->getUniformBlockSize("Lights");
	noiseProgram.lights.position          = noiseProgram.noiseProgram->getUniformBlockOffsets("Lights", names_l)[0];
	noiseProgram.lights.ambient          = noiseProgram.noiseProgram->getUniformBlockOffsets("Lights", names_l)[1];
	noiseProgram.lights.diffuse          = noiseProgram.noiseProgram->getUniformBlockOffsets("Lights", names_l)[2];
	noiseProgram.lights.specular          = noiseProgram.noiseProgram->getUniformBlockOffsets("Lights", names_l)[3];
	glGenBuffers(1, &noiseProgram.lights.lightsInfoUbod);

	noiseProgram.worldInfo.worldInfoBuffer     = noiseProgram.noiseProgram->getUniformBlockBuffer("WorldInfo");
	noiseProgram.worldInfo.size = noiseProgram.noiseProgram->getUniformBlockSize("WorldInfo");
	noiseProgram.worldInfo.mvpMtx          = noiseProgram.noiseProgram->getUniformBlockOffsets("WorldInfo", names)[0];
	noiseProgram.worldInfo.time          = noiseProgram.noiseProgram->getUniformBlockOffsets("WorldInfo", names)[1];
	noiseProgram.worldInfo.cameraPos          = noiseProgram.noiseProgram->getUniformBlockOffsets("WorldInfo", names)[2];
	glGenBuffers(1, &noiseProgram.worldInfo.worldInfoUbod);

	noiseProgram.permutationTable.permutationTableBuffer     = noiseProgram.noiseProgram->getUniformBlockBuffer("PermutationTable");
	noiseProgram.permutationTable.size     = noiseProgram.noiseProgram->getUniformBlockSize("PermutationTable");
	const GLchar* names_p[1] = {"permutations"};
	noiseProgram.permutationTable.permutations          = noiseProgram.noiseProgram->getUniformBlockOffsets("PermutationTable", names_p)[0];
	glGenBuffers(1, &noiseProgram.permutationTable.permutationTableUbod);

	controlProgram.worldInfo.worldInfoBuffer     = controlProgram.controlProgram->getUniformBlockBuffer("WorldInfo");
	controlProgram.worldInfo.size = controlProgram.controlProgram->getUniformBlockSize("WorldInfo");
	controlProgram.worldInfo.mvpMtx          = controlProgram.controlProgram->getUniformBlockOffsets("WorldInfo", names)[0];
	controlProgram.worldInfo.time          = controlProgram.controlProgram->getUniformBlockOffsets("WorldInfo", names)[1];
	controlProgram.worldInfo.cameraPos          = controlProgram.controlProgram->getUniformBlockOffsets("WorldInfo", names)[2];
	glGenBuffers(1, &controlProgram.worldInfo.worldInfoUbod);


    // load our text shader program
	textShaderProgram                   = new CSCI441::ShaderProgram( "shaders/freetypeColoredText.v.glsl","shaders/freetypeColoredText.f.glsl" );
    // query all of our uniform locations
	textShaderUniforms.tex              = textShaderProgram->getUniformLocation( "tex" );
    textShaderUniforms.color            = textShaderProgram->getUniformLocation( "color" );
    // query all of our attribute locations
    textShaderAttribLocs.coord          = textShaderProgram->getAttributeLocation( "coord" );
    // set static uniform values
	textShaderProgram->useProgram();
    glUniform1i( textShaderUniforms.tex, 0 );              // use Texture0

    GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};            // use white text
    glUniform4fv( textShaderUniforms.color, 1, white );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Create our VAOs & VBOs. Send vertex assets to the GPU for future rendering
///
// /////////////////////////////////////////////////////////////////////////////
void setupBuffers() {
	// generate our vertex array object descriptors
	glGenVertexArrays( NUM_VAOS, vaods );
    // generate our vertex buffer object descriptors
    glGenBuffers( NUM_VAOS, vbods );
    // generate our index buffer object descriptors
    glGenBuffers( NUM_VAOS, ibods );


		// Load whatever is stored in the tess file (no file extension)
    std::ifstream f("tess");
    std::vector<GLushort> order;
    std::vector<Vertex> points;
    int num_orders;
    f >> num_orders;
    for (int i = 0; i < num_orders; i++) {
      int pts;
      f >> pts;
      order.push_back(pts - 1);
      for (int j = 1; j < 16; j++) {
        char tmp;
        f >> tmp;
        f >> pts;
        order.push_back(pts - 1);
      }
    }
    int num_points;
    f >> num_points;
    for (int i = 0; i < num_points; i++) {
      float pts[3];
      f >> pts[0];
      for (int j = 1; j < 3; j++) {
        char tmp;
        f >> tmp;
        f >> pts[j];
      }
      points.push_back(Vertex{pts[0], pts[1], pts[2], 0, 0, 0});
    }

    iboCounts.cube = order.size();

	// bind our Cube VAO
	glBindVertexArray( vaods[VAOS.CUBE] );

	// bind the VBO for our Cube Array Buffer
	glBindBuffer( GL_ARRAY_BUFFER, vbods[VAOS.CUBE] );
	// send the data to the GPU
	glBufferData( GL_ARRAY_BUFFER, points.size() * sizeof(Vertex), points.data(), GL_STATIC_DRAW );

	// bind the VBO for our Cube Element Array Buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibods[VAOS.CUBE] );
	// send the data to the GPU
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, order.size() * sizeof(GLushort), order.data(), GL_STATIC_DRAW );

	// enable our vPos attribute
	glEnableVertexAttribArray(0);
	// map the vPos attribute to data within our buffer
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0 );

	glEnableVertexAttribArray(1);
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(GLfloat) * 3));

	//------------ BEGIN ground VAO ------------

    // specify our Ground Vertex Information
    const int NUM_GROUND_VERTICES = 4;
    const Vertex groundVertices[NUM_GROUND_VERTICES] = {
            { -5.0f, -3.0f, -5.0f, 0.0f, 1.0f, 0.0f }, // 0 - BL
            {  5.0f, -3.0f, -5.0f, 0.0f, 1.0f, 0.0f }, // 1 - BR
            {  5.0f, -3.0f,  5.0f, 0.0f, 1.0f, 0.0f }, // 2 - TR
            { -5.0f, -3.0f,  5.0f, 0.0f, 1.0f, 0.0f }  // 3 - TL
    };
    // specify our Ground Index Ordering
    const GLushort groundIndices[] = {
            0, 2, 1, 	0, 3, 2
    };
    iboCounts.ground = 6;

	// Draw Ground
	glBindVertexArray( vaods[VAOS.GROUND] );

	// bind the VBO for our Ground Array Buffer
	glBindBuffer( GL_ARRAY_BUFFER, vbods[VAOS.GROUND] );
	// send the data to the GPU
	glBufferData( GL_ARRAY_BUFFER, NUM_GROUND_VERTICES * sizeof(Vertex), groundVertices, GL_STATIC_DRAW );

	// bind the VBO for our Ground Element Array Buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibods[VAOS.GROUND] );
	// send the data to the GPU
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, iboCounts.ground * sizeof(GLushort), groundIndices, GL_STATIC_DRAW );

	// enable our vPos attribute
	glEnableVertexAttribArray(0);
	// map the vPos attribute to data within our buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0 );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(GLfloat) * 3));


	//------------  END  ground VAO------------
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Load in a font face from a ttf file, store the glyphs in a VAO+VBO w/ texture
///
// /////////////////////////////////////////////////////////////////////////////
void setupFonts() {
	FT_Library ftLibrary;
    FT_Face fontFace;

	if(FT_Init_FreeType(&ftLibrary)) {
        fprintf(stderr, "[ERROR]: Could not init FreeType library\n");
        exit(EXIT_FAILURE);
	} else {
	    fprintf(stdout, "[INFO]: FreeType library initialized\n");
	}

    const std::string FONT_FILE_NAME = "assets/fonts/DroidSansMono.ttf";
	if(FT_New_Face( ftLibrary, FONT_FILE_NAME.c_str(), 0, &fontFace)) {
        fprintf(stderr, "[ERROR]: Could not open font %s\n", FONT_FILE_NAME.c_str());
        exit(EXIT_FAILURE);
	} else {
	    fprintf(stdout, "[INFO]; Successfully loaded font face \"%s\"\n", FONT_FILE_NAME.c_str());
	}

	FT_Set_Pixel_Sizes( fontFace, 0, 20);

	FT_GlyphSlot g = fontFace->glyph;
	GLuint w = 0;
	GLuint h = 0;

	for(int i = 32; i < 128; i++) {
        if(FT_Load_Char(fontFace, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "[ERROR]: Loading character %c failed!\n", i);
            continue;
        }

        w += g->bitmap.width;
        h = (h > g->bitmap.rows ? h : g->bitmap.rows);
	}

	// you might as well save this value as it is needed later on
	atlasWidth = w;
    atlasHeight = h;

    // create texture memory to store the glyphs for rendering
	glEnable( GL_TEXTURE_2D );
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &fontTexture);
	glBindTexture( GL_TEXTURE_2D, fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLint x = 0;

	for(int i = 32; i < 128; i++) {
        if(FT_Load_Char( fontFace, i, FT_LOAD_RENDER))
            continue;

        fontCharacters[i].advanceX = g->advance.x >> 6;
        fontCharacters[i].advanceY = g->advance.y >> 6;

        fontCharacters[i].bitmapWidth = g->bitmap.width;
        fontCharacters[i].bitmapHeight = g->bitmap.rows;

        fontCharacters[i].bitmapLeft = g->bitmap_left;
        fontCharacters[i].bitmapTop = g->bitmap_top;

        fontCharacters[i].texCoordOffsetX = (float)x / w;

        glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

        x += g->bitmap.width;
	}

    fprintf(stdout, "[INFO]: Font face texture atlas setup\n");

	// create the VAO + VBO to ultimately store the text to be displayed
	glGenVertexArrays( 1, &fontVAO );
	glBindVertexArray( fontVAO );
	glGenBuffers( 1, &fontVBO );
	glBindBuffer( GL_ARRAY_BUFFER, fontVBO);
	glEnableVertexAttribArray( textShaderAttribLocs.coord );
	glVertexAttribPointer( textShaderAttribLocs.coord, 4, GL_FLOAT, GL_FALSE, 0, (void*)0 );

	fprintf(stdout, "[INFO]: Font face buffer setup\n");

	FT_Done_Face(fontFace);
	FT_Done_FreeType( ftLibrary);
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Initialize all of our scene information here
///
// /////////////////////////////////////////////////////////////////////////////
void setupScene() {
    // set up mouse info
    isLeftMouseDown = GL_FALSE;
    isShiftDown = GL_FALSE;
    mousePosition = glm::vec2( -9999.0f, -9999.0f );

    // set up camera info
    arcballCam.cameraAngles   = glm::vec3( 1.82f, 2.01f, 15.0f );
    arcballCam.camDir         = glm::vec3( -1.0f, -1.0f, -1.0f );
    arcballCam.lookAtPoint    = glm::vec3( 0.0f, 0.0f, 0.0f) ;
    arcballCam.upVector       = glm::vec3( 0.0f,  1.0f,  0.0f );
    updateCameraDirection();

    // initialize FPS timers to be non-zero
    currentTime = lastTime = glfwGetTime();
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Create our OpenGL context,
///          load all information to the GPU,
///          initialize scene information
///
// /////////////////////////////////////////////////////////////////////////////
GLFWwindow* initialize() {
    // GLFW sets up our OpenGL context so must be done first
    GLFWwindow* window = setupGLFW();	                // initialize all of the GLFW specific information related to OpenGL and our window
    setupGLEW();										// initialize all of the GLEW specific information
    setupOpenGL();										// initialize all of the OpenGL specific information

    CSCI441::OpenGLUtils::printOpenGLInfo();            // print our OpenGL information

    setupShaders();                                     // load all of our shader programs onto the GPU and get shader input locations
    setupBuffers();										// load all our VAOs and VBOs onto the GPU
    setupFonts();                                       // load all our fonts as a VAO to the GPU
    setupScene();                                       // initialize all of our scene information

    fprintf( stdout, "\n[INFO]: Setup complete\n" );

    return window;
}

//**********************************************************************************************************************
// Cleanup Functions

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete shaders off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupShaders() {
    fprintf( stdout, "[INFO]: ...deleting shaders.\n" );

    delete wireframeShaderProgram;
    delete tessShaderProgram;
    delete textShaderProgram;
    delete noiseShaderProgram;
    delete controlShaderProgram;
    delete justGroundProgram.justGroundProgram;
    delete controlFirstHalfProgram.controlFirst;
    delete phongProgram.phongProgram;
    delete noiseProgram.noiseProgram;
    delete controlProgram.controlProgram;
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete VAOs and VBOs off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting IBOs....\n" );
    glDeleteBuffers( NUM_VAOS, ibods );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );
    glDeleteBuffers( NUM_VAOS, vbods );

    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );
    glDeleteVertexArrays( NUM_VAOS, vaods );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete Fonts off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupFonts() {
    fprintf( stdout, "[INFO]: ...deleting fonts...\n" );

    glDeleteBuffers( 1, &fontVBO );
    glDeleteVertexArrays( 1, &fontVAO );
    glDeleteTextures( 1, &fontTexture );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Free all memory on the CPU/GPU and close our OpenGL context
///
// /////////////////////////////////////////////////////////////////////////////
void shutdown(GLFWwindow* window) {
    fprintf( stdout, "\n[INFO]: Shutting down.......\n" );
    fprintf( stdout, "[INFO]: ...closing window...\n" );
    glfwDestroyWindow( window );                        // close our window
    cleanupShaders();                                   // delete shaders from GPU
    cleanupBuffers();                                   // delete VAOs/VBOs from GPU
    cleanupFonts();                                     // delete VAOs/VBOs/textures from GPU
    fprintf( stdout, "[INFO]: ...closing GLFW.....\n" );
    glfwTerminate();						            // shut down GLFW to clean up our context
    fprintf( stdout, "[INFO]: ..shut down complete!\n" );
}

//**********************************************************************************************************************
// Rendering / Drawing Functions - this is where the magic happens!

// /////////////////////////////////////////////////////////////////////////////
/// \desc
/// Displays a given text string, using the corresponding character map, starting
///     at a given (x, y) coordinate.  Each character is scaled by
///     (scaleX, scaleY).
///
// /////////////////////////////////////////////////////////////////////////////
void renderText( const char *text, CharacterInfo characters[], float x, float y, float scaleX, float scaleY ) {
	const GLuint TEXT_LENGTH = strlen(text);    // the number of characters in the text string
	const GLuint NUM_POINTS = 6 * TEXT_LENGTH;  // each character is drawn as a quad of two triangles
    glm::vec4 coords[NUM_POINTS];               // values correspond to vertex attributes x, y, s, t

	GLint n = 0;

	for(const char *p = text; *p; p++) {
		auto characterIndex = (int)*p;

		CharacterInfo character = characters[characterIndex];

		GLfloat characterXPos = x + character.bitmapLeft * scaleX;
		GLfloat characterYPos = -y - character.bitmapTop * scaleY;
		GLfloat scaledCharacterWidth = character.bitmapWidth * scaleX;
		GLfloat scaledCharacterHeight = character.bitmapHeight * scaleY;
		GLfloat glyphWidth = character.bitmapWidth / atlasWidth;
		GLfloat glyphHeight = character.bitmapHeight / atlasHeight;

		// Advance the cursor to the start of the next character
		x += character.advanceX * scaleX;
		y += character.advanceY * scaleY;

		// Skip glyphs that have no pixels
		if( !scaledCharacterWidth || !scaledCharacterHeight)
			continue;

		coords[n++] = glm::vec4( characterXPos, -characterYPos, character.texCoordOffsetX, 0);
		coords[n++] = glm::vec4( characterXPos + scaledCharacterWidth, -characterYPos    , character.texCoordOffsetX + glyphWidth, 0);
		coords[n++] = glm::vec4( characterXPos, -characterYPos - scaledCharacterHeight, character.texCoordOffsetX, glyphHeight); //remember: each glyph occupies a different amount of vertical space

		coords[n++] = glm::vec4( characterXPos + scaledCharacterWidth, -characterYPos    , character.texCoordOffsetX + glyphWidth, 0);
		coords[n++] = glm::vec4( characterXPos, -characterYPos - scaledCharacterHeight, character.texCoordOffsetX, glyphHeight);
		coords[n++] = glm::vec4( characterXPos + scaledCharacterWidth, -characterYPos - scaledCharacterHeight, character.texCoordOffsetX + glyphWidth, glyphHeight);
	}

	glBindVertexArray(fontVAO);
	glBindBuffer(GL_ARRAY_BUFFER, fontVBO);
	glBufferData(GL_ARRAY_BUFFER, NUM_POINTS * sizeof( glm::vec4 ), coords, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, n);
}

void setupPermutationTable(PermutationTable *permutationTable, CSCI441::ShaderProgram *prog, const glm::mat4 VIEW_MTX, const glm::mat4 PROJ_MTX ) {
  const int p[512] = {151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};
  glm::ivec4 pTable[512];
  for(int i = 0; i < 512; i++) {
    pTable[i] = glm::ivec4(p[i], 0, 0, 0);
  }

#ifdef __APPLE__
  memcpy(permutationTable->permutationTableBuffer + permutationTable->permutations, &pTable[0], 512*sizeof(glm::ivec4));
#else
  memcpy(permutationTable->permutationTableBuffer + permutationTable->permutations, &p[0], 512*sizeof(int));
#endif
  glBindBuffer(GL_UNIFORM_BUFFER, permutationTable->permutationTableUbod);
  glBufferData(GL_UNIFORM_BUFFER, permutationTable->size, permutationTable->permutationTableBuffer, GL_DYNAMIC_DRAW);
  prog->setUniformBlockBinding("PermutationTable", 2);
  glBindBufferBase(GL_UNIFORM_BUFFER, 2, permutationTable->permutationTableUbod);
}

void setupWorldInfo(WorldInfo *worldInfo, CSCI441::ShaderProgram *prog, const glm::mat4 VIEW_MTX, const glm::mat4 PROJ_MTX ) {
  glm::mat4 modelMtx = glm::mat4( 1.0f);

  // precompute the modelViewProjection matrix
  glm::mat4 mvpMtx = PROJ_MTX * VIEW_MTX * modelMtx;
  glm::mat4 mvMtx = VIEW_MTX * modelMtx;
  GLfloat time = glfwGetTime();

	memcpy(worldInfo->worldInfoBuffer + worldInfo->mvpMtx, &mvpMtx[0][0], sizeof(GLfloat) * 4 * 4);
	memcpy(worldInfo->worldInfoBuffer + worldInfo->time, &time, sizeof(GLfloat));
  glm::vec3 camera_at = arcballCam.lookAtPoint + arcballCam.camDir * arcballCam.cameraAngles.z;
	memcpy(worldInfo->worldInfoBuffer + worldInfo->cameraPos, &camera_at[0], sizeof(GLfloat) * 3);
	glBindBuffer(GL_UNIFORM_BUFFER, worldInfo->worldInfoUbod);
	glBufferData(GL_UNIFORM_BUFFER, worldInfo->size, worldInfo->worldInfoBuffer, GL_DYNAMIC_DRAW);
	prog->setUniformBlockBinding("WorldInfo", 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfo->worldInfoUbod);
}

void setupLights(Lights *lights, CSCI441::ShaderProgram *prog, const glm::mat4 VIEW_MTX, const glm::mat4 PROJ_MTX) {
  glm::vec3 position = glm::vec3(0.0f, 10.0f, 10.0f);
  glm::vec3 ambient = glm::vec3(0.5f, 0.5f, 0.5f);
  glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
  glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);

  memcpy(lights->lightsBuffer + lights->position, &position[0], sizeof(GLfloat) * 3);
  memcpy(lights->lightsBuffer + lights->ambient, &ambient[0], sizeof(GLfloat) * 3);
  memcpy(lights->lightsBuffer + lights->diffuse, &diffuse[0], sizeof(GLfloat) * 3);
  memcpy(lights->lightsBuffer + lights->specular, &specular[0], sizeof(GLfloat) * 3);
  glBindBuffer(GL_UNIFORM_BUFFER, lights->lightsInfoUbod);
  glBufferData(GL_UNIFORM_BUFFER, lights->size, lights->lightsBuffer, GL_DYNAMIC_DRAW);
  prog->setUniformBlockBinding("Lights", 1);
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, lights->lightsInfoUbod);
}


// /////////////////////////////////////////////////////////////////////////////
/// \desc
/// Handles the drawing of everything to our buffer.  Needs the view and
///     projection matrices to apply as part of the ModelViewProjection matrix.
///
// /////////////////////////////////////////////////////////////////////////////
void renderScene( const glm::mat4 VIEW_MTX, const glm::mat4 PROJ_MTX ) {
  setupWorldInfo(&controlFirstHalfProgram.worldInfo, controlFirstHalfProgram.controlFirst, VIEW_MTX, PROJ_MTX);
  setupWorldInfo(&noiseProgram.worldInfo, noiseProgram.noiseProgram, VIEW_MTX, PROJ_MTX);
  setupWorldInfo(&controlProgram.worldInfo, controlProgram.controlProgram, VIEW_MTX, PROJ_MTX);
  setupWorldInfo(&justGroundProgram.worldInfo, justGroundProgram.justGroundProgram, VIEW_MTX, PROJ_MTX);
  setupWorldInfo(&phongProgram.worldInfo, phongProgram.phongProgram, VIEW_MTX, PROJ_MTX);

  setupLights(&phongProgram.lights, phongProgram.phongProgram, VIEW_MTX, PROJ_MTX);
  setupLights(&noiseProgram.lights, noiseProgram.noiseProgram, VIEW_MTX, PROJ_MTX);

  setupPermutationTable(&noiseProgram.permutationTable, noiseProgram.noiseProgram, VIEW_MTX, PROJ_MTX);

  glm::mat4 modelMtx = glm::mat4( 1.0f);
  glm::mat4 mvMtx = VIEW_MTX * modelMtx;
  glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(mvMtx)));
  glm::mat4 viewportMtx = {windowWidth / 2.0f, 0, 0, windowWidth / 2.0,
    0, windowHeight / 2.0, 0, windowHeight / 2.0,
    0, 0, 0.5, 0.5,
    0, 0, 0, 1};
	viewportMtx = glm::transpose(viewportMtx);

	justGroundProgram.justGroundProgram->setProgramUniform(justGroundProgram.modelView, mvMtx);
	justGroundProgram.justGroundProgram->setProgramUniform(justGroundProgram.normal, normalMtx);

	phongProgram.phongProgram->setProgramUniform(phongProgram.color, 0.0f, 1.0f, 0.0f, 1.0f);
	phongProgram.phongProgram->setProgramUniform(phongProgram.width, 0.02f);
	phongProgram.phongProgram->setProgramUniform(phongProgram.showWireframe, showWireframe);
	phongProgram.phongProgram->setProgramUniform(phongProgram.modelViewMatrix, mvMtx);
	phongProgram.phongProgram->setProgramUniform(phongProgram.viewportMtx, viewportMtx);

	controlFirstHalfProgram.controlFirst->setProgramUniform(controlFirstHalfProgram.modelView, mvMtx);
	controlFirstHalfProgram.controlFirst->setProgramUniform(controlFirstHalfProgram.normal, normalMtx);


	noiseProgram.noiseProgram->setProgramUniform(noiseProgram.color, 0.0f, 1.0f, 0.0f, 1.0f);
	noiseProgram.noiseProgram->setProgramUniform(noiseProgram.width, 0.02f);
	noiseProgram.noiseProgram->setProgramUniform(noiseProgram.showWireframe, showWireframe);
	noiseProgram.noiseProgram->setProgramUniform(noiseProgram.modelViewMatrix, mvMtx);
	noiseProgram.noiseProgram->setProgramUniform(noiseProgram.viewportMtx, viewportMtx);


	
	wireframeShaderProgram->bindPipeline();
	glBindVertexArray( vaods[VAOS.GROUND] );
	glDrawElements( GL_TRIANGLES, iboCounts.ground, GL_UNSIGNED_SHORT, (void*)0 );

	(perlinActive ? noiseShaderProgram : tessShaderProgram)->bindPipeline();
	glBindVertexArray( vaods[VAOS.CUBE] );
  glDrawElements( GL_PATCHES, iboCounts.cube, GL_UNSIGNED_SHORT, (void*)0 );

	if (showWireframe) {
		controlShaderProgram->bindPipeline();
		glDrawElements( GL_POINTS, iboCounts.cube, GL_UNSIGNED_SHORT, (void*)0 );
	}
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Update all of our scene objects - perform animation here
///
// /////////////////////////////////////////////////////////////////////////////
void updateScene() {

}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Print the formatted FPS information to the screen
///
// /////////////////////////////////////////////////////////////////////////////
void printFPS( GLint windowWidth, GLint windowHeight ) {
    GLdouble currFPS = 0, fpsAvg = 0, totalFPS = 0.0;

    // calculate the average FPS
    for( const auto& fps : fpsAvgs ) {
        totalFPS += fps;
    }
    if( !fpsAvgs.empty() ) {
        currFPS = fpsAvgs.back();
        fpsAvg = totalFPS / fpsAvgs.size();
    }

    char fpsStr[80];
    sprintf( fpsStr, "%.3f frames/sec (Avg: %.3f)", currFPS, fpsAvg);

    textShaderProgram->useProgram();
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBindVertexArray(fontVAO);

    GLfloat sx = 2.0 / (GLfloat)windowWidth;
    GLfloat sy = 2.0 / (GLfloat)windowHeight;

    renderText( fpsStr, fontCharacters, -1 + 8 * sx, 1 - 30 * sy, sx, sy );
    renderText( activeText, fontCharacters, -1 + 8 * sx, 1 - 60 * sy, sx, sy );
    renderText( activeIText, fontCharacters, -1 + 8 * sx, 1 - 90 * sy, sx, sy );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Runs our draw loop and renders/updates our scene
/// \param window - window to render the scene to
// /////////////////////////////////////////////////////////////////////////////
void run(GLFWwindow* window) {
    // NOTE: we are doing the Viewport and Projection calculations prior to the loop since we are not
    // allowing our window to be resized so these values will never change throughout the life of our
    // program.  If we allowed the window to be resized, then we would need to create a window resize
    // callback and update these values only on change.

    // Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
    // when using a Retina display the actual window can be larger than the requested window.  Therefore
    // query what the actual size of the window we are rendering to is.
    glfwGetFramebufferSize( window, &windowWidth, &windowHeight );

    // update the viewport - tell OpenGL we want to render to the whole window
    glViewport( 0, 0, windowWidth, windowHeight );

    // compute our projection matrix
    const glm::mat4 PROJ_MTX = glm::perspective( 45.0f, windowWidth / (GLfloat) windowHeight, 0.001f, 100.0f );

    //  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
    //	until the user decides to close the window and quit the program.  Without a loop, the
    //	window will display once and then the program exits.
    while( !glfwWindowShouldClose(window) ) {	        // check if the window was instructed to be closed
        // clear the prior contents of our buffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        // compute our view matrix based on our current camera setup
        glm::mat4 vMtx = glm::lookAt( arcballCam.lookAtPoint + arcballCam.camDir * arcballCam.cameraAngles.z,
                                      arcballCam.lookAtPoint,
                                      arcballCam.upVector );

        renderScene( vMtx, PROJ_MTX );                  // render our scene

        calculateFPS();                                 // compute current Frames Per Second
        printFPS( windowWidth, windowHeight );          // display FPS information on screen

        glfwSwapBuffers(window);                        // flush the OpenGL commands and make sure they get rendered!
        glfwPollEvents();				                // check for any events and signal to redraw screen

        updateScene();                                  // update the objects in our scene
    }
}

//**********************************************************************************************************************

// program entry point
int main() {
    GLFWwindow *window = initialize();                  // create OpenGL context and setup EVERYTHING for our program
    run(window);                                        // enter our draw loop and run our program
    shutdown(window);                                   // free up all the memory used and close OpenGL context
    return EXIT_SUCCESS;				                // exit our program successfully!
}
