#include <iostream>
#include <cmath>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projectionO, projectionP;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

struct COLOR {
    float r;
    float g;
    float b;
};

typedef struct COLOR color;
COLOR red = {0.882, 0.3333, 0.3333};
COLOR green = {0.1255, 0.75, 0.333};
COLOR black = {0, 0, 0};
COLOR steel = {196 / 255.0, 231 / 255.0, 249 / 255.0};
COLOR yellow = {1, 1, 0};
COLOR blue = {0, 0, 1};

struct Sprite {
    string name;
    int exists;
    COLOR color;
    float x, y, z;
    float height, width, depth, angle;
    VAO* object;
};

typedef struct Sprite Sprite;

map <string, Sprite> cube;
map <string, Sprite> tile;

GLuint programID;
int proj_type;
float goalx = 0, goalz = 0;
float camera_zoom = 0.7;
float camera_rotation_angle = 90;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
	{
	    std::string Line = "";
	    while(getline(VertexShaderStream, Line))
		VertexShaderCode += "\n" + Line;
	    VertexShaderStream.close();
	}

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
	std::string Line = "";
	while(getline(FragmentShaderStream, Line))
	    FragmentShaderCode += "\n" + Line;
	FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    //    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    //    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    //    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/



/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera_zoom += yoffset/10;
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
	case GLFW_KEY_C:
	    break;
	case GLFW_KEY_P:
	    break;
	case GLFW_KEY_X:
	    // do something ..
	    break;
	default:
	    break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
	case GLFW_KEY_ESCAPE:
	    quit(window);
	    break;
	default:
	    break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
    case 'o':
        camera_rotation_angle += 5;
        break;
    case 'p':
        camera_rotation_angle -= 5;
        break;
    case 'Q':
    case 'q':
    	quit(window);
	break;
    case ' ':
    	proj_type ^= 1;
    	break;
    case 'a':
    	cube["maincube"].x -= 0.5;
    	break;
    case 'd':
    	cube["maincube"].x += 0.5;
    	break;
    case 'w':
    	cube["maincube"].z -= 0.5;
    	break;
    case 's':
    	cube["maincube"].z += 0.5;
    	break;
    case 'f':
    	cube["maincube"].y += 0.5;
    	break;
    case 'r':
    	cube["maincube"].y -= 0.5;
    	break;
    default:
	break;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
        if(action == GLFW_RELEASE)
            break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        if(action == GLFW_RELEASE)
        break;
    default:
        break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = M_PI/2;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    Matrices.projectionP = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projectionO = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *triangle, *rectangle;

