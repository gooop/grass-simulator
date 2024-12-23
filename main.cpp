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
#include "ssbo.cpp"

//**********************************************************************************************************************
// Structure definitions

struct Vertex {
	GLfloat px, py, pz;	// point location x,y,z
	GLfloat r, g, b;	// color r,g,b
};
unsigned int quadVAO = 0;
unsigned int quadVBO;

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
const GLint WINDOW_WIDTH = 2560, WINDOW_HEIGHT = 1395;
/* const GLint WINDOW_WIDTH = 640, WINDOW_HEIGHT = 640; */

// keep track of our mouse information
GLboolean isShiftDown;                          // if the control key was pressed when the mouse was pressed
GLboolean isLeftMouseDown;                      // if the mouse left button is currently pressed
glm::vec2 mousePosition;                        // current moust vPos

// keep track of all our camera information
struct CameraParameters {
    glm::vec3 cameraAngles;                     // cameraAngles --> x = thera, y = phi, z = radius
    glm::vec3 camDir;                           // direction to the camera
    glm::vec3 eyePoint;                         // camera vPos
    glm::vec3 lookAtPoint;                      // location of our object of interest
    glm::vec3 upVector;                         // the upVector of our camera
} arcballCam;

CSCI441::ShaderProgram *phongShaderProgram = nullptr;
CSCI441::ShaderProgram *lightingShaderProgram = nullptr;
GLuint computeProgram = 0;
const char* gText = "";
const char* pText = "";
const char* activeText = gText;

const char* bpText = "";
const char* piText = "";
const char* activeIText = piText;

GLuint ssbo = 0;

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
GLuint pointsvbo;

struct GouradShaderUniforms {
	GLuint worldInfoUbod;                               // ModelViewProjection matrix
	GLubyte *worldInfoBuffer;                               // ModelViewProjection matrix
	GLint mvpMtx;                               // ModelViewProjection matrix
	GLint vpMtx;                               // ModelViewProjection matrix
	GLint time;                                 // current time within the application
	GLint cameraPos;                                 // current time within the application
	GLuint lightsUbod;
	GLubyte *lightsBuffer;
  GLint position;
  GLint ambient;
  GLint diffuse;
  GLint specular;
  GLint gPosition;
  GLint gNormal;
  GLint gColorSpec;
  GLint passType;
} phongShaderUniforms, lightingShaderUniforms;

struct GouradShaderAttributes {
	GLint vPos;                                 // Vertex Position
	GLint vNormal;                               // Vertex Color
	GLint loc;
} phongShaderAttributes, lightingShaderAttributes;

GouradShaderAttributes *shaderAttributes = &phongShaderAttributes;

// FPS information
GLdouble currentTime, lastTime;                 // used to compute elapsed time
GLdouble lastFrame, thisFrame;
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

glm::vec3 *points = {};

unsigned int gBuffer;
unsigned int gPosition, gNormal, gColorSpec;
unsigned int rboDepth;
GrassShader *gs;

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
    // ------ Color Shader Program ------
	// load our color shader program
	phongShaderProgram        		    = new CSCI441::ShaderProgram( "shaders/phong.v.glsl", "shaders/phong.f.glsl" );
	/* phongShaderProgram        		    = new CSCI441::ShaderProgram( "shaders/bezierPatch.v.glsl", "shaders/bezierPatch.f.glsl" ); */
  // HERE the phongShaderProgram handles everything before deferred rendering.
  // If you comment out this line and uncomment the line above it, you can see
  // a single triangle drawn. Currently, the tessellation shaders don't seem to
  // work.
	/* phongShaderProgram        		    = new CSCI441::ShaderProgram( "shaders/bezierPatch.v.glsl", "shaders/bezierPatch.tc.glsl", "shaders/bezierPatch.te.glsl", "shaders/bezierPatch.f.glsl" ); */
	lightingShaderProgram        		    = new CSCI441::ShaderProgram( "shaders/lighting.v.glsl", "shaders/lighting.f.glsl" );
	/* lightingShaderProgram        		    = new CSCI441::ShaderProgram( "shaders/move.c.glsl"); */
	/* GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER); */
  GLuint computeShader = CSCI441_INTERNAL::ShaderUtils::compileShader( "shaders/move.c.glsl", GL_COMPUTE_SHADER );
  printf("Compute shader had ID: %d\n", computeShader);
  computeProgram = glCreateProgram();
  glAttachShader(computeProgram, computeShader);
  glLinkProgram(computeProgram);
  CSCI441_INTERNAL::ShaderUtils::printLog( computeProgram );

  /* glShaderSource(ray_shader, 1, &the_ray_shader_string, NULL); */
  /* glCompileShader(ray_shader); */
  /* // check for compilation errors as per normal here */

  /* GLuint ray_program = glCreateProgram(); */
  /* glAttachShader(ray_program, ray_shader); */
  /* glLinkProgram(ray_program); */
