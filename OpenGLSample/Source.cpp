#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <vector>

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"      // Image loading Utility functions

#include "camera.h" // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "7-1: Final Project (J.Pierce Waren)"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];         // Handle for the vertex buffer object
        GLuint nIndices;    // Number of indicies of the mesh
        GLuint nVertices;    // Number of vertices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh gMeshPlaystation, gMeshPlaystationCylinder, gMeshFloor, gMeshLightSource, gMeshSpiderman, gMeshRedAlert, gMeshDK, gMeshGB;

    // Texture
    GLuint woodTexture, playstationPlasticTexture, playstationLogoTexture, dkTexture, spidermanTexture, redAlertTexture, gbTexture; // Declare separate texture IDs
    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader program
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool isPerspectiveView = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Cube and light color
    glm::vec3 gObjectColor(1.f, 1.0f, 1.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(3.0f, 8.0f, 8.0f);
    glm::vec3 gLightScale(1.0f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UCreateAllMeshes();
void UCreatePlaneMesh(GLMesh& mesh);
void UCreateCube(GLMesh& mesh);
void UCreateCylinderMesh(GLMesh& mesh); void UDestroyMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender(bool& isPerspectiveView);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;

    //Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor; // For outgoing cube color to the GPU

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture; // Useful when working with multiple textures
    uniform vec2 uvScale;

    void main()
    {
        /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
        float ambientStrength = 0.22f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.9f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


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
    UCreateAllMeshes();         // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename1 = "../resources/textures/wood.jpg";
    if (!UCreateTexture(texFilename1, woodTexture))
    {
        cout << "Failed to load texture " << texFilename1 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 2
    const char* texFilename2 = "../resources/textures/ps1.png";
    if (!UCreateTexture(texFilename2, playstationPlasticTexture))
    {
        cout << "Failed to load texture " << texFilename2 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 3
    const char* texFilename3 = "../resources/textures/logo.jpg";
    if (!UCreateTexture(texFilename3, playstationLogoTexture))
    {
        cout << "Failed to load texture " << texFilename3 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 4
    const char* texFilename4 = "../resources/textures/gb5.png";
    if (!UCreateTexture(texFilename4, gbTexture))
    {
        cout << "Failed to load texture " << texFilename3 << endl;
        return EXIT_FAILURE;
    }

    //// Load texture 5
    const char* texFilename5 = "../resources/textures/dk.jpg";
    if (!UCreateTexture(texFilename5, dkTexture))
    {
        cout << "Failed to load texture " << texFilename5 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 6
    const char* texFilename6 = "../resources/textures/ra.png";
    if (!UCreateTexture(texFilename6, redAlertTexture))
    {
        cout << "Failed to load texture " << texFilename6 << endl;
        return EXIT_FAILURE;
    }

    //// Load texture 7
    const char* texFilename7 = "../resources/textures/spiderman2.png";
    if (!UCreateTexture(texFilename7, spidermanTexture))
    {
        cout << "Failed to load texture " << texFilename7 << endl;
        return EXIT_FAILURE;
    }


    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

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
        URender(isPerspectiveView);

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMeshFloor);
    UDestroyMesh(gMeshPlaystation);
    UDestroyMesh(gMeshPlaystationCylinder);
    UDestroyMesh(gMeshGB);
    UDestroyMesh(gMeshSpiderman);
    UDestroyMesh(gMeshRedAlert);
    UDestroyMesh(gMeshDK);

    // Release texture
    UDestroyTexture(woodTexture);
    UDestroyTexture(playstationPlasticTexture);
    UDestroyTexture(playstationLogoTexture);
    UDestroyTexture(gbTexture);
    UDestroyTexture(spidermanTexture);
    UDestroyTexture(redAlertTexture);
    UDestroyTexture(dkTexture);


    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

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
    glfwSetKeyCallback(*window, key_callback);

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
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
}


// glfw: whenever the key P is pressed this callback function executes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        // This toggles the 2 different projection views
        isPerspectiveView = !(isPerspectiveView);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
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
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}


// Functioned called to render a frame
void URender(bool& isPerspectiveView)
{
    glm::mat4 scale;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    GLint UVScaleLoc;
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;
    GLint objectColorLoc;
    GLint lightColorLoc;
    GLint lightPositionLoc;
    GLint viewPositionLoc;
    const glm::vec3 cameraPosition = gCamera.Position;;

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the camera object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Transforms the camera: move the camera back (z axis) and upwards (y axis)
    view = gCamera.GetViewMatrix();
    // Creates a perspective projection. Allows the user to switch between perspective and ortho views
    if (isPerspectiveView) {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the floor (plane) object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Set the shader to be used
    glUseProgram(gProgramId);

    // 1. Scales the object by 1 in the x and z axis and 1.15 in the y axis.
    scale = glm::scale(glm::vec3(20.0f, 20.0f, 20.0f));
    // 2. No rotation is made to the cylinder.
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    // 3. Place object at the origin and lowers the cylinder on the y axis by -0.1
    translation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // const glm::vec3 cameraPosition = gCamera.Position;;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glm::vec2 gUVScaleFloor(4.24305f, 4.24305f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScaleFloor));

    // Render the plane mesh with texture 1
    glBindVertexArray(gMeshFloor.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodTexture);

    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
    
    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshFloor.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the Playstation Body Object (Cube) object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    scale = glm::scale(glm::vec3(2.8f, 2.0f, 0.4f));
    rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(0.0f, -0.3f, 0.0f));
    model = translation * rotation * scale;             // Model matrix: transformations are applied right-to-left order

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glm::vec2 gUVScalePS1Plastic(0.986171f, 0.986171f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScalePS1Plastic));

    // Render the cube mesh with texture 2
    glBindVertexArray(gMeshPlaystation.vao);
    glActiveTexture(GL_TEXTURE1); // Use a different texture unit for the second texture
    glBindTexture(GL_TEXTURE_2D, playstationPlasticTexture);
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1); // Use texture unit 1 for sampling

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshPlaystation.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the Playstation Cylinder object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // 1. Scales the object by 1 in the x and z axis and 1.15 in the y axis.
    scale = glm::scale(glm::vec3(0.9f, 0.05f, 0.9f));
    // 2. No rotation is made to the cylinder.
    rotation = glm::rotate(glm::radians(12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // 3. Place object at the origin and lowers the cylinder on the y axis by -0.1
    translation = glm::translate(glm::vec3(0.0f, -0.1f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    //  const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");

    glm::vec2 gUVScalePS1Logo(1.00998f, 1.00998f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScalePS1Logo));

    // Render the cylinder mesh with texture 3
    glBindVertexArray(gMeshPlaystationCylinder.vao);
    glActiveTexture(GL_TEXTURE2); // Use another texture unit for the third texture
    glBindTexture(GL_TEXTURE_2D, playstationLogoTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    gTexWrapMode = GL_MIRRORED_REPEAT;
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2); // Use texture unit 2 for sampling


    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the Game Boy object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    scale = glm::scale(glm::vec3(0.95f, 1.5f, 0.2f));
    rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(glm::vec3(-1.4f, -0.4f, 2.1f));
    model = translation * rotation * scale;             // Model matrix: transformations are applied right-to-left order

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");

    glm::vec2 gUVScaleGB(1.02017f, 1.02017f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScaleGB));

    // Render the cube mesh with texture 2
    glBindVertexArray(gMeshGB.vao);
    glActiveTexture(GL_TEXTURE3); // Use a different texture unit for the second texture
    glBindTexture(GL_TEXTURE_2D, gbTexture);
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3); // Use texture unit 1 for sampling

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshGB.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the Donkey Kong Game object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    scale = glm::scale(glm::vec3(0.51f, 0.05f, 0.61f));
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-0.25f, -0.46f, 1.65f));
    model = translation * rotation * scale;             // Model matrix: transformations are applied right-to-left order

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");

    glm::vec2 gUVScaleDK(0.97998f, 0.97998f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScaleDK));

    // Render the cube mesh with texture 2
    glBindVertexArray(gMeshDK.vao);
    glActiveTexture(GL_TEXTURE4); // Use a different texture unit for the second texture
    glBindTexture(GL_TEXTURE_2D, dkTexture);
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 4); // Use texture unit 1 for sampling

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshDK.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the Red Alert PS1 Game object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    scale = glm::scale(glm::vec3(1.50f, 0.15f, 1.37f));
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(355.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(1.7f, -0.42f, 1.65f));
    model = translation * rotation * scale;             // Model matrix: transformations are applied right-to-left order

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glm::vec2 gUVScaleRedAlert(0.987171f, 0.987171f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScaleRedAlert));

    // Render the cube mesh with texture 2
    glBindVertexArray(gMeshRedAlert.vao);
    glActiveTexture(GL_TEXTURE5); // Use a different texture unit for the second texture
    glBindTexture(GL_TEXTURE_2D, redAlertTexture);
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 5); // Use texture unit 1 for sampling

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshRedAlert.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // This section renders the Spiderman PS2 Game object.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // 1. Scales the object by 1 in the x and z axis and 1.15 in the y axis.
    scale = glm::scale(glm::vec3(0.58f, 0.01f, 0.58f));
    rotation = glm::rotate(glm::radians(93.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.13f, -0.5f, 2.7f));
    model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");

    glm::vec2 gUVScaleSpiderman(0.989171f, 0.989171f);
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScaleSpiderman));

    // Render the cube mesh with texture 2
    glBindVertexArray(gMeshSpiderman.vao);
    glActiveTexture(GL_TEXTURE6); // Use a different texture unit for the second texture
    glBindTexture(GL_TEXTURE_2D, spidermanTexture);
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 6); // Use texture unit 1 for sampling

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides
    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // LAMP: draw lamp
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    glUseProgram(gLampProgramId);

    glBindVertexArray(gMeshLightSource.vao);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshLightSource.nIndices, GL_UNSIGNED_INT, (void*)0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // LAMP: draw lamp
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    glUseProgram(gLampProgramId);

    glBindVertexArray(gMeshLightSource.vao);

    //Transform the smaller cube used as a visual que for the light source
    glm::vec3 gLightPosition(-3.0f, 8.0f, 8.0f);
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMeshLightSource.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Creates the cube and cylinder meshes
void UCreateAllMeshes() {
    UCreatePlaneMesh(gMeshFloor);   // Calls the function to create the Vertext Buffer Object
    UCreateCube(gMeshPlaystation); // Calls the function to create the Vertex Buffer Object
    UCreateCylinderMesh(gMeshPlaystationCylinder);  // Calls the function to create the Vertex Buffer Object
    UCreateCube(gMeshGB); // Calls the function to create the Vertex Buffer Object
    UCreateCube(gMeshDK); // Calls the function to create the Vertex Buffer Object
    UCreateCube(gMeshRedAlert); // Calls the function to create the Vertex Buffer Object
    UCreateCylinderMesh(gMeshSpiderman);
    UCreateCube(gMeshLightSource); // Calls the function to create the Vertex Buffer Object
}


