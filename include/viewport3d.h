#ifndef VIEWPORT3D_H
#define VIEWPORT3D_H


#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_PNG_Image.H>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <FTGL/ftgl.h>

#include <string>
#include <vector>
#include <random>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <unordered_map>

#include "resource.h"

enum Tool {
    MOVE,
    ROTATE,
    SCALE,
    NONE
};


class Materialm {
public:
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    float shininess;

    GLuint diffuseMapTexture;
    GLuint specularMapTexture;
    GLuint normalMapTexture;
    GLuint cubemapTexture;

    std::string diffuseMap;
    std::string specularMap;
    std::string normalMap;

    bool useTexture;
    bool useMaterial;

    Materialm();

    void applyRandomColors();


    void setAlpha(float v);
    void apply() const;




};

class Mesh {
public:
    std::vector<float> vertices;
    std::vector<float> normals;
    //std::vector<unsigned int> indices;
    std::vector<float> colors;
    std::vector<glm::vec2> tverts;
    std::vector<glm::ivec3> faces;
    std::vector<glm::ivec3> uvFaces;
    std::vector<Materialm> materials;
    std::vector<int> faceMaterialIndices;
    GLuint VAO, VBO, EBO, UVVBO, UVVAO;
    glm::mat4 transform;
    bool isSelected;
    glm::vec3 minVertex;
    glm::vec3 maxVertex;
    bool showMaterial;
    bool showBackfaceCull;
    bool isTransparent;
    bool showBoundBox;
    bool showPivotAxis;

    enum RenderMode {
        Wireframe,
        EdgeWires,
        Vertices,
        Normals,
        Skin,
        Material,
        Faceted,
        BackfaceCull,
        XRay,
        Outline,
        Selected,
        Pivot,
        UV,
        Normal
    };

    std::vector<RenderMode> renderModes;

    Mesh();
    Mesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& faces, const std::vector<int>& materialIDs, const std::vector<glm::vec2>& tverts, const std::vector<Materialm>& materials, const std::vector<glm::vec3>& normals);

    void generateTeapot();
    void calculateNormals();
    void Sphere(unsigned int X_SEGMENTS = 32, unsigned int Y_SEGMENTS = 32);
    void calculateAABB();
    void addRenderMode(RenderMode mode);
    void removeRenderMode(RenderMode mode);
    void toggleMaterial();
    void toggleBackfaceCull();
    void setupMesh();
    void setupShader(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const std::vector<glm::vec3>& lightPositions, const std::vector<glm::vec3>& lightColors);
    void drawBoundingBox(const glm::mat4& view, const glm::mat4& projection);
    void drawGappedSegment(glm::vec3 start, glm::vec3 end, float gapPercentage);
    void drawPivot(const glm::mat4& view, const glm::mat4& projection, float lineLength = 0.6f);
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, GLuint shaderProgram, const std::vector<glm::vec3>& lightPositions, const std::vector<glm::vec3>& lightColors);

    // New methods for handling external mesh data
    void setVertices(const std::vector<glm::vec3>& verts);
    void setFaces(const std::vector<glm::ivec3>& fs);
    void setNormals(const std::vector<glm::vec3>& norms);
    void setTexCoords(const std::vector<glm::vec2>& texCoords);
    void setMaterials(const std::vector<Materialm>& mats);
    void setMaterialIDs(const std::vector<int>& matIDs);
    void generateNormals();
    void generateUVs();
    void validateFaceIndices();
    void loadTextures();
    int getVertexCount() const;
    int getFaceCount() const;

private:
    std::vector<float> convertVec3ToFloat(const std::vector<glm::vec3>& vec3Array);
    void validateIndices(const std::vector<glm::ivec3>& faces, size_t limit) const;
    void generateUVFaces();
    static const unsigned int teapotPatches[32][16];
    static const float teapotVertices[306][3];
    float bernstein(int i, int n, float t);
    glm::vec3 evaluateBezierPatch(const std::vector<glm::vec3>& controlPoints, float u, float v);
};

class MyGlWindow : public Fl_Gl_Window {

public:
    MyGlWindow(int x, int y, int w, int h, const char* l = 0);
    void initOpenGL();

    void render3D();
    void renderUVs();

    void draw() override;
    void orbit(float deltaX, float deltaY);
    void pan(float dx, float dy);
    void zoom(float delta);
    int handle(int event) override;
    void resize(int x, int y, int w, int h) override;

    bool drawGradientBackground;
    Mesh addMesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& faces, const std::vector<int>& materialIDs, const std::vector<glm::vec2>& tverts, const std::vector<Materialm>& materials, const std::vector<glm::vec3>& normals);
    void loadMeshTextures();
    void zoomExtents();
void setupMeshes();
private:
    GLuint gBuffer, gPosition, gNormal, gAlbedoSpec, rboDepth;
    GLuint gShaderProgram, lightingShaderProgram, lightBoxShaderProgram;
    GLuint gridShaderProgram, gridVBO, gridVAO, axisVBO, axisVAO;
    GLuint gradientShaderProgram, gradientVBO, gradientVAO, gradientEBO;
    GLuint lightShaderProgram, lightVBO, lightVAO, lightCircleVAO, lightCircleVBO;
    GLuint uvShaderProgram;
    std::vector<Mesh> meshes;
    glm::vec3 lightPos;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    float angleX, angleY, distance;
    glm::vec3 cameraPosition, cameraTarget, cameraUp;
    FTGLPixmapFont* font;
    bool showLegend;
    bool orthographic;
    bool isAltPressed;
    bool showGrid;
    bool showUVs;
    bool isCtrlPressed;
    bool drawingSelectionBox;
    glm::vec2 panOffset;
    int currentMaterialID;
    float zoomLevel;
    int selectionStartX, selectionStartY;
    int selectionEndX, selectionEndY;

    Tool currentTool;

    float fps;
    int frameCount;
    std::chrono::high_resolution_clock::time_point lastTime;

    static const char* gVertexShaderSource;
    static const char* gFragmentShaderSource;
    static const char* lightingVertexShaderSource;
    static const char* lightingFragmentShaderSource;
    static const char* lightBoxVertexShaderSource;
    static const char* lightBoxFragmentShaderSource;
    static const char* gridVertexShaderSource;
    static const char* gridFragmentShaderSource;
    static const char* gradientVertexShaderSource;
    static const char* gradientFragmentShaderSource;
    static const char* uvVertexShaderSource;
    static const char* uvFragmentShaderSource;

    int gridVertexCount;
    int axisVertexCount;

    void checkShaderCompileError(GLuint shader);
    void checkProgramLinkError(GLuint program);
    void compileShader(GLuint shader, const char* shaderSource);
    void linkProgram(GLuint program, GLuint vertexShader, GLuint fragmentShader);

    void setupGrid();
    void setupGradient();
    void setupLightHelper();
    void drawLightHelper(glm::mat4& view, glm::mat4& projection);
    void setupGBuffer();
    void initializeFont();
    void updateFPS();
    void renderText(const char* text, float x, float y, bool bold = false);
    void renderLegend();
    void drawSelectionRectangle();
    void selectObject(int x, int y);
    bool rayIntersectsMesh(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const Mesh& mesh, glm::vec3& intersectionPoint);
    bool rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& minVertex, const glm::vec3& maxVertex);
    bool rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, glm::vec3& intersectionPoint);
    void selectObjectsInRectangle();
    GLuint createProceduralCubemap();


};

#endif // VIEWPORT3D_H