// check for linking errors and validate program as per normal here
	// query all of our uniform locations
	const GLchar* names[4] = {"mvpMtx", "time", "cameraPos", "vpMtx"};

	const GLchar* names_l[4] = {"position", "ambient", "diffuse", "specular"};

  // I should be able to use the same buffer as the gourad program, but the ShaderProgram API doesn't support that?
	phongShaderUniforms.worldInfoBuffer     = phongShaderProgram->getUniformBlockBuffer( "WorldInfo");
	phongShaderUniforms.mvpMtx          = phongShaderProgram->getUniformBlockOffsets("WorldInfo", names)[0];
	phongShaderUniforms.time          = phongShaderProgram->getUniformBlockOffsets("WorldInfo", names)[1];
	phongShaderUniforms.cameraPos          = phongShaderProgram->getUniformBlockOffsets("WorldInfo", names)[2];
	phongShaderUniforms.vpMtx          = phongShaderProgram->getUniformBlockOffsets("WorldInfo", names)[3];
	glGenBuffers(1, &phongShaderUniforms.worldInfoUbod);

	phongShaderUniforms.lightsBuffer     = phongShaderProgram->getUniformBlockBuffer( "Lights");
	phongShaderUniforms.position          = phongShaderProgram->getUniformBlockOffsets("Lights", names_l)[0];
	phongShaderUniforms.ambient          = phongShaderProgram->getUniformBlockOffsets("Lights", names_l)[1];
	phongShaderUniforms.diffuse          = phongShaderProgram->getUniformBlockOffsets("Lights", names_l)[2];
	phongShaderUniforms.specular          = phongShaderProgram->getUniformBlockOffsets("Lights", names_l)[3];
	glGenBuffers(1, &phongShaderUniforms.lightsUbod);

	// query all of our attribute locations
	phongShaderAttributes.vPos          = phongShaderProgram->getAttributeLocation( "vPos" );
	phongShaderAttributes.vNormal        = phongShaderProgram->getAttributeLocation("vNormal");
	phongShaderAttributes.loc        = phongShaderProgram->getAttributeLocation("loc");

	lightingShaderUniforms.worldInfoBuffer     = lightingShaderProgram->getUniformBlockBuffer( "WorldInfo");
	lightingShaderUniforms.mvpMtx          = lightingShaderProgram->getUniformBlockOffsets("WorldInfo", names)[0];
	lightingShaderUniforms.time          = lightingShaderProgram->getUniformBlockOffsets("WorldInfo", names)[1];
	lightingShaderUniforms.cameraPos          = lightingShaderProgram->getUniformBlockOffsets("WorldInfo", names)[2];
	lightingShaderUniforms.vpMtx          = lightingShaderProgram->getUniformBlockOffsets("WorldInfo", names)[3];
	glGenBuffers(1, &lightingShaderUniforms.worldInfoUbod);

	lightingShaderUniforms.lightsBuffer     = lightingShaderProgram->getUniformBlockBuffer( "Lights");
	lightingShaderUniforms.position          = lightingShaderProgram->getUniformBlockOffsets("Lights", names_l)[0];
	lightingShaderUniforms.ambient          = lightingShaderProgram->getUniformBlockOffsets("Lights", names_l)[1];
	lightingShaderUniforms.diffuse          = lightingShaderProgram->getUniformBlockOffsets("Lights", names_l)[2];
	lightingShaderUniforms.specular          = lightingShaderProgram->getUniformBlockOffsets("Lights", names_l)[3];
	glGenBuffers(1, &lightingShaderUniforms.lightsUbod);

	lightingShaderUniforms.gPosition          = lightingShaderProgram->getUniformLocation("gPosition");
	lightingShaderUniforms.gNormal          = lightingShaderProgram->getUniformLocation("gNormal");
	lightingShaderUniforms.gColorSpec          = lightingShaderProgram->getUniformLocation("gColorSpec");

	// query all of our attribute locations
	lightingShaderAttributes.vPos          = lightingShaderProgram->getAttributeLocation( "vPos" );
	lightingShaderAttributes.vNormal        = lightingShaderProgram->getAttributeLocation("vNormal");
	lightingShaderAttributes.loc        = lightingShaderProgram->getAttributeLocation("loc");

    // ------ FreeType Text Shader Program ------
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
  glPatchParameteri(GL_PATCH_VERTICES, 3);
  gs = new GrassShader();
  // Code from https://learnopengl.com/Advanced-Lighting/Deferred-Shading found from the 544 website.
  glGenFramebuffers(1, &gBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

  glGenRenderbuffers(1, &rboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

  // - position color buffer
  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &gPosition);
  glBindTexture(GL_TEXTURE_2D, gPosition);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

  // - normal color buffer
  glActiveTexture(GL_TEXTURE2);
  glGenTextures(1, &gNormal);
  glBindTexture(GL_TEXTURE_2D, gNormal);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

  // - color + specular color buffer
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &gColorSpec);
  glBindTexture(GL_TEXTURE_2D, gColorSpec);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

  // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
  unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
  glDrawBuffers(3, attachments);

  // finally check if framebuffer is complete
  /* if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { */
  /*   printf("Framebuffer not complete!\n"); */
  /* } */
  glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// generate our vertex array object descriptors
	glGenVertexArrays( NUM_VAOS, vaods );
  // generate our vertex buffer object descriptors
  glGenBuffers( NUM_VAOS, vbods );
  glGenBuffers(1, &pointsvbo);
  glGenBuffers(1, &ssbo);
  // generate our index buffer object descriptors
  glGenBuffers( NUM_VAOS, ibods );

  /* points = (glm::vec3*) malloc(sizeof(glm::vec3) * 100); */
  /* for (int i = 0; i < 100; i++) { */
  /*   points[i] = glm::vec3((i % 10) * 2 - 10, 0, i / 10 * 2 - 10); */
  /* } */
  points = (glm::vec3*) malloc(sizeof(glm::vec3) * 3);
  points[0] = {0.0f, 0.0f, 0.0f};
  points[1] = {0.0f, 0.2f, 0.0f};
  points[2] = {1.0f, 1.0f, 1.0f};

	glBindBuffer(GL_ARRAY_BUFFER, pointsvbo);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

  glm::vec4 locations[100];
  for (int i = 0; i < 100; i++) {
    locations[i] = {0, 0, 0, 0};
  }
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(locations), &locations, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

	//------------ BEGIN sphere VAO ------------

    // Loads the sphere from an obj file called sphere.obj
    std::vector<Vertex> verts;
    std::vector<Vertex> normals;
    std::vector<GLushort> ibos ;
    std::ifstream f("sphere.obj");

    while (!f.eof()) {
      std::string type;
      f >> type;
      if (type == "v") {
        float x, y, z;
        f >> x >> y >> z;
        verts.push_back(Vertex{x, y, z, 0, 0, 0});
      } else if (type == "vn") {
        float x, y, z;
        f >> x >> y >> z;
        normals.push_back(Vertex{0, 0, 0, x, y, z});
      } else if (type == "f") {
        std::string s;
        f >> s;
        int v = stoi(s.substr(0, s.find("/")))-1;
        int vn = stoi(s.substr(s.find("/", s.find("/") + 1) + 1, s.size() - s.find("/", s.find("/") + 1)))-1;
        verts[v].r = normals[vn].r;
        verts[v].g = normals[vn].g;
        verts[v].b = normals[vn].b;
        ibos.push_back(v);

        f >> s;
        v = stoi(s.substr(0, s.find("/")))-1;
        vn = stoi(s.substr(s.find("/", s.find("/") + 1) + 1, s.size() - s.find("/", s.find("/") + 1)))-1;
        verts[v].r = normals[vn].r;
        verts[v].g = normals[vn].g;
        verts[v].b = normals[vn].b;
        ibos.push_back(v);

        f >> s;
        v = stoi(s.substr(0, s.find("/")))-1;
        vn = stoi(s.substr(s.find("/", s.find("/") + 1) + 1, s.size() - s.find("/", s.find("/") + 1)))-1;
        verts[v].r = normals[vn].r;
        verts[v].g = normals[vn].g;
        verts[v].b = normals[vn].b;
        ibos.push_back(v);
      }
    }

    // HERE this is where that one triangle is created. You should be able to
    // ignore the big block of code above this. That was for loading obj files.
    ibos = {0, 1, 2};
    verts = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.2f, 0.0f}, {1.0f, 1.0f, 1.0f}};

    iboCounts.cube = ibos.size();

	// bind our Cube VAO
	glBindVertexArray( vaods[VAOS.CUBE] );

	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, pointsvbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(glm::vec3), points, GL_STATIC_DRAW );
	glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)(0));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(2, 1);

	// bind the VBO for our Cube Array Buffer
	glBindBuffer( GL_ARRAY_BUFFER, vbods[VAOS.CUBE] );
	// send the data to the GPU
	glBufferData( GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW );

	// bind the VBO for our Cube Element Array Buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibods[VAOS.CUBE] );
	// send the data to the GPU
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, ibos.size() * sizeof(GLushort), ibos.data(), GL_STATIC_DRAW );


	// enable our vPos attribute
	glEnableVertexAttribArray(0);
	// map the vPos attribute to data within our buffer
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0 );

	glEnableVertexAttribArray(1);
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(GLfloat) * 3));


	//------------  END  sphere VAO ------------

	//------------ BEGIN ground VAO ------------

    // specify our Ground Vertex Information
    const int NUM_GROUND_VERTICES = 4;
    const Vertex groundVertices[NUM_GROUND_VERTICES] = {
            { -15.0f, -5.0f, -15.0f, 0.0f, 1.0f, 0.0f }, // 0 - BL
            {  15.0f, -5.0f, -15.0f, 1.0f, 0.0f, 0.0f }, // 1 - BR
            {  15.0f, -5.0f,  15.0f, 0.0f, 1.0f, 1.0f }, // 2 - TR
            { -15.0f, -5.0f,  15.0f, 0.0f, 0.0f, 1.0f }  // 3 - TL
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
	glGenTextures(1, &fontTexture);
	printf("fonts is %d", fontTexture);
	/* glEnable( GL_TEXTURE_2D ); */
	glActiveTexture(GL_TEXTURE0);
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
    arcballCam.cameraAngles   = glm::vec3( 0.0, 1.85f, 15.0f );
    arcballCam.camDir         = glm::vec3( 1.0f, -1.0f, -1.0f );
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


    float quadVertices[] = {
      // positions        // texture Coords
      -1.0f,  1.0f, 0.2f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.2f, 0.0f, 0.0f,
      1.0f,  1.0f, 0.2f, 1.0f, 1.0f,
      1.0f, -1.0f, 0.2f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

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

    delete phongShaderProgram;
    delete textShaderProgram;
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

void setUniforms( const glm::mat4 VIEW_MTX, const glm::mat4 PROJ_MTX, CSCI441::ShaderProgram &shaderProgram, GouradShaderUniforms &shaderUniforms ) {
  // create our Model, View, Projection matrices
  glm::mat4 modelMtx = glm::mat4( 1.0f);

  // precompute the modelViewProjection matrix
  glm::mat4 mvpMtx = PROJ_MTX * VIEW_MTX * modelMtx;
  GLfloat time = 0; // glfwGetTime();


	memcpy(shaderUniforms.worldInfoBuffer + shaderUniforms.mvpMtx, &mvpMtx[0][0], sizeof(GLfloat) * 4 * 4);
	memcpy(shaderUniforms.worldInfoBuffer + shaderUniforms.time, &time, sizeof(GLfloat));
  glm::vec3 camera_at = arcballCam.lookAtPoint + arcballCam.camDir * arcballCam.cameraAngles.z;
	memcpy(shaderUniforms.worldInfoBuffer + shaderUniforms.cameraPos, &camera_at[0], sizeof(GLfloat) * 3);
	glBindBuffer(GL_UNIFORM_BUFFER, shaderUniforms.worldInfoUbod);
	glBufferData(GL_UNIFORM_BUFFER, shaderProgram.getUniformBlockSize("WorldInfo"), shaderUniforms.worldInfoBuffer, GL_DYNAMIC_DRAW);
	shaderProgram.setUniformBlockBinding("WorldInfo", 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, shaderUniforms.worldInfoUbod);

  if (&shaderProgram != phongShaderProgram) {
    glm::vec3 position = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::vec3 ambient = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
    memcpy(shaderUniforms.lightsBuffer + shaderUniforms.position, &position[0], sizeof(GLfloat) * 3);
    memcpy(shaderUniforms.lightsBuffer + shaderUniforms.ambient, &ambient[0], sizeof(GLfloat) * 3);
    memcpy(shaderUniforms.lightsBuffer + shaderUniforms.diffuse, &diffuse[0], sizeof(GLfloat) * 3);
    memcpy(shaderUniforms.lightsBuffer + shaderUniforms.specular, &specular[0], sizeof(GLfloat) * 3);
    glBindBuffer(GL_UNIFORM_BUFFER, shaderUniforms.lightsUbod);
    glBufferData(GL_UNIFORM_BUFFER, shaderProgram.getUniformBlockSize("Lights"), shaderUniforms.lightsBuffer, GL_DYNAMIC_DRAW);
    shaderProgram.setUniformBlockBinding("Lights", 1);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, shaderUniforms.lightsUbod);
  }

}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
/// Handles the drawing of everything to our buffer.  Needs the view and
///     projection matrices to apply as part of the ModelViewProjection matrix.
///
// /////////////////////////////////////////////////////////////////////////////
void renderScene( const glm::mat4 VIEW_MTX, const glm::mat4 PROJ_MTX ) {
	// use our Color Shading Program
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	phongShaderProgram->useProgram();
	setUniforms(VIEW_MTX, PROJ_MTX, *phongShaderProgram, phongShaderUniforms);

  // -------- draw our ground --------

	// bind our Ground VAO
	/* glBindVertexArray( vaods[VAOS.GROUND] ); */
	// draw our ground!
	/* glDrawElements( GL_TRIANGLES, iboCounts.ground, GL_UNSIGNED_SHORT, (void*)0 ); */

	// -------- draw our rotating cube --------

	// send the data to the GPU
  /* glBindVertexArray( vaods[VAOS.CUBE] ); */
  /* // HERE we draw our "cube". I haven't actually changed the names since the first lab. */
  /* glDrawElementsInstanced( GL_PATCHES, iboCounts.cube, GL_UNSIGNED_SHORT, (void*)0, 1 ); */
  lastFrame = thisFrame;
  thisFrame = glfwGetTime();
  glm::mat4 modelMtx = glm::mat4( 1.0f);
  glm::mat4 mvpMtx = PROJ_MTX * VIEW_MTX * modelMtx;
  gs->run(thisFrame, thisFrame - lastFrame, mvpMtx);

  // this is all the deferred stuff.
	lightingShaderProgram->useProgram();
	setUniforms(VIEW_MTX, PROJ_MTX, *lightingShaderProgram, lightingShaderUniforms);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gPosition);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gNormal);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, gColorSpec);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
  // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
  // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
  // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
  glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    /* glEnable( GL_TEXTURE_2D ); */
    glActiveTexture(GL_TEXTURE0);
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
    GLint windowWidth, windowHeight;
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