// Creates the triangle object used in this sample code
void createTriangle ()
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    static const GLfloat vertex_buffer_data [] = {
	0, 0, 0, // vertex 0
	-1, 1, 0, // vertex 1
	-1, 0, 0, // vertex 2
    };

    static const GLfloat color_buffer_data [] = {
	1,0,0, // color 0
	1,0,0, // color 1
	1,0,0, // color 2
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Creates the rectangle object used in this sample code
void createRectangle (string name, float x, float y, float z, float width, float height, float depth, string type, float angle, COLOR mycolor)
{
    float w = width / 2;
    float h = height / 2;
    float d = depth / 2;
    // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data[] = {
        -w,-h,-d, // triangle 1 : begin
        -w,-h, d,
        -w, h, d, // triangle 1 : end
        w, h,-d, // triangle 2 : begin
        -w,-h,-d,
        -w, h,-d, // triangle 2 : end
        w,-h, d,
        -w,-h,-d,
        w,-h,-d,
        w, h,-d,
        w,-h,-d,
        -w,-h,-d,
        -w,-h,-d,
        -w, h, d,
        -w, h,-d,
        w,-h, d,
        -w,-h, d,
        -w,-h,-d,
        -w, h, d,
        -w,-h, d,
        w,-h, d,
        w, h, d,
        w,-h,-d,
        w, h,-d,
        w,-h,-d,
        w, h, d,
        w,-h, d,
        w, h, d,
        w, h,-d,
        -w, h,-d,
        w, h, d,
        -w, h,-d,
        -w, h, d,
        w, h, d,
        -w, h, d,
        w,-h, d
    };

    if (type == "cube")
    {
        GLfloat color_buffer_data[] =
            {
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b, // color 2
                blue.r, blue.g, blue.b, // color 1
                yellow.r, yellow.g, yellow.b // color 2
            };
        rectangle = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
    }
    else
    {
        GLfloat color_buffer_data [] = {
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3
        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b, // color 6
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3
        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b, // color 6
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3
        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b, // color 6
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3
        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b, // color 6
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3
        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b, // color 6
        mycolor.r, mycolor.g, mycolor.b, // color 1
        mycolor.r, mycolor.g, mycolor.b, // color 2
        mycolor.r, mycolor.g, mycolor.b, // color 3
        mycolor.r, mycolor.g, mycolor.b, // color 4
        mycolor.r, mycolor.g, mycolor.b, // color 5
        mycolor.r, mycolor.g, mycolor.b // color 6
    };
        rectangle = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
    }

    // create3DObject creates and returns a handle to a VAO that can be used later
    Sprite elem = {};
    elem.exists = 1;
    elem.name = name;
    elem.object = rectangle;
    elem.x = x;
    elem.y = y;
    elem.z = z;
    elem.height = height;
    elem.width = width;
    elem.depth = depth;
    elem.angle = angle;

    if(type == "cube")
    {
        cube[name] = elem;
    }
    else if(type == "tile")
    {
        tile[name] = elem;
    }
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window, float x, float y, float w, float h)
{
    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));


    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 2, 5*sin(camera_rotation_angle*M_PI/180.0f) );
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (0, 0, 0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = (proj_type?Matrices.projectionP:Matrices.projectionO) * Matrices.view * glm::scale(glm::vec3(exp(camera_zoom)));

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    // Matrices.model = glm::mat4(1.0f);

    // glm::mat4 translateRectangle = glm::translate (rect_pos);        // glTranslatef
    // glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    // Matrices.model *= (translateRectangle * rotateRectangle);
    // MVP = VP * Matrices.model;
    // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // // draw3DObject draws the VAO given to it using current MVP matrix
    // draw3DObject(rectangle);

    // Increment angles
    // float increments = 1;

    for(map<string,Sprite>::iterator it = cube.begin(); it != cube.end(); it++)
    {
      int flag = 0;
      string current = it->first; //The name of the current object
      if(cube[current].exists == 0)
      {
          continue;
      }

      for(map<string,Sprite>::iterator it1=tile.begin(); it1!=tile.end(); it1++)
      {
          string tile_current = it1->first; //The name of the current object
          if(tile[tile_current].x == cube[current].x && tile[tile_current].z == cube[current].z)
          {
              flag = 1;
          }
      }
      if(cube[current].x == goalx && cube[current].z == goalz)
      {
          cout<<"You've won"<<endl;
          exit(0);
      }
      if(flag == 0)
      {
          cout<<"GAME OVER"<<endl;
          exit(0);
      }

      glm::mat4 MVP;    // MVP = Projection * View * Model

      Matrices.model = glm::mat4(1.0f);

      /* Render your scene */
      glm::mat4 ObjectTransform;
      glm::mat4 translateObject = glm::translate (glm::vec3(cube[current].x, cube[current].y, cube[current].z)); // glTranslatef
      glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)

      ObjectTransform=translateObject*rotateTriangle;
      Matrices.model *= ObjectTransform;
      MVP = VP * Matrices.model; // MVP = p * V * M

      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(cube[current].object);

      //glPopMatrix ();
    }

    for(map<string,Sprite>::iterator it=tile.begin(); it!=tile.end(); it++)
    {
      string current = it->first; //The name of the current object
      if(tile[current].exists == 0)
      {
          continue;
      }
      glm::mat4 MVP;    // MVP = Projection * View * Model


      Matrices.model = glm::mat4(1.0f);

      /* Render your scene */
      glm::mat4 ObjectTransform;
      glm::mat4 translateObject = glm::translate (glm::vec3(tile[current].x, tile[current].y, tile[current].z)); // glTranslatef
      glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)

      ObjectTransform=translateObject*rotateTriangle;
      Matrices.model *= ObjectTransform;
      MVP = VP * Matrices.model; // MVP = p * V * M

      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(tile[current].object);

      //glPopMatrix ();
    }

    //camera_rotation_angle++; // Simulating camera rotation
    //  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
    //  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
	exit(EXIT_FAILURE);
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    glfwSetWindowCloseCallback(window, quit);
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, scroll_callback);

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    // createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer

    createRectangle("maincube", 0.5, -0.15, 0, 0.5, 1, 0.5, "cube", 0, green);
    createRectangle("t(0.5,0)", 0.5, -0.7, 0, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(1,0)", 1, -0.7, 0, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(1.5,0)", 1.5, -0.7, 0, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(1,0.5)", 1, -0.7, 0.5, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(1,-0.5)", 1, -0.7, -0.5, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(0,0.5)", 0, -0.7, 0.5, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(-0.5,0)", -0.5, -0.7, 0, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(0.5,0.5)", 0.5, -0.7, 0.5, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(-0.5,0.5)", -0.5, -0.7, 0.5, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(0.5,-0.5)", 0.5, -0.7, -0.5, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(-0.5,-0.5)", -0.5, -0.7, -0.5, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(0,-0.5)", 0, -0.7, -0.5, 0.5, 0.1, 0.5, "tile", 0, black);
    createRectangle("t(-1,0)", -1, -0.7, 0, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(0,1)", 0, -0.7, 1, 0.5, 0.1, 0.5, "tile", 0, red);
    createRectangle("t(0,-1)", 0, -0.7, -1, 0.5, 0.1, 0.5, "tile", 0, red);

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    // cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    // cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    // cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    // cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
    int width = 600;
    int height = 600;
    proj_type = 1;

    GLFWwindow* window = initGLFW(width, height);
    initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

	// clear the color and depth in the frame buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // OpenGL Draw commands
	draw(window, 0, 0, 1, 1);
	// proj_type ^= 1;
	// draw(window, 0.5, 0, 0.5, 1);
	// proj_type ^= 1;

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}