// Impements the UCreatePlane function
void UCreatePlaneMesh(GLMesh& mesh) {
    // Vertex data
    GLfloat verts[] = {
        // Vertex Positions		// Normals			// Texture coords	        // Color          // Index
        -1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	0.0f, 0.0f,	  //0
        1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	1.0f, 0.0f,	 //1
        1.0f,  0.0f, -1.0f,		0.0f, 1.0f, 0.0f,	1.0f, 1.0f, //2
        -1.0f, 0.0f, -1.0f,		0.0f, 1.0f, 0.0f,	0.0f, 1.0f //3
    };

    // Index data
    GLuint indices[] = {
        0,1,2,
        0,3,2
    };

    // total float values per each type
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    // store vertex and index count
    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    // Generate the VAO for the mesh
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);	// activate the VAO

    // Create VBOs for the mesh
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends data to the GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


// Implements the UCreateCube function
void UCreateCube(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions				//Normals
        // ------------------------------------------------------

        //Back Face				//Negative Z Normal  Texture Coords.
        0.5f, 0.5f, -0.5f,		0.0f,  0.0f, -1.0f,  0.0f, 1.0f,   //0
        0.5f, -0.5f, -0.5f,		0.0f,  0.0f, -1.0f,  0.0f, 0.0f,   //1
        -0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,  1.0f, 0.0f,   //2
        -0.5f, 0.5f, -0.5f,		0.0f,  0.0f, -1.0f,  1.0f, 1.0f,   //3

        //Bottom Face			//Negative Y Normal
        -0.5f, -0.5f, 0.5f,		0.0f, -1.0f,  0.0f,  0.0f, 1.0f,  //4
        -0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  //5
        0.5f, -0.5f, -0.5f,		0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  //6
        0.5f, -0.5f,  0.5f,		0.0f, -1.0f,  0.0f,  1.0f, 1.0f,  //7

        //Left Face				//Negative X Normal
        -0.5f, 0.5f, -0.5f,		1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  //8
        -0.5f, -0.5f,  -0.5f,	1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  //9
        -0.5f,  -0.5f,  0.5f,	1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  //10
        -0.5f,  0.5f,  0.5f,	1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  //11

        //Right Face			//Positive X Normal
        0.5f,  0.5f,  0.5f,		1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  //12
        0.5f,  -0.5f, 0.5f,		1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  //13
        0.5f, -0.5f, -0.5f,		1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  //14
        0.5f, 0.5f, -0.5f,		1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  //15

        //Top Face				//Positive Y Normal
        -0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,  0.0f, 1.0f, //16
        -0.5f,  0.5f, 0.5f,		0.0f,  1.0f,  0.0f,  0.0f, 0.0f, //17
        0.5f,  0.5f,  0.5f,		0.0f,  1.0f,  0.0f,  1.0f, 0.0f, //18
        0.5f,  0.5f,  -0.5f,	0.0f,  1.0f,  0.0f,  1.0f, 1.0f, //19

        //Front Face			//Positive Z Normal
        -0.5f, 0.5f,  0.5f,	    0.0f,  0.0f,  1.0f,  0.0f, 1.0f, //20
        -0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,  0.0f, 0.0f, //21
        0.5f,  -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,  1.0f, 0.0f, //22
        0.5f,  0.5f,  0.5f,		0.0f,  0.0f,  1.0f,  1.0f, 1.0f, //23
    };

    // Index data
    GLuint indices[] = {
        0,1,2,
        0,3,2,
        4,5,6,
        4,7,6,
        8,9,10,
        8,11,10,
        12,13,14,
        12,15,14,
        16,17,18,
        16,19,18,
        20,21,22,
        20,23,22
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


// Implements the UCreateCylinderMesh function to create a cylinder
void UCreateCylinderMesh(GLMesh& mesh)
{
    GLfloat verts[] = {
        // cylinder bottom		// normals			// texture coords
        1.0f, 0.0f, 0.0f,		0.0f, -1.0f, 0.0f,	0.5f,1.0f,
        .98f, 0.0f, -0.17f,		0.0f, -1.0f, 0.0f,	0.41f, 0.983f,
        .94f, 0.0f, -0.34f,		0.0f, -1.0f, 0.0f,	0.33f, 0.96f,
        .87f, 0.0f, -0.5f,		0.0f, -1.0f, 0.0f,	0.25f, 0.92f,
        .77f, 0.0f, -0.64f,		0.0f, -1.0f, 0.0f,	0.17f, 0.87f,
        .64f, 0.0f, -0.77f,		0.0f, -1.0f, 0.0f,	0.13f, 0.83f,
        .5f, 0.0f, -0.87f,		0.0f, -1.0f, 0.0f,	0.08f, 0.77f,
        .34f, 0.0f, -0.94f,		0.0f, -1.0f, 0.0f,	0.04f, 0.68f,
        .17f, 0.0f, -0.98f,		0.0f, -1.0f, 0.0f,	0.017f, 0.6f,
        0.0f, 0.0f, -1.0f,		0.0f, -1.0f, 0.0f,	0.0f,0.5f,
        -.17f, 0.0f, -0.98f,	0.0f, -1.0f, 0.0f,	0.017f, 0.41f,
        -.34f, 0.0f, -0.94f,	0.0f, -1.0f, 0.0f,	0.04f, 0.33f,
        -.5f, 0.0f, -0.87f,		0.0f, -1.0f, 0.0f,	0.08f, 0.25f,
        -.64f, 0.0f, -0.77f,	0.0f, -1.0f, 0.0f,	0.13f, 0.17f,
        -.77f, 0.0f, -0.64f,	0.0f, -1.0f, 0.0f,	0.17f, 0.13f,
        -.87f, 0.0f, -0.5f,		0.0f, -1.0f, 0.0f,	0.25f, 0.08f,
        -.94f, 0.0f, -0.34f,	0.0f, -1.0f, 0.0f,	0.33f, 0.04f,
        -.98f, 0.0f, -0.17f,	0.0f, -1.0f, 0.0f,	0.41f, 0.017f,
        -1.0f, 0.0f, 0.0f,		0.0f, -1.0f, 0.0f,	0.5f, 0.0f,
        -.98f, 0.0f, 0.17f,		0.0f, -1.0f, 0.0f,	0.6f, 0.017f,
        -.94f, 0.0f, 0.34f,		0.0f, -1.0f, 0.0f,	0.68f, 0.04f,
        -.87f, 0.0f, 0.5f,		0.0f, -1.0f, 0.0f,	0.77f, 0.08f,
        -.77f, 0.0f, 0.64f,		0.0f, -1.0f, 0.0f,	0.83f, 0.13f,
        -.64f, 0.0f, 0.77f,		0.0f, -1.0f, 0.0f,	0.87f, 0.17f,
        -.5f, 0.0f, 0.87f,		0.0f, -1.0f, 0.0f,	0.92f, 0.25f,
        -.34f, 0.0f, 0.94f,		0.0f, -1.0f, 0.0f,	0.96f, 0.33f,
        -.17f, 0.0f, 0.98f,		0.0f, -1.0f, 0.0f,	0.983f, 0.41f,
        0.0f, 0.0f, 1.0f,		0.0f, -1.0f, 0.0f,	1.0f, 0.5f,
        .17f, 0.0f, 0.98f,		0.0f, -1.0f, 0.0f,	0.983f, 0.6f,
        .34f, 0.0f, 0.94f,		0.0f, -1.0f, 0.0f,	0.96f, 0.68f,
        .5f, 0.0f, 0.87f,		0.0f, -1.0f, 0.0f,	0.92f, 0.77f,
        .64f, 0.0f, 0.77f,		0.0f, -1.0f, 0.0f,	0.87f, 0.83f,
        .77f, 0.0f, 0.64f,		0.0f, -1.0f, 0.0f,	0.83f, 0.87f,
        .87f, 0.0f, 0.5f,		0.0f, -1.0f, 0.0f,	0.77f, 0.92f,
        .94f, 0.0f, 0.34f,		0.0f, -1.0f, 0.0f,	0.68f, 0.96f,
        .98f, 0.0f, 0.17f,		0.0f, -1.0f, 0.0f,	0.6f, 0.983f,

        // cylinder top			// normals			// texture coords
        1.0f, 1.0f, 0.0f,		0.0f, 1.0f, 0.0f,	0.5f,1.0f,
        .98f, 1.0f, -0.17f,		0.0f, 1.0f, 0.0f,	0.41f, 0.983f,
        .94f, 1.0f, -0.34f,		0.0f, 1.0f, 0.0f,	0.33f, 0.96f,
        .87f, 1.0f, -0.5f,		0.0f, 1.0f, 0.0f,	0.25f, 0.92f,
        .77f, 1.0f, -0.64f,		0.0f, 1.0f, 0.0f,	0.17f, 0.87f,
        .64f, 1.0f, -0.77f,		0.0f, 1.0f, 0.0f,	0.13f, 0.83f,
        .5f, 1.0f, -0.87f,		0.0f, 1.0f, 0.0f,	0.08f, 0.77f,
        .34f, 1.0f, -0.94f,		0.0f, 1.0f, 0.0f,	0.04f, 0.68f,
        .17f, 1.0f, -0.98f,		0.0f, 1.0f, 0.0f,	0.017f, 0.6f,
        0.0f, 1.0f, -1.0f,		0.0f, 1.0f, 0.0f,	0.0f,0.5f,
        -.17f, 1.0f, -0.98f,	0.0f, 1.0f, 0.0f,	0.017f, 0.41f,
        -.34f, 1.0f, -0.94f,	0.0f, 1.0f, 0.0f,	0.04f, 0.33f,
        -.5f, 1.0f, -0.87f,		0.0f, 1.0f, 0.0f,	0.08f, 0.25f,
        -.64f, 1.0f, -0.77f,	0.0f, 1.0f, 0.0f,	0.13f, 0.17f,
        -.77f, 1.0f, -0.64f,	0.0f, 1.0f, 0.0f,	0.17f, 0.13f,
        -.87f, 1.0f, -0.5f,		0.0f, 1.0f, 0.0f,	0.25f, 0.08f,
        -.94f, 1.0f, -0.34f,	0.0f, 1.0f, 0.0f,	0.33f, 0.04f,
        -.98f, 1.0f, -0.17f,	0.0f, 1.0f, 0.0f,	0.41f, 0.017f,
        -1.0f, 1.0f, 0.0f,		0.0f, 1.0f, 0.0f,	0.5f, 0.0f,
        -.98f, 1.0f, 0.17f,		0.0f, 1.0f, 0.0f,	0.6f, 0.017f,
        -.94f, 1.0f, 0.34f,		0.0f, 1.0f, 0.0f,	0.68f, 0.04f,
        -.87f, 1.0f, 0.5f,		0.0f, 1.0f, 0.0f,	0.77f, 0.08f,
        -.77f, 1.0f, 0.64f,		0.0f, 1.0f, 0.0f,	0.83f, 0.13f,
        -.64f, 1.0f, 0.77f,		0.0f, 1.0f, 0.0f,	0.87f, 0.17f,
        -.5f, 1.0f, 0.87f,		0.0f, 1.0f, 0.0f,	0.92f, 0.25f,
        -.34f, 1.0f, 0.94f,		0.0f, 1.0f, 0.0f,	0.96f, 0.33f,
        -.17f, 1.0f, 0.98f,		0.0f, 1.0f, 0.0f,	0.983f, 0.41f,
        0.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,	1.0f, 0.5f,
        .17f, 1.0f, 0.98f,		0.0f, 1.0f, 0.0f,	0.983f, 0.6f,
        .34f, 1.0f, 0.94f,		0.0f, 1.0f, 0.0f,	0.96f, 0.68f,
        .5f, 1.0f, 0.87f,		0.0f, 1.0f, 0.0f,	0.92f, 0.77f,
        .64f, 1.0f, 0.77f,		0.0f, 1.0f, 0.0f,	0.87f, 0.83f,
        .77f, 1.0f, 0.64f,		0.0f, 1.0f, 0.0f,	0.83f, 0.87f,
        .87f, 1.0f, 0.5f,		0.0f, 1.0f, 0.0f,	0.77f, 0.92f,
        .94f, 1.0f, 0.34f,		0.0f, 1.0f, 0.0f,	0.68f, 0.96f,
        .98f, 1.0f, 0.17f,		0.0f, 1.0f, 0.0f,	0.6f, 0.983f,

        // cylinder body		// normals				// texture coords
        1.0f, 1.0f, 0.0f,		1.0f, 0.0f, 0.0f,		0.0,1.0,
        1.0f, 0.0f, 0.0f,		1.0f, 0.0f, 0.0f,		0.0,0.0,
        .98f, 0.0f, -0.17f,		1.0f, 0.0f, 0.0f,		0.0277,0.0,
        1.0f, 1.0f, 0.0f,		0.92f, 0.0f, -0.08f,	0.0,1.0,
        .98f, 1.0f, -0.17f,		0.92f, 0.0f, -0.08f,	0.0277,1.0,
        .98f, 0.0f, -0.17f,		0.92f, 0.0f, -0.08f,	0.0277,0.0,
        .94f, 0.0f, -0.34f,		0.83f, 0.0f, -0.17f,	0.0554,0.0,
        .98f, 1.0f, -0.17f,		0.83f, 0.0f, -0.17f,	0.0277,1.0,
        .94f, 1.0f, -0.34f,		0.83f, 0.0f, -0.17f,	0.0554,1.0,
        .94f, 0.0f, -0.34f,		0.75f, 0.0f, -0.25f,	0.0554,0.0,
        .87f, 0.0f, -0.5f,		0.75f, 0.0f, -0.25f,	0.0831,0.0,
        .94f, 1.0f, -0.34f,		0.75f, 0.0f, -0.25f,	0.0554,1.0,
        .87f, 1.0f, -0.5f,		0.67f, 0.0f, -0.33f,	0.0831,1.0,
        .87f, 0.0f, -0.5f,		0.67f, 0.0f, -0.33f,	0.0831,0.0,
        .77f, 0.0f, -0.64f,		0.67f, 0.0f, -0.33f,	0.1108,0.0,
        .87f, 1.0f, -0.5f,		0.58f, 0.0f, -0.42f,	0.0831,1.0,
        .77f, 1.0f, -0.64f,		0.58f, 0.0f, -0.42f,	0.1108,1.0,
        .77f, 0.0f, -0.64f,		0.58f, 0.0f, -0.42f,	0.1108,0.0,
        .64f, 0.0f, -0.77f,		0.5f, 0.0f, -0.5f,		0.1385,0.0,
        .77f, 1.0f, -0.64f,		0.5f, 0.0f, -0.5f,		0.1108,1.0,
        .64f, 1.0f, -0.77f,		0.5f, 0.0f, -0.5f,		0.1385,1.0,
        .64f, 0.0f, -0.77f,		0.42f, 0.0f, -0.58f,	0.1385,0.0,
        .5f, 0.0f, -0.87f,		0.42f, 0.0f, -0.58f,	0.1662,0.0,
        .64f, 1.0f, -0.77f,		0.42f, 0.0f, -0.58f,	0.1385, 1.0,
        .5f, 1.0f, -0.87f,		0.33f, 0.0f, -0.67f,	0.1662, 1.0,
        .5f, 0.0f, -0.87f,		0.33f, 0.0f, -0.67f,	0.1662, 0.0,
        .34f, 0.0f, -0.94f,		0.33f, 0.0f, -0.67f,	0.1939, 0.0,
        .5f, 1.0f, -0.87f,		0.25f, 0.0f, -0.75f,	0.1662, 1.0,
        .34f, 1.0f, -0.94f,		0.25f, 0.0f, -0.75f,	0.1939, 1.0,
        .34f, 0.0f, -0.94f,		0.25f, 0.0f, -0.75f,	0.1939, 0.0,
        .17f, 0.0f, -0.98f,		0.17f, 0.0f, -0.83f,	0.2216, 0.0,
        .34f, 1.0f, -0.94f,		0.17f, 0.0f, -0.83f,	0.1939, 1.0,
        .17f, 1.0f, -0.98f,		0.17f, 0.0f, -0.83f,	0.2216, 1.0,
        .17f, 0.0f, -0.98f,		0.08f, 0.0f, -0.92f,	0.2216, 0.0,
        0.0f, 0.0f, -1.0f,		0.08f, 0.0f, -0.92f,	0.2493, 0.0,
        .17f, 1.0f, -0.98f,		0.08f, 0.0f, -0.92f,	0.2216, 1.0,
        0.0f, 1.0f, -1.0f,		0.0f, 0.0f, -1.0f,		0.2493, 1.0,
        0.0f, 0.0f, -1.0f,		0.0f, 0.0f, -1.0f,		0.2493, 0.0,
        -.17f, 0.0f, -0.98f,	0.0f, 0.0f, -1.0f,		0.277, 0.0,
        0.0f, 1.0f, -1.0f,		0.08f, 0.0f, -1.08f,	0.2493, 1.0,
        -.17f, 1.0f, -0.98f,	-0.08f, 0.0f, -0.92f,	0.277, 1.0,
        -.17f, 0.0f, -0.98f,	-0.08f, 0.0f, -0.92f,	0.277, 0.0,
        -.34f, 0.0f, -0.94f,	-0.08f, 0.0f, -0.92f,	0.3047, 0.0,
        -.17f, 1.0f, -0.98f,	-0.08f, 0.0f, -0.92f,	0.277, 1.0,
        -.34f, 1.0f, -0.94f,	-0.17f, 0.0f, -0.83f,	0.3047, 1.0,
        -.34f, 0.0f, -0.94f,	-0.17f, 0.0f, -0.83f,	0.3047, 0.0,
        -.5f, 0.0f, -0.87f,		-0.17f, 0.0f, -0.83f,	0.3324, 0.0,
        -.34f, 1.0f, -0.94f,	-0.25f, 0.0f, -0.75f,	0.3047, 1.0,
        -.5f, 1.0f, -0.87f,		-0.25f, 0.0f, -0.75f,	0.3324, 1.0,
        -.5f, 0.0f, -0.87f,		-0.25f, 0.0f, -0.75f,	0.3324, 0.0,
        -.64f, 0.0f, -0.77f,	-0.33f, 0.0f, -0.67f,	0.3601, 0.0,
        -.5f, 1.0f, -0.87f,		-0.33f, 0.0f, -0.67f,	0.3324, 1.0,
        -.64f, 1.0f, -0.77f,	-0.33f, 0.0f, -0.67f,	0.3601, 1.0,
        -.64f, 0.0f, -0.77f,	-0.42f, 0.0f, -0.58f,	0.3601, 0.0,
        -.77f, 0.0f, -0.64f,	-0.42f, 0.0f, -0.58f,	0.3878, 0.0,
        -.64f, 1.0f, -0.77f,	-0.42f, 0.0f, -0.58f,	0.3601, 1.0,
        -.77f, 1.0f, -0.64f,	-0.5f, 0.0f, -0.5f,		0.3878, 1.0,
        -.77f, 0.0f, -0.64f,	-0.5f, 0.0f, -0.5f,		0.3878, 0.0,
        -.87f, 0.0f, -0.5f,		-0.5f, 0.0f, -0.5f,		0.4155, 0.0,
        -.77f, 1.0f, -0.64f,	-0.58f, 0.0f, -0.42f,	0.3878, 1.0,
        -.87f, 1.0f, -0.5f,		-0.58f, 0.0f, -0.42f,	0.4155, 1.0,
        -.87f, 0.0f, -0.5f,		-0.58f, 0.0f, -0.42f,	0.4155, 0.0,
        -.94f, 0.0f, -0.34f,	-0.67f, 0.0f, -0.33f,	0.4432, 0.0,
        -.87f, 1.0f, -0.5f,		-0.67f, 0.0f, -0.33f,	0.4155, 1.0,
        -.94f, 1.0f, -0.34f,	-0.67f, 0.0f, -0.33f,	0.4432, 1.0,
        -.94f, 0.0f, -0.34f,	-0.75f, 0.0f, -0.25f,	0.4432, 0.0,
        -.98f, 0.0f, -0.17f,	-0.75f, 0.0f, -0.25f,	0.4709, 0.0,
        -.94f, 1.0f, -0.34f,	-0.75f, 0.0f, -0.25f,	0.4432, 1.0,
        -.98f, 1.0f, -0.17f,	-0.83f, 0.0f, -0.17f,	0.4709, 1.0,
        -.98f, 0.0f, -0.17f,	-0.83f, 0.0f, -0.17f,	0.4709, 0.0,
        -1.0f, 0.0f, 0.0f,		-0.83f, 0.0f, -0.17f,	0.4986, 0.0,
        -.98f, 1.0f, -0.17f,	-0.92f, 0.0f, -0.08f,	0.4709, 1.0,
        -1.0f, 1.0f, 0.0f,		-0.92f, 0.0f, -0.08f,	0.4986, 1.0,
        -1.0f, 0.0f, 0.0f,		-0.92f, 0.0f, -0.08f,	0.4986, 0.0,
        -.98f, 0.0f, 0.17f,		-1.0f, 0.0f, 0.0f,		0.5263, 0.0,
        -1.0f, 1.0f, 0.0f,		-1.0f, 0.0f, 0.0f,		0.4986, 1.0,
        -.98f, 1.0f, 0.17f,		-1.0f, 0.0f, 0.0f,		0.5263, 1.0,
        -.98f, 0.0f, 0.17f,		-0.92f, 0.0f, 0.08f,	0.5263, 0.0,
        -.94f, 0.0f, 0.34f,		-0.92f, 0.0f, 0.08f,	0.554, 0.0,
        -.98f, 1.0f, 0.17f,		-0.92f, 0.0f, 0.08f,	0.5263, 1.0,
        -.94f, 1.0f, 0.34f,		-0.83f, 0.0f, 0.17f,	0.554, 1.0,
        -.94f, 0.0f, 0.34f,		-0.83f, 0.0f, 0.17f,	0.554, 0.0,
        -.87f, 0.0f, 0.5f,		-0.83f, 0.0f, 0.17f,	0.5817, 0.0,
        -.94f, 1.0f, 0.34f,		-0.75f, 0.0f, 0.25f,	0.554, 1.0,
        -.87f, 1.0f, 0.5f,		-0.75f, 0.0f, 0.25f,	0.5817, 1.0,
        -.87f, 0.0f, 0.5f,		-0.75f, 0.0f, 0.25f,	0.5817, 0.0,
        -.77f, 0.0f, 0.64f,		-0.67f, 0.0f, 0.33f,	0.6094, 0.0,
        -.87f, 1.0f, 0.5f,		-0.67f, 0.0f, 0.33f,	0.5817, 1.0,
        -.77f, 1.0f, 0.64f,		-0.67f, 0.0f, 0.33f,	0.6094, 1.0,
        -.77f, 0.0f, 0.64f,		-0.58f, 0.0f, 0.42f,	0.6094, 0.0,
        -.64f, 0.0f, 0.77f,		-0.58f, 0.0f, 0.42f,	0.6371, 0.0,
        -.77f, 1.0f, 0.64f,		-0.58f, 0.0f, 0.42f,	0.6094, 1.0,
        -.64f, 1.0f, 0.77f,		-0.5f, 0.0f, 0.5f,		0.6371, 1.0,
        -.64f, 0.0f, 0.77f,		-0.5f, 0.0f, 0.5f,		0.6371, 0.0,
        -.5f, 0.0f, 0.87f,		-0.5f, 0.0f, 0.5f,		0.6648, 0.0,
        -.64f, 1.0f, 0.77f,		-0.42f, 0.0f, 0.58f,	0.6371, 1.0,
        -.5f, 1.0f, 0.87f,		-0.42f, 0.0f, 0.58f,	0.6648, 1.0,
        -.5f, 0.0f, 0.87f,		-0.42f, 0.0f, 0.58f,	0.6648, 0.0,
        -.34f, 0.0f, 0.94f,		-0.33f, 0.0f, 0.67f,	0.6925, 0.0,
        -.5f, 1.0f, 0.87f,		-0.33f, 0.0f, 0.67f,	0.6648, 1.0,
        -.34f, 1.0f, 0.94f,		-0.33f, 0.0f, 0.67f,	0.6925, 1.0,
        -.34f, 0.0f, 0.94f,		-0.25f, 0.0f, 0.75f,	0.6925, 0.0,
        -.17f, 0.0f, 0.98f,		-0.25f, 0.0f, 0.75f,	0.7202, 0.0,
        -.34f, 1.0f, 0.94f,		-0.25f, 0.0f, 0.75f,	0.6925, 1.0,
        -.17f, 1.0f, 0.98f,		-0.17f, 0.0f, 0.83f,	0.7202, 1.0,
        -.17f, 0.0f, 0.98f,		-0.17f, 0.0f, 0.83f,	0.7202, 0.0,
        0.0f, 0.0f, 1.0f,		-0.17f, 0.0f, 0.83f,	0.7479, 0.0,
        -.17f, 1.0f, 0.98f,		-0.08f, 0.0f, 0.92f,	0.7202, 1.0,
        0.0f, 1.0f, 1.0f,		-0.08f, 0.0f, 0.92f,	0.7479, 1.0,
        0.0f, 0.0f, 1.0f,		-0.08f, 0.0f, 0.92f,	0.7479, 0.0,
        .17f, 0.0f, 0.98f,		-0.0f, 0.0f, 1.0f,		0.7756, 0.0,
        0.0f, 1.0f, 1.0f,		-0.0f, 0.0f, 1.0f,		0.7479, 1.0,
        .17f, 1.0f, 0.98f,		-0.0f, 0.0f, 1.0f,		0.7756, 1.0,
        .17f, 0.0f, 0.98f,		0.08f, 0.0f, 0.92f,		0.7756, 0.0,
        .34f, 0.0f, 0.94f,		0.08f, 0.0f, 0.92f,		0.8033, 0.0,
        .17f, 1.0f, 0.98f,		0.08f, 0.0f, 0.92f,		0.7756, 1.0,
        .34f, 1.0f, 0.94f,		0.17f, 0.0f, 0.83f,		0.8033, 1.0,
        .34f, 0.0f, 0.94f,		0.17f, 0.0f, 0.83f,		0.8033, 0.0,
        .5f, 0.0f, 0.87f,		0.17f, 0.0f, 0.83f,		0.831, 0.0,
        .34f, 1.0f, 0.94f,		0.25f, 0.0f, 0.75f,		0.8033, 1.0,
        .5f, 1.0f, 0.87f,		0.25f, 0.0f, 0.75f,		0.831, 1.0,
        .5f, 0.0f, 0.87f,		0.25f, 0.0f, 0.75f,		0.831, 0.0,
        .64f, 0.0f, 0.77f,		0.33f, 0.0f, 0.67f,		0.8587, 0.0,
        .5f, 1.0f, 0.87f,		0.33f, 0.0f, 0.67f,		0.831, 1.0,
        .64f, 1.0f, 0.77f,		0.33f, 0.0f, 0.67f,		0.8587, 1.0,
        .64f, 0.0f, 0.77f,		0.42f, 0.0f, 0.58f,		0.8587, 0.0,
        .77f, 0.0f, 0.64f,		0.42f, 0.0f, 0.58f,		0.8864, 0.0,
        .64f, 1.0f, 0.77f,		0.42f, 0.0f, 0.58f,		0.8587, 1.0,
        .77f, 1.0f, 0.64f,		0.5f, 0.0f, 0.5f,		0.8864, 1.0,
        .77f, 0.0f, 0.64f,		0.5f, 0.0f, 0.5f,		0.8864, 0.0,
        .87f, 0.0f, 0.5f,		0.5f, 0.0f, 0.5f,		0.9141, 0.0,
        .77f, 1.0f, 0.64f,		0.58f, 0.0f, 0.42f,		0.8864, 1.0,
        .87f, 1.0f, 0.5f,		0.58f, 0.0f, 0.42f,		0.9141, 1.0,
        .87f, 0.0f, 0.5f,		0.58f, 0.0f, 0.42f,		0.9141, 0.0,
        .94f, 0.0f, 0.34f,		0.67f, 0.0f, 0.33f,		0.9418, 0.0,
        .87f, 1.0f, 0.5f,		0.67f, 0.0f, 0.33f,		0.9141, 1.0,
        .94f, 1.0f, 0.34f,		0.67f, 0.0f, 0.33f,		0.9418, 1.0,
        .94f, 0.0f, 0.34f,		0.75f, 0.0f, 0.25f,		0.9418, 0.0,
        .98f, 0.0f, 0.17f,		0.75f, 0.0f, 0.25f,		0.9695, 0.0,
        .94f, 1.0f, 0.34f,		0.75f, 0.0f, 0.25f,		0.9418, 0.0,
        .98f, 1.0f, 0.17f,		0.83f, 0.0f, 0.17f,		0.9695, 1.0,
        .98f, 0.0f, 0.17f,		0.83f, 0.0f, 0.17f,		0.9695, 0.0,
        1.0f, 0.0f, 0.0f,		0.83f, 0.0f, 0.17f,		1.0, 0.0,
        .98f, 1.0f, 0.17f,		0.92f, 0.0f, 0.08f,		0.9695, 1.0,
        1.0f, 1.0f, 0.0f,		0.92f, 0.0f, 0.08f,		1.0, 1.0,
        1.0f, 0.0f, 0.0f,		0.92f, 0.0f, 0.08f,		1.0, 0.0
    };

    // total float values per each type
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    // store vertex and index count
    mesh.nIndices = 0;

    // Create VAO
    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Destroys a given mesh
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);               // Stores how many textures we want to generate
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

// Destroy Texture Program
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
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

// Destroy Shader program
void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
