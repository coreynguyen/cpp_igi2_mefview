#include "viewport3d.h"
#include "Texture2D.h"
#include "resource.h"



#ifndef PI
#define PI 3.14159265358979323846
#endif


Materialm::Materialm()
    : ambient(glm::vec4(0.588f, 0.588f, 0.588f, 1.0f)),
      diffuse(glm::vec4(0.588f, 0.588f, 0.588f, 1.0f)),
      specular(glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)),
      shininess(0.75f),
      diffuseMapTexture(0),
      specularMapTexture(0),
      normalMapTexture(0),
      cubemapTexture(0),
      useTexture(true),
      useMaterial(true) {

    applyRandomColors();
}




void Materialm::applyRandomColors() {
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    ambient = glm::vec4(dist(gen), dist(gen), dist(gen), 1.0f);
    diffuse = glm::vec4(dist(gen), dist(gen), dist(gen), 1.0f);
    specular = glm::vec4(dist(gen), dist(gen), dist(gen), 1.0f);
    shininess = 0.6f;
}



void Materialm::setAlpha(float v) {
//    ambient.a = v;
//    diffuse.a = v;
//    specular.a = v;
}

void Materialm::apply() const {
    if (useMaterial) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, glm::value_ptr(ambient));
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, glm::value_ptr(diffuse));
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, glm::value_ptr(specular));
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * 128.0f);
    }

    if (useTexture) {
        if (diffuseMapTexture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMapTexture);
        }
        if (specularMapTexture != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularMapTexture);
        }
        if (normalMapTexture != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalMapTexture);
        }
    }

    if (cubemapTexture != 0) {
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_GEN_R);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_TEXTURE_CUBE_MAP);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
    }

    glActiveTexture(GL_TEXTURE0); // Always reset to GL_TEXTURE0
}




Mesh::Mesh() : showMaterial(true), showBackfaceCull(false), isSelected(false), transform(glm::mat4(1.0f)), isTransparent(false), showBoundBox(true), showPivotAxis(true) {
    addRenderMode(Normal);
    //generateTeapot();
    calculateAABB();
}

Mesh::Mesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& faces, const std::vector<int>& materialIDs,
    const std::vector<glm::vec2>& tverts, const std::vector<Materialm>& materials, const std::vector<glm::vec3>& normals)
    : vertices(convertVec3ToFloat(vertices)), faces(faces), tverts(tverts), uvFaces(faces), materials(materials), transform(glm::mat4(1.0f)), isSelected(false), isTransparent(false), showBoundBox(true), showPivotAxis(true) {


    if (this->tverts.empty()) {
        generateUVs();
    }

    if (this->uvFaces.empty()) {
        if (vertices.size() == tverts.size()) {
            this->uvFaces = faces;
        } else {
            generateUVFaces();
        }
    }

    if (!normals.empty()) {
        this->normals = convertVec3ToFloat(normals);
    } else {
        calculateNormals();
    }


    if (this->materials.empty()) {
        Materialm defaultMaterial;
        this->materials.push_back(defaultMaterial);
    }

    if (materialIDs.empty()) {
        this->faceMaterialIndices.resize(faces.size(), 0);
    } else {
        this->faceMaterialIndices.resize(faces.size());
        for (size_t i = 0; i < faces.size(); i++) {
            int index = materialIDs[i % materialIDs.size()];
            this->faceMaterialIndices[i] = (index >= 0 && index < static_cast<int>(this->materials.size())) ? index : 0;
        }
    }

    validateFaceIndices();
    calculateAABB();
    addRenderMode(Normal);
}

float Mesh::bernstein(int i, int n, float t) {
    float binomial_coeff = 1;
    for (int j = 0; j < i; ++j)
        binomial_coeff *= (float)(n - j) / (j + 1);
    return binomial_coeff * pow(t, i) * pow(1 - t, n - i);
}

glm::vec3 Mesh::evaluateBezierPatch(const std::vector<glm::vec3>& controlPoints, float u, float v) {
    glm::vec3 point(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            point += controlPoints[i * 4 + j] * bernstein(i, 3, u) * bernstein(j, 3, v);
        }
    }
    return point;
}

int Mesh::getVertexCount() const {
    return vertices.size() / 3;
}

int Mesh::getFaceCount() const {
    return faces.size();
}


const unsigned int Mesh::teapotPatches[32][16] = {
    {  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16},
    {  4,  17,  18,  19,   8,  20,  21,  22,  12,  23,  24,  25,  16,  26,  27,  28},
    { 19,  29,  30,  31,  22,  32,  33,  34,  25,  35,  36,  37,  28,  38,  39,  40},
    { 31,  41,  42,   1,  34,  43,  44,   5,  37,  45,  46,   9,  40,  47,  48,  13},
    { 13,  14,  15,  16,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60},
    { 16,  26,  27,  28,  52,  61,  62,  63,  56,  64,  65,  66,  60,  67,  68,  69},
    { 28,  38,  39,  40,  63,  70,  71,  72,  66,  73,  74,  75,  69,  76,  77,  78},
    { 40,  47,  48,  13,  72,  79,  80,  49,  75,  81,  82,  53,  78,  83,  84,  57},
    { 57,  58,  59,  60,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96},
    { 60,  67,  68,  69,  88,  97,  98,  99,  92, 100, 101, 102,  96, 103, 104, 105},
    { 69,  76,  77,  78,  99, 106, 107, 108, 102, 109, 110, 111, 105, 112, 113, 114},
    { 78,  83,  84,  57, 108, 115, 116,  85, 111, 117, 118,  89, 114, 119, 120,  93},
    {121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136},
    {124, 137, 138, 121, 128, 139, 140, 125, 132, 141, 142, 129, 136, 143, 144, 133},
    {133, 134, 135, 136, 145, 146, 147, 148, 149, 150, 151, 152,  69, 153, 154, 155},
    {136, 143, 144, 133, 148, 156, 157, 145, 152, 158, 159, 149, 155, 160, 161,  69},
    {162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177},
    {165, 178, 179, 162, 169, 180, 181, 166, 173, 182, 183, 170, 177, 184, 185, 174},
    {174, 175, 176, 177, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197},
    {177, 184, 185, 174, 189, 198, 199, 186, 193, 200, 201, 190, 197, 202, 203, 194},
    {204, 204, 204, 204, 207, 208, 209, 210, 211, 211, 211, 211, 212, 213, 214, 215},
    {204, 204, 204, 204, 210, 217, 218, 219, 211, 211, 211, 211, 215, 220, 221, 222},
    {204, 204, 204, 204, 219, 224, 225, 226, 211, 211, 211, 211, 222, 227, 228, 229},
    {204, 204, 204, 204, 226, 230, 231, 207, 211, 211, 211, 211, 229, 232, 233, 212},
    {212, 213, 214, 215, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245},
    {215, 220, 221, 222, 237, 246, 247, 248, 241, 249, 250, 251, 245, 252, 253, 254},
    {222, 227, 228, 229, 248, 255, 256, 257, 251, 258, 259, 260, 254, 261, 262, 263},
    {229, 232, 233, 212, 257, 264, 265, 234, 260, 266, 267, 238, 263, 268, 269, 242},
    {270, 270, 270, 270, 279, 280, 281, 282, 275, 276, 277, 278, 271, 272, 273, 274},
    {270, 270, 270, 270, 282, 289, 290, 291, 278, 286, 287, 288, 274, 283, 284, 285},
    {270, 270, 270, 270, 291, 298, 299, 300, 288, 295, 296, 297, 285, 292, 293, 294},
    {270, 270, 270, 270, 300, 305, 306, 279, 297, 303, 304, 275, 294, 301, 302, 271}
};

const float Mesh::teapotVertices[306][3] = {
    { 1.4000,  0.0000,  2.4000},
    { 1.4000, -0.7840,  2.4000},
    { 0.7840, -1.4000,  2.4000},
    { 0.0000, -1.4000,  2.4000},
    { 1.3375,  0.0000,  2.5312},
    { 1.3375, -0.7490,  2.5312},
    { 0.7490, -1.3375,  2.5312},
    { 0.0000, -1.3375,  2.5312},
    { 1.4375,  0.0000,  2.5312},
    { 1.4375, -0.8050,  2.5312},
    { 0.8050, -1.4375,  2.5312},
    { 0.0000, -1.4375,  2.5312},
    { 1.5000,  0.0000,  2.4000},
    { 1.5000, -0.8400,  2.4000},
    { 0.8400, -1.5000,  2.4000},
    { 0.0000, -1.5000,  2.4000},
    {-0.7840, -1.4000,  2.4000},
    {-1.4000, -0.7840,  2.4000},
    {-1.4000,  0.0000,  2.4000},
    {-0.7490, -1.3375,  2.5312},
    {-1.3375, -0.7490,  2.5312},
    {-1.3375,  0.0000,  2.5312},
    {-0.8050, -1.4375,  2.5312},
    {-1.4375, -0.8050,  2.5312},
    {-1.4375,  0.0000,  2.5312},
    {-0.8400, -1.5000,  2.4000},
    {-1.5000, -0.8400,  2.4000},
    {-1.5000,  0.0000,  2.4000},
    {-1.4000,  0.7840,  2.4000},
    {-0.7840,  1.4000,  2.4000},
    { 0.0000,  1.4000,  2.4000},
    {-1.3375,  0.7490,  2.5312},
    {-0.7490,  1.3375,  2.5312},
    { 0.0000,  1.3375,  2.5312},
    {-1.4375,  0.8050,  2.5312},
    {-0.8050,  1.4375,  2.5312},
    { 0.0000,  1.4375,  2.5312},
    {-1.5000,  0.8400,  2.4000},
    {-0.8400,  1.5000,  2.4000},
    { 0.0000,  1.5000,  2.4000},
    { 0.7840,  1.4000,  2.4000},
    { 1.4000,  0.7840,  2.4000},
    { 0.7490,  1.3375,  2.5312},
    { 1.3375,  0.7490,  2.5312},
    { 0.8050,  1.4375,  2.5312},
    { 1.4375,  0.8050,  2.5312},
    { 0.8400,  1.5000,  2.4000},
    { 1.5000,  0.8400,  2.4000},
    { 1.7500,  0.0000,  1.8750},
    { 1.7500, -0.9800,  1.8750},
    { 0.9800, -1.7500,  1.8750},
    { 0.0000, -1.7500,  1.8750},
    { 2.0000,  0.0000,  1.3500},
    { 2.0000, -1.1200,  1.3500},
    { 1.1200, -2.0000,  1.3500},
    { 0.0000, -2.0000,  1.3500},
    { 2.0000,  0.0000,  0.9000},
    { 2.0000, -1.1200,  0.9000},
    { 1.1200, -2.0000,  0.9000},
    { 0.0000, -2.0000,  0.9000},
    {-0.9800, -1.7500,  1.8750},
    {-1.7500, -0.9800,  1.8750},
    {-1.7500,  0.0000,  1.8750},
    {-1.1200, -2.0000,  1.3500},
    {-2.0000, -1.1200,  1.3500},
    {-2.0000,  0.0000,  1.3500},
    {-1.1200, -2.0000,  0.9000},
    {-2.0000, -1.1200,  0.9000},
    {-2.0000,  0.0000,  0.9000},
    {-1.7500,  0.9800,  1.8750},
    {-0.9800,  1.7500,  1.8750},
    { 0.0000,  1.7500,  1.8750},
    {-2.0000,  1.1200,  1.3500},
    {-1.1200,  2.0000,  1.3500},
    { 0.0000,  2.0000,  1.3500},
    {-2.0000,  1.1200,  0.9000},
    {-1.1200,  2.0000,  0.9000},
    { 0.0000,  2.0000,  0.9000},
    { 0.9800,  1.7500,  1.8750},
    { 1.7500,  0.9800,  1.8750},
    { 1.1200,  2.0000,  1.3500},
    { 2.0000,  1.1200,  1.3500},
    { 1.1200,  2.0000,  0.9000},
    { 2.0000,  1.1200,  0.9000},
    { 2.0000,  0.0000,  0.4500},
    { 2.0000, -1.1200,  0.4500},
    { 1.1200, -2.0000,  0.4500},
    { 0.0000, -2.0000,  0.4500},
    { 1.5000,  0.0000,  0.2250},
    { 1.5000, -0.8400,  0.2250},
    { 0.8400, -1.5000,  0.2250},
    { 0.0000, -1.5000,  0.2250},
    { 1.5000,  0.0000,  0.1500},
    { 1.5000, -0.8400,  0.1500},
    { 0.8400, -1.5000,  0.1500},
    { 0.0000, -1.5000,  0.1500},
    {-1.1200, -2.0000,  0.4500},
    {-2.0000, -1.1200,  0.4500},
    {-2.0000,  0.0000,  0.4500},
    {-0.8400, -1.5000,  0.2250},
    {-1.5000, -0.8400,  0.2250},
    {-1.5000,  0.0000,  0.2250},
    {-0.8400, -1.5000,  0.1500},
    {-1.5000, -0.8400,  0.1500},
    {-1.5000,  0.0000,  0.1500},
    {-2.0000,  1.1200,  0.4500},
    {-1.1200,  2.0000,  0.4500},
    { 0.0000,  2.0000,  0.4500},
    {-1.5000,  0.8400,  0.2250},
    {-0.8400,  1.5000,  0.2250},
    { 0.0000,  1.5000,  0.2250},
    {-1.5000,  0.8400,  0.1500},
    {-0.8400,  1.5000,  0.1500},
    { 0.0000,  1.5000,  0.1500},
    { 1.1200,  2.0000,  0.4500},
    { 2.0000,  1.1200,  0.4500},
    { 0.8400,  1.5000,  0.2250},
    { 1.5000,  0.8400,  0.2250},
    { 0.8400,  1.5000,  0.1500},
    { 1.5000,  0.8400,  0.1500},
    {-1.6000,  0.0000,  2.0250},
    {-1.6000, -0.3000,  2.0250},
    {-1.5000, -0.3000,  2.2500},
    {-1.5000,  0.0000,  2.2500},
    {-2.3000,  0.0000,  2.0250},
    {-2.3000, -0.3000,  2.0250},
    {-2.5000, -0.3000,  2.2500},
    {-2.5000,  0.0000,  2.2500},
    {-2.7000,  0.0000,  2.0250},
    {-2.7000, -0.3000,  2.0250},
    {-3.0000, -0.3000,  2.2500},
    {-3.0000,  0.0000,  2.2500},
    {-2.7000,  0.0000,  1.8000},
    {-2.7000, -0.3000,  1.8000},
    {-3.0000, -0.3000,  1.8000},
    {-3.0000,  0.0000,  1.8000},
    {-1.5000,  0.3000,  2.2500},
    {-1.6000,  0.3000,  2.0250},
    {-2.5000,  0.3000,  2.2500},
    {-2.3000,  0.3000,  2.0250},
    {-3.0000,  0.3000,  2.2500},
    {-2.7000,  0.3000,  2.0250},
    {-3.0000,  0.3000,  1.8000},
    {-2.7000,  0.3000,  1.8000},
    {-2.7000,  0.0000,  1.5750},
    {-2.7000, -0.3000,  1.5750},
    {-3.0000, -0.3000,  1.3500},
    {-3.0000,  0.0000,  1.3500},
    {-2.5000,  0.0000,  1.1250},
    {-2.5000, -0.3000,  1.1250},
    {-2.6500, -0.3000,  0.9375},
    {-2.6500,  0.0000,  0.9375},
    {-2.0000, -0.3000,  0.9000},
    {-1.9000, -0.3000,  0.6000},
    {-1.9000,  0.0000,  0.6000},
    {-3.0000,  0.3000,  1.3500},
    {-2.7000,  0.3000,  1.5750},
    {-2.6500,  0.3000,  0.9375},
    {-2.5000,  0.3000,  1.1250},
    {-1.9000,  0.3000,  0.6000},
    {-2.0000,  0.3000,  0.9000},
    { 1.7000,  0.0000,  1.4250},
    { 1.7000, -0.6600,  1.4250},
    { 1.7000, -0.6600,  0.6000},
    { 1.7000,  0.0000,  0.6000},
    { 2.6000,  0.0000,  1.4250},
    { 2.6000, -0.6600,  1.4250},
    { 3.1000, -0.6600,  0.8250},
    { 3.1000,  0.0000,  0.8250},
    { 2.3000,  0.0000,  2.1000},
    { 2.3000, -0.2500,  2.1000},
    { 2.4000, -0.2500,  2.0250},
    { 2.4000,  0.0000,  2.0250},
    { 2.7000,  0.0000,  2.4000},
    { 2.7000, -0.2500,  2.4000},
    { 3.3000, -0.2500,  2.4000},
    { 3.3000,  0.0000,  2.4000},
    { 1.7000,  0.6600,  0.6000},
    { 1.7000,  0.6600,  1.4250},
    { 3.1000,  0.6600,  0.8250},
    { 2.6000,  0.6600,  1.4250},
    { 2.4000,  0.2500,  2.0250},
    { 2.3000,  0.2500,  2.1000},
    { 3.3000,  0.2500,  2.4000},
    { 2.7000,  0.2500,  2.4000},
    { 2.8000,  0.0000,  2.4750},
    { 2.8000, -0.2500,  2.4750},
    { 3.5250, -0.2500,  2.4938},
    { 3.5250,  0.0000,  2.4938},
    { 2.9000,  0.0000,  2.4750},
    { 2.9000, -0.1500,  2.4750},
    { 3.4500, -0.1500,  2.5125},
    { 3.4500,  0.0000,  2.5125},
    { 2.8000,  0.0000,  2.4000},
    { 2.8000, -0.1500,  2.4000},
    { 3.2000, -0.1500,  2.4000},
    { 3.2000,  0.0000,  2.4000},
    { 3.5250,  0.2500,  2.4938},
    { 2.8000,  0.2500,  2.4750},
    { 3.4500,  0.1500,  2.5125},
    { 2.9000,  0.1500,  2.4750},
    { 3.2000,  0.1500,  2.4000},
    { 2.8000,  0.1500,  2.4000},
    { 0.0000,  0.0000,  3.1500},
    { 0.0000, -0.0020,  3.1500},
    { 0.0020,  0.0000,  3.1500},
    { 0.8000,  0.0000,  3.1500},
    { 0.8000, -0.4500,  3.1500},
    { 0.4500, -0.8000,  3.1500},
    { 0.0000, -0.8000,  3.1500},
    { 0.0000,  0.0000,  2.8500},
    { 0.2000,  0.0000,  2.7000},
    { 0.2000, -0.1120,  2.7000},
    { 0.1120, -0.2000,  2.7000},
    { 0.0000, -0.2000,  2.7000},
    {-0.0020,  0.0000,  3.1500},
    {-0.4500, -0.8000,  3.1500},
    {-0.8000, -0.4500,  3.1500},
    {-0.8000,  0.0000,  3.1500},
    {-0.1120, -0.2000,  2.7000},
    {-0.2000, -0.1120,  2.7000},
    {-0.2000,  0.0000,  2.7000},
    { 0.0000,  0.0020,  3.1500},
    {-0.8000,  0.4500,  3.1500},
    {-0.4500,  0.8000,  3.1500},
    { 0.0000,  0.8000,  3.1500},
    {-0.2000,  0.1120,  2.7000},
    {-0.1120,  0.2000,  2.7000},
    { 0.0000,  0.2000,  2.7000},
    { 0.4500,  0.8000,  3.1500},
    { 0.8000,  0.4500,  3.1500},
    { 0.1120,  0.2000,  2.7000},
    { 0.2000,  0.1120,  2.7000},
    { 0.4000,  0.0000,  2.5500},
    { 0.4000, -0.2240,  2.5500},
    { 0.2240, -0.4000,  2.5500},
    { 0.0000, -0.4000,  2.5500},
    { 1.3000,  0.0000,  2.5500},
    { 1.3000, -0.7280,  2.5500},
    { 0.7280, -1.3000,  2.5500},
    { 0.0000, -1.3000,  2.5500},
    { 1.3000,  0.0000,  2.4000},
    { 1.3000, -0.7280,  2.4000},
    { 0.7280, -1.3000,  2.4000},
    { 0.0000, -1.3000,  2.4000},
    {-0.2240, -0.4000,  2.5500},
    {-0.4000, -0.2240,  2.5500},
    {-0.4000,  0.0000,  2.5500},
    {-0.7280, -1.3000,  2.5500},
    {-1.3000, -0.7280,  2.5500},
    {-1.3000,  0.0000,  2.5500},
    {-0.7280, -1.3000,  2.4000},
    {-1.3000, -0.7280,  2.4000},
    {-1.3000,  0.0000,  2.4000},
    {-0.4000,  0.2240,  2.5500},
    {-0.2240,  0.4000,  2.5500},
    { 0.0000,  0.4000,  2.5500},
    {-1.3000,  0.7280,  2.5500},
    {-0.7280,  1.3000,  2.5500},
    { 0.0000,  1.3000,  2.5500},
    {-1.3000,  0.7280,  2.4000},
    {-0.7280,  1.3000,  2.4000},
    { 0.0000,  1.3000,  2.4000},
    { 0.2240,  0.4000,  2.5500},
    { 0.4000,  0.2240,  2.5500},
    { 0.7280,  1.3000,  2.5500},
    { 1.3000,  0.7280,  2.5500},
    { 0.7280,  1.3000,  2.4000},
    { 1.3000,  0.7280,  2.4000},
    { 0.0000,  0.0000,  0.0000},
    { 1.5000,  0.0000,  0.1500},
    { 1.5000,  0.8400,  0.1500},
    { 0.8400,  1.5000,  0.1500},
    { 0.0000,  1.5000,  0.1500},
    { 1.5000,  0.0000,  0.0750},
    { 1.5000,  0.8400,  0.0750},
    { 0.8400,  1.5000,  0.0750},
    { 0.0000,  1.5000,  0.0750},
    { 1.4250,  0.0000,  0.0000},
    { 1.4250,  0.7980,  0.0000},
    { 0.7980,  1.4250,  0.0000},
    { 0.0000,  1.4250,  0.0000},
    {-0.8400,  1.5000,  0.1500},
    {-1.5000,  0.8400,  0.1500},
    {-1.5000,  0.0000,  0.1500},
    {-0.8400,  1.5000,  0.0750},
    {-1.5000,  0.8400,  0.0750},
    {-1.5000,  0.0000,  0.0750},
    {-0.7980,  1.4250,  0.0000},
    {-1.4250,  0.7980,  0.0000},
    {-1.4250,  0.0000,  0.0000},
    {-1.5000, -0.8400,  0.1500},
    {-0.8400, -1.5000,  0.1500},
    { 0.0000, -1.5000,  0.1500},
    {-1.5000, -0.8400,  0.0750},
    {-0.8400, -1.5000,  0.0750},
    { 0.0000, -1.5000,  0.0750},
    {-1.4250, -0.7980,  0.0000},
    {-0.7980, -1.4250,  0.0000},
    { 0.0000, -1.4250,  0.0000},
    { 0.8400, -1.5000,  0.1500},
    { 1.5000, -0.8400,  0.1500},
    { 0.8400, -1.5000,  0.0750},
    { 1.5000, -0.8400,  0.0750},
    { 0.7980, -1.4250,  0.0000},
    { 1.4250, -0.7980,  0.0000}
};

void Mesh::generateTeapot() {
    // Parameters for tessellation
    const int divs = 10;
    const float step = 1.0f / divs;
    const float weldThreshold = 0.01f;

    static const int kTeapotNumPatches = 32;
    static const int kTeapotNumVertices = 306;

    // Temporary container to hold control points for a patch
    std::vector<glm::vec3> controlPoints(16);

    // Random number generator for colors
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Containers for vertices, normals, and their counts
    vertices.clear();
    normals.clear();
    faces.clear();
    colors.clear();

    std::vector<glm::vec3> tempVertices;
    std::vector<glm::vec3> tempNormals;

    // Loop over each patch
    for (int patch = 0; patch < kTeapotNumPatches; ++patch) {
        // Generate a random color for this patch
        float r = dis(gen);
        float g = dis(gen);
        float b = dis(gen);

        // Extract control points for this patch
        for (int i = 0; i < 16; ++i) {
            int index = teapotPatches[patch][i] - 1;
            controlPoints[i] = glm::vec3(teapotVertices[index][0], teapotVertices[index][1], teapotVertices[index][2]);
        }

        // Tessellate the patch
        for (int i = 0; i <= divs; ++i) {
            for (int j = 0; j <= divs; ++j) {
                float u = i * step;
                float v = j * step;
                glm::vec3 vertex = evaluateBezierPatch(controlPoints, u, v);

                // Rotate the vertex to align the teapot correctly
                glm::vec3 rotatedVertex = glm::vec3(vertex.x, vertex.z, -vertex.y);

                tempVertices.push_back(rotatedVertex);

                // Add initial zero normals and colors
                tempNormals.push_back(glm::vec3(0.0f));
                colors.push_back(r);
                colors.push_back(g);
                colors.push_back(b);
            }
        }

        // Generate indices for the tessellated patch
        for (int i = 0; i < divs; ++i) {
            for (int j = 0; j < divs; ++j) {
                int startIndex = patch * (divs + 1) * (divs + 1) + i * (divs + 1) + j;

                faces.push_back(glm::ivec3(startIndex, startIndex + divs + 1, startIndex + 1));
                faces.push_back(glm::ivec3(startIndex + 1, startIndex + divs + 1, startIndex + divs + 2));
            }
        }
    }

    // Weld vertices by proximity threshold
    std::vector<int> weldMap(tempVertices.size());
    std::vector<glm::vec3> weldedVertices;
    std::vector<glm::vec3> weldedNormals;
    std::vector<int> normalCounts;

    for (size_t i = 0; i < tempVertices.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < weldedVertices.size(); ++j) {
            if (glm::distance(tempVertices[i], weldedVertices[j]) < weldThreshold) {
                weldMap[i] = j;
                found = true;
                break;
            }
        }
        if (!found) {
            weldMap[i] = weldedVertices.size();
            weldedVertices.push_back(tempVertices[i]);
            weldedNormals.push_back(glm::vec3(0.0f));
            normalCounts.push_back(0);
        }
    }

    // Accumulate normals for each vertex
    for (const auto& face : faces) {
        glm::vec3 v0 = tempVertices[face.x];
        glm::vec3 v1 = tempVertices[face.y];
        glm::vec3 v2 = tempVertices[face.z];

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        if (glm::length(normal) > 0.0f) { // Check for degenerate faces
            for (int j = 0; j < 3; ++j) {
                int weldedIndex = weldMap[face[j]];
                weldedNormals[weldedIndex] += normal;
                normalCounts[weldedIndex] += 1;
            }
        }
    }

    // Average the normals
    for (size_t i = 0; i < weldedNormals.size(); ++i) {
        if (normalCounts[i] > 0) {
            weldedNormals[i] = glm::normalize(weldedNormals[i] / static_cast<float>(normalCounts[i]));
        }
    }

    // Store vertices and averaged normals
    vertices.reserve(weldedVertices.size() * 3);
    normals.reserve(weldedNormals.size() * 3);

    for (const auto& vertex : weldedVertices) {
        vertices.push_back(vertex.x);
        vertices.push_back(vertex.y);
        vertices.push_back(vertex.z);
    }

    for (const auto& normal : weldedNormals) {
        normals.push_back(-normal.x);  // Invert the normals
        normals.push_back(-normal.y);
        normals.push_back(-normal.z);
    }

    // Update faces to point to welded vertices
    for (auto& face : faces) {
        face = glm::ivec3(weldMap[face.x], weldMap[face.y], weldMap[face.z]);
    }
}

void Mesh::Sphere(unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS) {
    // Random number generator for colors
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = static_cast<float>(x) / static_cast<float>(X_SEGMENTS);
            float ySegment = static_cast<float>(y) / static_cast<float>(Y_SEGMENTS);
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            normals.push_back(xPos);
            normals.push_back(yPos);
            normals.push_back(zPos);

            colors.push_back(dis(gen));
            colors.push_back(dis(gen));
            colors.push_back(dis(gen));
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            unsigned int topLeft = y * (X_SEGMENTS + 1) + x;
            unsigned int bottomLeft = (y + 1) * (X_SEGMENTS + 1) + x;
            unsigned int bottomRight = (y + 1) * (X_SEGMENTS + 1) + x + 1;
            unsigned int topRight = y * (X_SEGMENTS + 1) + x + 1;

            faces.push_back(glm::ivec3(topLeft, bottomLeft, bottomRight));
            faces.push_back(glm::ivec3(topLeft, bottomRight, topRight));
        }
    }
}


void Mesh::calculateAABB() {
    if (vertices.size() < 3) {
        std::cerr << "Error: Cannot calculate AABB, vertices array does not have enough elements." << std::endl;
        return;
    }

    minVertex = glm::vec3(vertices[0], vertices[1], vertices[2]);
    maxVertex = glm::vec3(vertices[0], vertices[1], vertices[2]);

    for (size_t i = 3; i < vertices.size(); i += 3) {
        glm::vec3 currentVertex(vertices[i], vertices[i + 1], vertices[i + 2]);
        minVertex = glm::min(minVertex, currentVertex);
        maxVertex = glm::max(maxVertex, currentVertex);
    }
}


void Mesh::addRenderMode(RenderMode mode) {
    if (std::find(renderModes.begin(), renderModes.end(), mode) == renderModes.end()) {
        renderModes.push_back(mode);
    }
}

void Mesh::removeRenderMode(RenderMode mode) {
    auto it = std::find(renderModes.begin(), renderModes.end(), mode);
    if (it != renderModes.end()) {
        renderModes.erase(it);
    }
}

void Mesh::toggleMaterial() { showMaterial = !showMaterial; }
void Mesh::toggleBackfaceCull() { showBackfaceCull = !showBackfaceCull; }

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Prepare interleaved vertex data
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Color;
        glm::vec2 TexCoord;
        float MaterialID;
    };

    std::vector<Vertex> interleavedVertices;
    std::vector<unsigned int> indices;

    // Initialize default colors if not provided
    if (colors.empty()) {
        colors.resize(vertices.size() * 3, 1.0f); // RGB = (1.0, 1.0, 1.0)
    }

    // For each face, create vertices with per-face material ID
    for (size_t i = 0; i < faces.size(); ++i) {
        glm::ivec3 face = faces[i];
        int materialID = faceMaterialIndices[i];

        for (int j = 0; j < 3; ++j) {
            unsigned int vertexIndex = face[j];

            Vertex vertex;
            vertex.Position = glm::vec3(vertices[vertexIndex * 3], vertices[vertexIndex * 3 + 1], vertices[vertexIndex * 3 + 2]);
            vertex.Normal = glm::vec3(normals[vertexIndex * 3], normals[vertexIndex * 3 + 1], normals[vertexIndex * 3 + 2]);
            vertex.Color = glm::vec3(colors[vertexIndex * 3], colors[vertexIndex * 3 + 1], colors[vertexIndex * 3 + 2]);
            vertex.TexCoord = tverts[vertexIndex];
            vertex.MaterialID = static_cast<float>(materialID);

            interleavedVertices.push_back(vertex);
            indices.push_back(static_cast<unsigned int>(interleavedVertices.size() - 1));
        }
    }

    // Upload interleaved vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, interleavedVertices.size() * sizeof(Vertex), interleavedVertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Define vertex attribute pointers
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    // Color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(2);

    // TexCoord
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));
    glEnableVertexAttribArray(3);

    // MaterialID
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, MaterialID));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    // Error checking
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in setupMesh: " << error << std::endl;
    }
}








void Mesh::setupShader(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection,
                       const glm::vec3& cameraPos, const std::vector<glm::vec3>& lightPositions,
                       const std::vector<glm::vec3>& lightColors) {
    glUseProgram(shaderProgram);

    // Set transformation matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(transform));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Set camera position
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

    // Set lights
    for (unsigned int i = 0; i < lightPositions.size(); ++i) {
        std::string lightIndex = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (lightIndex + ".Position").c_str()), 1, glm::value_ptr(lightPositions[i]));
        glUniform3fv(glGetUniformLocation(shaderProgram, (lightIndex + ".Color").c_str()), 1, glm::value_ptr(lightColors[i]));
        glUniform1f(glGetUniformLocation(shaderProgram, (lightIndex + ".Linear").c_str()), 0.7f);
        glUniform1f(glGetUniformLocation(shaderProgram, (lightIndex + ".Quadratic").c_str()), 1.8f);
    }

    // Set material properties
    glUniform1f(glGetUniformLocation(shaderProgram, "metallicValue"), 0.5f);  // Adjust as needed
    glUniform1f(glGetUniformLocation(shaderProgram, "roughnessValue"), 0.5f); // Adjust as needed
    glUniform1f(glGetUniformLocation(shaderProgram, "aoValue"), 1.0f);        // Adjust as needed

    // Handle albedo (diffuse) texture
    const Materialm& material = !materials.empty() ? materials[0] : Materialm();
    if (material.diffuseMapTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.diffuseMapTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "albedoMap"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "useAlbedoMap"), GL_TRUE);
    } else {
        glUniform1i(glGetUniformLocation(shaderProgram, "useAlbedoMap"), GL_FALSE);
    }

    // Reset active texture
    glActiveTexture(GL_TEXTURE0);

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in setupShader: " << error << std::endl;
    }
}




void Mesh::drawBoundingBox(const glm::mat4& view, const glm::mat4& projection) {
    glm::vec3 corners[8] = {
        minVertex,
        glm::vec3(maxVertex.x, minVertex.y, minVertex.z),
        glm::vec3(maxVertex.x, maxVertex.y, minVertex.z),
        glm::vec3(minVertex.x, maxVertex.y, minVertex.z),
        glm::vec3(minVertex.x, minVertex.y, maxVertex.z),
        glm::vec3(maxVertex.x, minVertex.y, maxVertex.z),
        glm::vec3(maxVertex.x, maxVertex.y, maxVertex.z),
        glm::vec3(minVertex.x, maxVertex.y, maxVertex.z),
    };

    glUseProgram(0); // Disable shader program
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projection));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view * transform));
    glColor3f(1.0f, 1.0f, 1.0f); // Set color to white
    glLineWidth(2.0f);  // Set line thickness

    float gapPercentage = 0.6f; // Adjust this value as needed to get the desired gap size

    for (int i = 0; i < 4; ++i) {
        // Bottom edges
        drawGappedSegment(corners[i], corners[(i + 1) % 4], gapPercentage);
        // Top edges
        drawGappedSegment(corners[i + 4], corners[((i + 1) % 4) + 4], gapPercentage);
        // Vertical edges
        drawGappedSegment(corners[i], corners[i + 4], gapPercentage);
    }

    glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white
    glLineWidth(1.0f);  // Reset line thickness
}

void Mesh::drawGappedSegment(glm::vec3 start, glm::vec3 end, float gapPercentage) {
    glm::vec3 mid = (start + end) / 2.0f;
    glm::vec3 gapStart = glm::mix(start, mid, gapPercentage);
    glm::vec3 gapEnd = glm::mix(end, mid, gapPercentage);

    glBegin(GL_LINES);
    glVertex3fv(glm::value_ptr(start));
    glVertex3fv(glm::value_ptr(gapStart));
    glVertex3fv(glm::value_ptr(gapEnd));
    glVertex3fv(glm::value_ptr(end));
    glEnd();
}

void Mesh::drawPivot(const glm::mat4& view, const glm::mat4& projection, float lineLength) {
    // Save the current state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    glUseProgram(0);  // Disable shader program

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(view * transform));

    // Draw X-axis in red
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(lineLength, 0.0f, 0.0f);
    glEnd();

    // Draw Y-axis in green
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, lineLength, 0.0f);
    glEnd();

    // Draw Z-axis in blue
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, lineLength);
    glEnd();

    // Restore the state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopMatrix();
    glPopAttrib();
    }

void Mesh::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, GLuint shaderProgram, const std::vector<glm::vec3>& lightPositions, const std::vector<glm::vec3>& lightColors) {
    if (showBackfaceCull) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Apply materials and textures
    for (const auto& material : materials) {
        material.apply();
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUseProgram(shaderProgram);
    setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
    glBindVertexArray(VAO);

    if (isSelected) {
        addRenderMode(Selected);
        // Outline should be rendered first

        // Render outline by drawing the mesh slightly scaled up in wireframe mode
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glUseProgram(0);  // Disable shader program
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow color for outline
        glLineWidth(3.0f);  // Set outline thickness
        glm::mat4 outlineTransform = glm::scale(transform, glm::vec3(1.01f)); // Slightly scale up
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(projection));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(view * outlineTransform));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
    } else {
        removeRenderMode(Selected);
    }

    for (const auto& mode : renderModes) {
        if (mode == Outline) continue;
        switch (mode) {
            case Wireframe: {
                //std::cout << "Wires\n";
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDisable(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glUseProgram(0);  // Disable shader program

                glColor3f(1.0f, 1.0f, 1.0f); // Set color to white
                glLineWidth(0.5f);  // Set wire thickness

                glMatrixMode(GL_PROJECTION);
                glLoadMatrixf(glm::value_ptr(projection));
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(glm::value_ptr(view));

                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Restore the default polygon mode
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                break;
            }
            case EdgeWires: {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(1.0, 1.0);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                glDisable(GL_POLYGON_OFFSET_FILL);

                // Draw wireframe
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDisable(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glUseProgram(0);  // Disable shader program
                glColor3f(1.0f, 1.0f, 1.0f); // Set color to white
                glLineWidth(0.5f);  // Set wire thickness

                glMatrixMode(GL_PROJECTION);
                glLoadMatrixf(glm::value_ptr(projection));
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(glm::value_ptr(view * transform));

                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Restore the default polygon mode
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            }
            case Vertices: {
                // Disable shader program
                glUseProgram(0);

                // Set up the projection and modelview matrices
                glMatrixMode(GL_PROJECTION);
                glLoadMatrixf(glm::value_ptr(projection));
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(glm::value_ptr(view * transform));

                // Set the point size and color
                glColor3f(1.0f, 0.0f, 0.0f); // Red color
                glPointSize(5.0f); // Set point size

                // Draw vertices as points
                glBegin(GL_POINTS);
                for (size_t i = 0; i < vertices.size(); i += 3) {
                    glVertex3f(vertices[i], vertices[i + 1], vertices[i + 2]);
                }
                glEnd();

                // Reset point size and color
                glPointSize(1.0f);
                glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white
                break;
            }
            case Normals: {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Draw normals
                glUseProgram(0); // Disable shader program

                // Set up the projection and modelview matrices
                glMatrixMode(GL_PROJECTION);
                glLoadMatrixf(glm::value_ptr(projection));
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(glm::value_ptr(view * transform));

                // Draw blue lines for vertex normals
                glColor3f(0.0f, 0.0f, 1.0f); // Set color to blue
                glBegin(GL_LINES);
                for (size_t i = 0; i < vertices.size(); i += 3) {
                    glm::vec3 vertex(vertices[i], vertices[i + 1], vertices[i + 2]);
                    glm::vec3 normal(normals[i], normals[i + 1], normals[i + 2]);
                    glm::vec3 normalEnd = vertex + 0.1f * normal; // Short blue line

                    glVertex3f(vertex.x, vertex.y, vertex.z);
                    glVertex3f(normalEnd.x, normalEnd.y, normalEnd.z);
                }
                glEnd();
                glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white
                break;
            }
            case Faceted: {
                glShadeModel(GL_FLAT);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                glShadeModel(GL_SMOOTH);
                break;
            }
            case XRay: {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glUniform4f(glGetUniformLocation(shaderProgram, "colorOverride"), 0.5f, 0.5f, 0.5f, 0.5f); // Translucent grey
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                glUniform4f(glGetUniformLocation(shaderProgram, "colorOverride"), 1.0f, 1.0f, 1.0f, 1.0f); // Reset color override
                glDisable(GL_BLEND);
                break;
            }
            case Selected: {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                drawBoundingBox(view, projection);
                break;
            }
            case Pivot: {
                // Draw the mesh first
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Disable depth testing to ensure pivot is drawn on top
                glDisable(GL_DEPTH_TEST);
                drawPivot(view, projection);
                glEnable(GL_DEPTH_TEST); // Re-enable depth testing
                break;
            }
            case UV: {
                glBindVertexArray(UVVAO);
                glUseProgram(shaderProgram);
                glDrawArrays(GL_LINES, 0, tverts.size());
                glBindVertexArray(0);
                break;
            }
            case Normal: {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUseProgram(shaderProgram);
                setupShader(shaderProgram, view, projection, cameraPos, lightPositions, lightColors);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                break;
            }
            default:
                break;
        }
    }

    glBindVertexArray(0);

    // Restore the default polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


// New methods for handling external mesh data
void Mesh::setVertices(const std::vector<glm::vec3>& verts) {
    vertices = convertVec3ToFloat(verts);
}

void Mesh::setFaces(const std::vector<glm::ivec3>& fs) {
    faces = fs;
}


void Mesh::setNormals(const std::vector<glm::vec3>& norms) {
    normals = convertVec3ToFloat(norms);
}

void Mesh::setTexCoords(const std::vector<glm::vec2>& texCoords) {
    tverts = texCoords;
}

void Mesh::setMaterials(const std::vector<Materialm>& mats) {
    materials = mats;
}

void Mesh::setMaterialIDs(const std::vector<int>& matIDs) {
    faceMaterialIndices = matIDs;
}

void Mesh::generateNormals() {
    if (normals.empty()) {
        calculateNormals();
    }
}

void Mesh::generateUVs() {
    if (tverts.empty()) {
        tverts.resize(vertices.size() / 3);
        for (size_t i = 0; i < vertices.size(); i += 3) {
            tverts[i / 3] = glm::vec2(vertices[i], vertices[i + 1]);
        }
    }
}

void Mesh::validateFaceIndices() {
    validateIndices(faces, vertices.size());
    validateIndices(uvFaces, tverts.size());
}

void Mesh::loadTextures() {
    //std::cout << "NumMaterials: \t" << materials.size() << std::endl;
//    for (auto& material : materials) {
//        material.loadTextureFromMemory();
//        material.loadSpecularMapFromMemory();
//        material.loadNormalMapFromMemory();
//    }
}

void Mesh::validateIndices(const std::vector<glm::ivec3>& faces, size_t limit) const {
    for (const auto& face : faces) {
        for (int i = 0; i < 3; ++i) {
            if (face[i] < 0 || static_cast<size_t>(face[i]) >= limit) {
                std::cerr << "Error: Face index out of bounds. Index: " << face[i] << ", Limit: " << limit << std::endl;
                throw std::out_of_range("Face index out of bounds");
            }
        }
    }
}

void Mesh::generateUVFaces() {
    uvFaces.resize(faces.size());
    for (size_t i = 0; i < faces.size(); ++i) {
        uvFaces[i] = faces[i];
    }
}

void Mesh::calculateNormals() {
    // Updated implementation of calculateNormals() that uses faces instead of indices
    // Initialize normals to zero
    normals.resize(vertices.size(), 0.0f);

    // Create a vector to count the number of times each vertex is used in a face
    std::vector<int> normalCounts(vertices.size() / 3, 0);

    // Temporary containers for vertices and normals before welding
    std::vector<glm::vec3> tempVertices(vertices.size() / 3);
    std::vector<glm::vec3> tempNormals(vertices.size() / 3, glm::vec3(0.0f));

    // Populate tempVertices
    for (size_t i = 0; i < vertices.size(); i += 3) {
        tempVertices[i / 3] = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
    }

    // Calculate face normals and accumulate them to the vertices
    for (const auto& face : faces) {
        glm::vec3 v0 = tempVertices[face.x];
        glm::vec3 v1 = tempVertices[face.y];
        glm::vec3 v2 = tempVertices[face.z];

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        for (int i = 0; i < 3; ++i) {
            tempNormals[face[i]] += faceNormal;
            normalCounts[face[i]]++;
        }
    }

    // Average the normals
    for (size_t i = 0; i < tempNormals.size(); ++i) {
        if (normalCounts[i] > 0) {
            tempNormals[i] = glm::normalize(tempNormals[i] / static_cast<float>(normalCounts[i]));
        }
    }

    // Convert tempNormals back to the flattened normals vector
    for (size_t i = 0; i < tempNormals.size(); ++i) {
        normals[i * 3] = tempNormals[i].x;
        normals[i * 3 + 1] = tempNormals[i].y;
        normals[i * 3 + 2] = tempNormals[i].z;
    }
}


std::vector<float> Mesh::convertVec3ToFloat(const std::vector<glm::vec3>& vec3Array) {
    std::vector<float> floatArray;
    floatArray.reserve(vec3Array.size() * 3);
    for (const auto& vec : vec3Array) {
        floatArray.push_back(vec.x);
        floatArray.push_back(vec.y);
        floatArray.push_back(vec.z);
    }
    return floatArray;
}

const char* MyGlWindow::gVertexShaderSource = (R"(
#version 330 core

layout(location = 0) in vec3 aPos;        // Vertex position
layout(location = 1) in vec3 aNormal;     // Vertex normal
layout(location = 2) in vec3 aColor;      // Vertex color
layout(location = 3) in vec2 aTexCoord;   // Texture coordinates
layout(location = 4) in float aMaterialID; // Material ID

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    flat float MaterialID;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.TexCoord = aTexCoord;
    vs_out.MaterialID = aMaterialID;
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}

)");


const char* MyGlWindow::gFragmentShaderSource = (R"(
#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    flat float MaterialID;
} fs_in;

uniform vec3 viewPos;

struct Light {
    vec3 Position;
    vec3 Color;
    float Linear;
    float Quadratic;
};

uniform Light lights[4];

// Material properties
uniform sampler2D albedoMap;
uniform bool useAlbedoMap;

uniform float metallicValue;  // Metallic factor (0.0 = dielectric, 1.0 = metal)
uniform float roughnessValue; // Roughness factor (0.0 = smooth, 1.0 = rough)
uniform float aoValue;        // Ambient occlusion factor

const float PI = 3.14159265359;

// Function to generate a color based on MaterialID
vec3 materialColor(float id) {
    float x = fract(sin(id * 12.9898) * 43758.5453);
    float y = fract(sin((id + 1.0) * 78.233) * 43758.5453);
    float z = fract(sin((id + 2.0) * 39.425) * 43758.5453);
    return vec3(x, y, z);
}

// Normal mapping (if normal map is not used, just normalize the normal)
vec3 getNormalFromMap() {
    return normalize(fs_in.Normal);
}

// Distribution function (GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

// Geometry function (Smith)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel function (Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec3 N = getNormalFromMap();
    vec3 V = normalize(viewPos - fs_in.FragPos);

    // Retrieve material properties
    vec3 albedo = useAlbedoMap ? pow(texture(albedoMap, fs_in.TexCoord).rgb, vec3(2.2)) : materialColor(fs_in.MaterialID);
    float metallic  = metallicValue;
    float roughness = roughnessValue;
    float ao        = aoValue;

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) {
        // Calculate per-light radiance
        vec3 L = normalize(lights[i].Position - fs_in.FragPos);
        vec3 H = normalize(V + L);
        float distance    = length(lights[i].Position - fs_in.FragPos);
        float attenuation = 1.0 / (lights[i].Quadratic * distance * distance + lights[i].Linear * distance + 1.0);
        vec3 radiance     = lights[i].Color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // Prevent divide by zero
        vec3 specular     = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // kD is diffuse component and equals 1 - kS
        vec3 kD = vec3(1.0) - kS;
        // Multiply kD by (1 - metallic) to account for metalness
        kD *= 1.0 - metallic;

        // Scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // Add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient lighting (approximate)
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // Apply tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2)); // Gamma correction

    FragColor = vec4(color, 1.0);
}

)");

const char* MyGlWindow::lightingVertexShaderSource = (R"(
    #version 330 core
    layout(location = 0) in vec2 aTexCoords;
    out vec2 TexCoords;
    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aTexCoords * 2.0 - 1.0, 0.0, 1.0);
    }
)");

const char* MyGlWindow::lightingFragmentShaderSource = (R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D gPosition;
    uniform sampler2D gNormal;
    uniform sampler2D gAlbedoSpec;

    struct Light {
        vec3 Position;
        vec3 Color;
        float Linear;
        float Quadratic;
    };

    uniform Light lights[4];
    uniform vec3 viewPos;

    void main() {
        vec3 FragPos = texture(gPosition, TexCoords).rgb;
        vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
        vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
        float SpecularStrength = texture(gAlbedoSpec, TexCoords).a;

        vec3 lighting = Albedo * 0.1; // Hard-coded ambient component
        vec3 viewDir = normalize(viewPos - FragPos);
        for (int i = 0; i < 4; ++i) {
            vec3 lightDir = normalize(lights[i].Position - FragPos);
            float diff = max(dot(Normal, lightDir), 0.0);
            vec3 diffuse = lights[i].Color * diff * Albedo;

            vec3 reflectDir = reflect(-lightDir, Normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = lights[i].Color * spec * SpecularStrength;

            float distance = length(lights[i].Position - FragPos);
            float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * (distance * distance));

            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
        FragColor = vec4(lighting, 1.0);
    }
)");

const char* MyGlWindow::lightBoxVertexShaderSource = (R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)");

const char* MyGlWindow::lightBoxFragmentShaderSource = (R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 lightColor;
    void main() {
        FragColor = vec4(lightColor, 1.0);
    }
)");

const char* MyGlWindow::gridVertexShaderSource = (R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;

    out vec3 ourColor;

    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * vec4(aPos, 1.0);
        ourColor = aColor;
    }
)");

const char* MyGlWindow::gridFragmentShaderSource = (R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;

    void main() {
        FragColor = vec4(ourColor, 1.0);
    }
)");

const char* MyGlWindow::gradientVertexShaderSource = (R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;

    out vec3 ourColor;

    void main() {
        gl_Position = vec4(aPos, 1.0);
        ourColor = aColor;
    }
)");

const char* MyGlWindow::gradientFragmentShaderSource = (R"(
    #version 330 core
    out vec4 FragColor;
    in vec3 ourColor;

    void main() {
        FragColor = vec4(ourColor, 1.0);
    }
)");


const char* MyGlWindow::uvVertexShaderSource = (R"(
#version 330 core
layout(location = 0) in vec2 aTexCoord;
uniform mat4 projection;

void main() {
    // Apply the projection matrix and flip the V coordinate
    gl_Position = projection * vec4(aTexCoord.x, 1.0 - aTexCoord.y, 0.0, 1.0);
}
)");

const char* MyGlWindow::uvFragmentShaderSource = (R"(
#version 330 core
out vec4 FragColor;
uniform vec3 lineColor;

void main() {
    FragColor = vec4(lineColor, 1.0);  // Use uniform color for UV lines
}
)");




void MyGlWindow::checkShaderCompileError(GLuint shader) {
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << shader << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
    }
}

void MyGlWindow::checkProgramLinkError(GLuint program) {
    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << program << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
    }
}



void MyGlWindow::compileShader(GLuint shader, const char* shaderSource) {
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

void MyGlWindow::linkProgram(GLuint program, GLuint vertexShader, GLuint fragmentShader) {
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
}

void MyGlWindow::setupMeshes() {
    for (auto& mesh : meshes) {
        mesh.setupMesh();
    }
}

void MyGlWindow::setupGrid() {
    float length = 5.0f;
    float segments = 10.0f;
    float seglen = length / segments;
    float line_thickness = 1.0f;
    float axis_thickness = 2.0f;
    float epsilon = 0.0001f; // Small value to determine proximity to axis

    std::vector<float> gridVertices;
    std::vector<float> axisVertices;
    std::vector<float> gridColors;
    std::vector<float> axisColors;

    // Create grid lines parallel to the X-axis
    for (float i = -length; i <= length; i += seglen) {
        // Lines parallel to the X-axis
        if (std::abs(i) < epsilon) {
            // Axis lines (thicker and darker)
            axisVertices.insert(axisVertices.end(), { -length, 0.0f, i, length, 0.0f, i });
            axisColors.insert(axisColors.end(), { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        } else {
            // Regular grid lines
            gridVertices.insert(gridVertices.end(), { -length, 0.0f, i, length, 0.0f, i });
            gridColors.insert(gridColors.end(), { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f });
        }
    }

    // Create grid lines parallel to the Z-axis
    for (float i = -length; i <= length; i += seglen) {
        // Lines parallel to the Z-axis
        if (std::abs(i) < epsilon) {
            // Axis lines (thicker and darker)
            axisVertices.insert(axisVertices.end(), { i, 0.0f, -length, i, 0.0f, length });
            axisColors.insert(axisColors.end(), { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        } else {
            // Regular grid lines
            gridVertices.insert(gridVertices.end(), { i, 0.0f, -length, i, 0.0f, length });
            gridColors.insert(gridColors.end(), { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f });
        }
    }

    gridVertexCount = gridVertices.size() / 3;
    axisVertexCount = axisVertices.size() / 3;

    // Create and bind the grid VAO and VBO
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), &gridVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Create and bind the color VBO for grid lines
    GLuint gridColorVBO;
    glGenBuffers(1, &gridColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridColorVBO);
    glBufferData(GL_ARRAY_BUFFER, gridColors.size() * sizeof(float), &gridColors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Create and bind the axis VAO and VBO
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);
    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, axisVertices.size() * sizeof(float), &axisVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Create and bind the color VBO for axis lines
    GLuint axisColorVBO;
    glGenBuffers(1, &axisColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, axisColorVBO);
    glBufferData(GL_ARRAY_BUFFER, axisColors.size() * sizeof(float), &axisColors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void MyGlWindow::setupGradient() {
    float gradientVertices[] = {
        // Positions        // Colors
        -1.0f,  1.0f, 0.0f,  0.0862745f, 0.0862745f, 0.0862745f, // Top-left corner, dark grey
        -1.0f, -1.0f, 0.0f,  0.294118f,  0.294118f,  0.294118f,  // Bottom-left corner, light grey
         1.0f, -1.0f, 0.0f,  0.294118f,  0.294118f,  0.294118f,  // Bottom-right corner, light grey
         1.0f,  1.0f, 0.0f,  0.0862745f, 0.0862745f, 0.0862745f  // Top-right corner, dark grey
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &gradientVAO);
    glGenBuffers(1, &gradientVBO);
    glGenBuffers(1, &gradientEBO);
    glBindVertexArray(gradientVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gradientVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gradientVertices), gradientVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gradientEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void MyGlWindow::setupLightHelper() {
    float diamondVertices[] = {
        // Vertices of a diamond shape
        0.0f,  0.1f, 0.0f,
        0.1f,  0.0f, 0.0f,
        0.0f, -0.1f, 0.0f,
       -0.1f,  0.0f, 0.0f
    };

    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(diamondVertices), diamondVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Circle vertices
    std::vector<float> circleVertices;
    int numSegments = 36;
    float radius = 2.0f;
    for (int i = 0; i < numSegments; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(numSegments);
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        circleVertices.push_back(x);
        circleVertices.push_back(y);
        circleVertices.push_back(0.0f);
    }

    glGenVertexArrays(1, &lightCircleVAO);
    glGenBuffers(1, &lightCircleVBO);
    glBindVertexArray(lightCircleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCircleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), &circleVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void MyGlWindow::drawLightHelper(glm::mat4& view, glm::mat4& projection) {
    glUseProgram(lightShaderProgram);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), lightPos);
    glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(lightVAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glBindVertexArray(0);

    for (int i = 0; i < 3; ++i) {
        glm::mat4 rotation;
        if (i == 0) {
            rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        } else if (i == 1) {
            rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            rotation = glm::mat4(1.0f);
        }
        glm::mat4 transform = model * rotation;
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(transform));
        glBindVertexArray(lightCircleVAO);
        glDrawArrays(GL_LINE_LOOP, 0, 36);
        glBindVertexArray(0);
    }
}

void MyGlWindow::setupGBuffer() {
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // Position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w(), h(), 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // Normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w(), h(), 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // Color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w(), h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    // Create and attach depth buffer (renderbuffer)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w(), h());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // Finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyGlWindow::initializeFont() {
    fps = 0.0f;
    frameCount = 0;
    lastTime = std::chrono::high_resolution_clock::now();

    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(FONT_RESOURCE), RT_RCDATA);
    if (hResource) {
        DWORD fontSize = SizeofResource(NULL, hResource);
        HGLOBAL hMemory = LoadResource(NULL, hResource);
        if (hMemory) {
            void* pFontData = LockResource(hMemory);
            if (pFontData) {
                font = new FTGLPixmapFont(static_cast<const unsigned char*>(pFontData), fontSize);
                if (font->Error()) {
                    std::cerr << "Failed to load font from resource" << std::endl;
                    delete font;
                    font = nullptr;
                    exit(EXIT_FAILURE);
                }
                font->FaceSize(16);
                std::cout << "Font loaded successfully." << std::endl;
            } else {
                std::cerr << "Failed to lock resource" << std::endl;
            }
        } else {
            std::cerr << "Failed to load resource" << std::endl;
        }
    } else {
        std::cerr << "Failed to find resource" << std::endl;
    }
}

void MyGlWindow::updateFPS() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
    float deltaTime = duration.count() / 1000.0f;

    frameCount++;

    if (deltaTime >= 1.0f) {
        fps = frameCount / deltaTime;
        frameCount = 0;
        lastTime = now;
    }
}

void MyGlWindow::renderText(const char* text, float x, float y, bool bold) {
    if (font) {
        glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, w(), 0, h());
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glRasterPos2f(x, y);
        if (bold) {
            font->FaceSize(18);
        } else {
            font->FaceSize(16);
        }
        glColor3f(1.0f, 1.0f, 0.0f); // Set text color to yellow for vertex and face count
        font->Render(text);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();
    }
}

void MyGlWindow::renderLegend() {
    float y = h() - 20.0f; // Starting y position for the legend
    renderText("Hotkeys:", 10.0f, y);
    renderText("Ctrl+A - Select All", 10.0f, y - 20.0f);
    renderText("Ctrl+I - Invert Selection", 10.0f, y - 40.0f);
    renderText("H - Show Object List", 10.0f, y - 60.0f);
    renderText("W - Move Tool", 10.0f, y - 80.0f);
    renderText("E - Rotate Tool", 10.0f, y - 100.0f);
    renderText("R - Scale Tool", 10.0f, y - 120.0f);
    renderText("Z - Zoom Extents", 10.0f, y - 140.0f);
    renderText("Ctrl+Shift+Z - Zoom Extents All", 10.0f, y - 160.0f);
    renderText("P - Perspective View", 10.0f, y - 180.0f);
    renderText("F - Front View", 10.0f, y - 200.0f);
    renderText("T - Top View", 10.0f, y - 220.0f);
    renderText("L - Left View", 10.0f, y - 240.0f);
    renderText("B - Bottom View", 10.0f, y - 260.0f);
    renderText("Alt+B - Toggle Background", 10.0f, y - 280.0f);
    renderText("F3 - Wireframe Mode", 10.0f, y - 300.0f);
    renderText("F4 - Edged Faces Mode", 10.0f, y - 320.0f);
    renderText("G - Toggle Grid", 10.0f, y - 340.0f);
    renderText("I - Center Viewport to Mouse", 10.0f, y - 360.0f);
    renderText("J - Toggle Selection Brackets", 10.0f, y - 380.0f);
    renderText("U - Toggle Orthographic", 10.0f, y - 400.0f);
    renderText("X - Hide/Unhide Gizmo", 10.0f, y - 420.0f);
    renderText("7 - Polygon Count", 10.0f, y - 440.0f);
    renderText("Ctrl+Z - Undo", 10.0f, y - 460.0f);
    renderText("Ctrl+Y - Redo", 10.0f, y - 480.0f);
    renderText("Ctrl+LMB - Add to Selection", 10.0f, y - 500.0f);
    renderText("Alt+LMB - Remove from Selection", 10.0f, y - 520.0f);
    renderText("Shift+LMB - Clone and Move", 10.0f, y - 540.0f);
    renderText("Alt+X - Xray View Mode", 10.0f, y - 560.0f);
    renderText("MMB - Pan View", 10.0f, y - 580.0f);
    renderText("Alt+MMB - Orbit View", 10.0f, y - 600.0f);
    renderText("Ctrl+MMB - Fast Pan View", 10.0f, y - 620.0f);
}


MyGlWindow::MyGlWindow(int x, int y, int w, int h, const char* l)
    : Fl_Gl_Window(x, y, w, h, l), angleX(0.0f), angleY(0.0f), distance(5.0f), showLegend(false), orthographic(false), showGrid(true), drawGradientBackground(true), cameraPosition(glm::vec3(0.0f, 0.0f, 5.0f)), cameraTarget(glm::vec3(0.0f, 0.0f, 0.0f)), cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)), drawingSelectionBox(false), selectionStartX(0), selectionStartY(0), selectionEndX(0), selectionEndY(0), currentTool(NONE), showUVs(false), panOffset(glm::vec2(0.0f, 0.0f)), zoomLevel(1.0f), currentMaterialID(0) {




    // Mode settings for OpenGL
    mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE | FL_OPENGL3);

    // Calculate initial camera position (45 degrees to the right)
    float distanceFromMesh = 5.0f;  // Adjust this as necessary
    float angle = glm::radians(45.0f);
    glm::vec3 initialPosition = glm::vec3(
        distanceFromMesh * cos(angle),
        0.0f,  // Adjust the height if necessary
        distanceFromMesh * sin(angle)
    );

    // Set camera target to the center of the mesh
    glm::vec3 meshCenter = glm::vec3(0.0f, 0.0f, 0.0f);  // Adjust if your mesh is not centered at the origin

    // Set the up vector
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Update camera vectors
    cameraPosition = initialPosition;
    cameraTarget = meshCenter;
    cameraUp = upVector;
}

void MyGlWindow::initOpenGL() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    compileShader(vertexShader, gVertexShaderSource);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    compileShader(fragmentShader, gFragmentShaderSource);

    gShaderProgram = glCreateProgram();
    linkProgram(gShaderProgram, vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint lightingVertexShader = glCreateShader(GL_VERTEX_SHADER);
    compileShader(lightingVertexShader, lightingVertexShaderSource);

    GLuint lightingFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    compileShader(lightingFragmentShader, lightingFragmentShaderSource);

    lightingShaderProgram = glCreateProgram();
    linkProgram(lightingShaderProgram, lightingVertexShader, lightingFragmentShader);

    glDeleteShader(lightingVertexShader);
    glDeleteShader(lightingFragmentShader);

    GLuint lightBoxVertexShader = glCreateShader(GL_VERTEX_SHADER);
    compileShader(lightBoxVertexShader, lightBoxVertexShaderSource);

    GLuint lightBoxFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    compileShader(lightBoxFragmentShader, lightBoxFragmentShaderSource);

    lightBoxShaderProgram = glCreateProgram();
    linkProgram(lightBoxShaderProgram, lightBoxVertexShader, lightBoxFragmentShader);

    glDeleteShader(lightBoxVertexShader);
    glDeleteShader(lightBoxFragmentShader);

    GLuint gridVertexShader = glCreateShader(GL_VERTEX_SHADER);
    compileShader(gridVertexShader, gridVertexShaderSource);

    GLuint gridFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    compileShader(gridFragmentShader, gridFragmentShaderSource);

    gridShaderProgram = glCreateProgram();
    linkProgram(gridShaderProgram, gridVertexShader, gridFragmentShader);

    glDeleteShader(gridVertexShader);
    glDeleteShader(gridFragmentShader);

    GLuint gradientVertexShader = glCreateShader(GL_VERTEX_SHADER);
    compileShader(gradientVertexShader, gradientVertexShaderSource);

    GLuint gradientFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    compileShader(gradientFragmentShader, gradientFragmentShaderSource);

    gradientShaderProgram = glCreateProgram();
    linkProgram(gradientShaderProgram, gradientVertexShader, gradientFragmentShader);

    glDeleteShader(gradientVertexShader);
    glDeleteShader(gradientFragmentShader);

    // Compile and link UV shader program
    GLuint uvVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(uvVertexShader, 1, &uvVertexShaderSource, NULL);
    glCompileShader(uvVertexShader);
    checkShaderCompileError(uvVertexShader);

    GLuint uvFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(uvFragmentShader, 1, &uvFragmentShaderSource, NULL);
    glCompileShader(uvFragmentShader);
    checkShaderCompileError(uvFragmentShader);

    uvShaderProgram = glCreateProgram();
    glAttachShader(uvShaderProgram, uvVertexShader);
    glAttachShader(uvShaderProgram, uvFragmentShader);
    glLinkProgram(uvShaderProgram);
    checkProgramLinkError(uvShaderProgram);

    glDeleteShader(uvVertexShader);
    glDeleteShader(uvFragmentShader);


    meshes.push_back(Mesh()); // Add a default mesh for testing
    setupMeshes();
    setupGrid();
    setupGradient();
    setupLightHelper();
    setupGBuffer();


    glEnable(GL_DEPTH_TEST);

    lightPositions = {
        glm::vec3(2.0f, 4.0f, -2.0f),
        glm::vec3(-2.0f, 4.0f, -2.0f),
        glm::vec3(2.0f, 4.0f, 2.0f),
        glm::vec3(-2.0f, 4.0f, 2.0f)
    };

    lightColors = {
        glm::vec3(30.0f, 30.0f, 30.0f),
        glm::vec3(30.0f, 30.0f, 30.0f),
        glm::vec3(30.0f, 30.0f, 30.0f),
        glm::vec3(30.0f, 30.0f, 30.0f)
    };

    initializeFont();
}



void MyGlWindow::render3D() {

    glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    glm::mat4 projection;
    if (orthographic) {
        float orthoSize = distance;
        projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
    } else {
        projection = glm::perspective(glm::radians(45.0f), (float)w() / (float)h(), 0.1f, 100.0f);
    }
    glm::vec3 cameraPos = cameraPosition;

    // 1. Render gradient background
    if (drawGradientBackground) {
    glUseProgram(gradientShaderProgram);
    glBindVertexArray(gradientVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glClear(GL_DEPTH_BUFFER_BIT);
    }

    for (auto& mesh : meshes) {
        mesh.render(view, projection, cameraPos, gShaderProgram, lightPositions, lightColors);
    }

    // 2. Render grid
    if(showGrid) {
        glUseProgram(gridShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(gridVAO);
        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, gridVertexCount);
        glBindVertexArray(0);



        // 3. Render axes
        glUseProgram(gridShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(axisVAO);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, axisVertexCount);
        glBindVertexArray(0);
        }
    // Render the legend or help text
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (showLegend) {
        renderLegend();
    } else {
        renderText("Press F2 for Help", 10.0f, h() - 20.0f);
    }

    if (drawingSelectionBox) {
        drawSelectionRectangle();
    }




    // Update and render vertex, face counts, and FPS
    //updateFPS();
    std::stringstream ss;
    int totalVertices = 0;
    int totalFaces = 0;
    for (const auto& mesh : meshes) {
        totalVertices += mesh.getVertexCount();
        totalFaces += mesh.getFaceCount();
    }

        glColor3f(1.0f, 1.0f, 0.0f);  // RGB for yellow


    ss << "Vertices: " << totalVertices << " Faces: " << totalFaces;
    renderText(ss.str().c_str(), w() - font->Advance(ss.str().c_str()) - 10.0f, h() - 20.0f); // Render vertex and face count

    //ss.str(""); // Clear the stringstream
    //ss << "FPS: " << static_cast<int>(fps);
    //renderText(ss.str().c_str(), w() - 100.0f, h() - 20.0f, true); // Render FPS in bold

        // Reset color to white (optional, for other drawings)
        glColor3f(1.0f, 1.0f, 1.0f);



    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}




void MyGlWindow::renderUVs() {
    glClearColor(120.0f / 255.0f, 120.0f / 255.0f, 120.0f / 255.0f, 1.0f); // Grey background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST); // Disable depth test for 2D rendering

    // Get the window aspect ratio
    float aspect = (float)w() / (float)h();

    // Adjust the orthographic projection to maintain a square aspect ratio
    glm::mat4 fixedOrthoProjection;
    if (aspect > 1.0f) {
        fixedOrthoProjection = glm::ortho(
            -zoomLevel * aspect + panOffset.x, zoomLevel * aspect + panOffset.x,
            -zoomLevel + panOffset.y, zoomLevel + panOffset.y
        );
    } else {
        fixedOrthoProjection = glm::ortho(
            -zoomLevel + panOffset.x, zoomLevel + panOffset.x,
            -zoomLevel / aspect + panOffset.y, zoomLevel / aspect + panOffset.y
        );
    }

    glUseProgram(uvShaderProgram); // Use UV shader program

    // Set shader uniforms
    glUniformMatrix4fv(glGetUniformLocation(uvShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(fixedOrthoProjection));

    // Render infinite axes
    std::vector<glm::vec2> axesLines = {
        {-1000.0f, 0.0f}, {1000.0f, 0.0f}, // X axis
        {0.0f, -1000.0f}, {0.0f, 1000.0f}  // Y axis
    };

    GLuint axesVBO;
    glGenBuffers(1, &axesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, axesLines.size() * sizeof(glm::vec2), axesLines.data(), GL_STATIC_DRAW);

    // Set axis color to dark blue
    glUniform3f(glGetUniformLocation(uvShaderProgram, "lineColor"), 24.0f / 255.0f, 12.0f / 255.0f, 74.0f / 255.0f);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, axesLines.size());

    glDeleteBuffers(1, &axesVBO);

    // Render checkered grid
    std::vector<glm::vec2> checkeredGrid;
    for (float i = 0; i < 1.0f; i += 0.1f) {
        for (float j = 0; j < 1.0f; j += 0.1f) {
            checkeredGrid.push_back({i, j});
            checkeredGrid.push_back({i + 0.1f, j});
            checkeredGrid.push_back({i + 0.1f, j});
            checkeredGrid.push_back({i + 0.1f, j + 0.1f});
            checkeredGrid.push_back({i + 0.1f, j + 0.1f});
            checkeredGrid.push_back({i, j + 0.1f});
            checkeredGrid.push_back({i, j + 0.1f});
            checkeredGrid.push_back({i, j});
        }
    }

    GLuint gridVBO;
    glGenBuffers(1, &gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, checkeredGrid.size() * sizeof(glm::vec2), checkeredGrid.data(), GL_STATIC_DRAW);

    // Set grid line color to dark grey (67, 67, 67)
    glUniform3f(glGetUniformLocation(uvShaderProgram, "lineColor"), 67.0f / 255.0f, 67.0f / 255.0f, 67.0f / 255.0f);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, checkeredGrid.size());

    glDeleteBuffers(1, &gridVBO);

    // Render UVs
    for (auto& mesh : meshes) {
        if (mesh.isSelected && !mesh.tverts.empty()) {
            std::vector<glm::vec2> uvLines;
            std::vector<glm::vec2> uvBorders;
            std::vector<glm::vec2> uvVertices;

            for (size_t i = 0; i < mesh.faces.size(); ++i) {
                if (mesh.faceMaterialIndices[i] == currentMaterialID) {
                    const auto& face = mesh.uvFaces[i];
                    uvLines.push_back(mesh.tverts[face.x]);
                    uvLines.push_back(mesh.tverts[face.y]);
                    uvLines.push_back(mesh.tverts[face.y]);
                    uvLines.push_back(mesh.tverts[face.z]);
                    uvLines.push_back(mesh.tverts[face.z]);
                    uvLines.push_back(mesh.tverts[face.x]);

                    // Add logic to determine if an edge is a border
                    // Assuming an edge with no neighboring face is a border for now
                    // Check if edge face.x-face.y has a neighboring face
                    bool edge1Border = true;
                    bool edge2Border = true;
                    bool edge3Border = true;
                    for (size_t j = 0; j < mesh.faces.size(); ++j) {
                        if (i != j && mesh.faceMaterialIndices[j] == currentMaterialID) {
                            const auto& neighborFace = mesh.uvFaces[j];
                            if ((mesh.tverts[neighborFace.x] == mesh.tverts[face.x] && mesh.tverts[neighborFace.y] == mesh.tverts[face.y]) ||
                                (mesh.tverts[neighborFace.y] == mesh.tverts[face.x] && mesh.tverts[neighborFace.z] == mesh.tverts[face.y]) ||
                                (mesh.tverts[neighborFace.z] == mesh.tverts[face.x] && mesh.tverts[neighborFace.x] == mesh.tverts[face.y])) {
                                edge1Border = false;
                            }
                            if ((mesh.tverts[neighborFace.x] == mesh.tverts[face.y] && mesh.tverts[neighborFace.y] == mesh.tverts[face.z]) ||
                                (mesh.tverts[neighborFace.y] == mesh.tverts[face.y] && mesh.tverts[neighborFace.z] == mesh.tverts[face.z]) ||
                                (mesh.tverts[neighborFace.z] == mesh.tverts[face.y] && mesh.tverts[neighborFace.x] == mesh.tverts[face.z])) {
                                edge2Border = false;
                            }
                            if ((mesh.tverts[neighborFace.x] == mesh.tverts[face.z] && mesh.tverts[neighborFace.y] == mesh.tverts[face.x]) ||
                                (mesh.tverts[neighborFace.y] == mesh.tverts[face.z] && mesh.tverts[neighborFace.z] == mesh.tverts[face.x]) ||
                                (mesh.tverts[neighborFace.z] == mesh.tverts[face.z] && mesh.tverts[neighborFace.x] == mesh.tverts[face.x])) {
                                edge3Border = false;
                            }
                        }
                    }
                    if (edge1Border) {
                        uvBorders.push_back(mesh.tverts[face.x]);
                        uvBorders.push_back(mesh.tverts[face.y]);
                    }
                    if (edge2Border) {
                        uvBorders.push_back(mesh.tverts[face.y]);
                        uvBorders.push_back(mesh.tverts[face.z]);
                    }
                    if (edge3Border) {
                        uvBorders.push_back(mesh.tverts[face.z]);
                        uvBorders.push_back(mesh.tverts[face.x]);
                    }

                    uvVertices.push_back(mesh.tverts[face.x]);
                    uvVertices.push_back(mesh.tverts[face.y]);
                    uvVertices.push_back(mesh.tverts[face.z]);
                }
            }

            // Draw UV lines
            GLuint uvLinesVBO;
            glGenBuffers(1, &uvLinesVBO);
            glBindBuffer(GL_ARRAY_BUFFER, uvLinesVBO);
            glBufferData(GL_ARRAY_BUFFER, uvLines.size() * sizeof(glm::vec2), uvLines.data(), GL_STATIC_DRAW);

            // Set UV line color to white
            glUniform3f(glGetUniformLocation(uvShaderProgram, "lineColor"), 1.0f, 1.0f, 1.0f);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_LINES, 0, uvLines.size());

            glDeleteBuffers(1, &uvLinesVBO);

            // Draw UV borders
            GLuint uvBordersVBO;
            glGenBuffers(1, &uvBordersVBO);
            glBindBuffer(GL_ARRAY_BUFFER, uvBordersVBO);
            glBufferData(GL_ARRAY_BUFFER, uvBorders.size() * sizeof(glm::vec2), uvBorders.data(), GL_STATIC_DRAW);

            // Set UV border color to green
            glUniform3f(glGetUniformLocation(uvShaderProgram, "lineColor"), 0.0f, 1.0f, 0.0f);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_LINES, 0, uvBorders.size());

            glDeleteBuffers(1, &uvBordersVBO);

            // Draw UV vertices
            GLuint uvVerticesVBO;
            glGenBuffers(1, &uvVerticesVBO);
            glBindBuffer(GL_ARRAY_BUFFER, uvVerticesVBO);
            glBufferData(GL_ARRAY_BUFFER, uvVertices.size() * sizeof(glm::vec2), uvVertices.data(), GL_STATIC_DRAW);

            // Set UV vertex color to white
            glUniform3f(glGetUniformLocation(uvShaderProgram, "lineColor"), 1.0f, 1.0f, 1.0f);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, uvVertices.size());

            glDeleteBuffers(1, &uvVerticesVBO);

            glBindVertexArray(0);
        }
    }

    glEnable(GL_DEPTH_TEST); // Re-enable depth test
}


void MyGlWindow::draw() {
    if (!valid()) {
        initOpenGL();
    }

    glViewport(0, 0, w(), h());  // Ensure viewport is updated
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (showUVs) {
        renderUVs();
    } else {
        render3D();
    }

    glFlush();
    swap_buffers();
}


void MyGlWindow::orbit(float deltaX, float deltaY) {
    // Convert mouse delta to radians
    deltaX = glm::radians(deltaX);
    deltaY = glm::radians(deltaY);

    glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, cameraUp);

    glm::vec3 pivotView = glm::vec3(viewMatrix * glm::vec4(cameraTarget, 1.0f));

    // Rotation around the view space x axis
    glm::mat4 rotViewX = glm::rotate(glm::mat4(1.0f), deltaY, glm::vec3(1, 0, 0));
    glm::mat4 rotPivotViewX = glm::translate(glm::mat4(1.0f), pivotView) * rotViewX * glm::translate(glm::mat4(1.0f), -pivotView);

    // Rotation around the world space up vector (inverted for correct direction)
    glm::mat4 rotWorldUp = glm::rotate(glm::mat4(1.0f), -deltaX, glm::vec3(0, 1, 0));
    glm::mat4 rotPivotWorldUp = glm::translate(glm::mat4(1.0f), cameraTarget) * rotWorldUp * glm::translate(glm::mat4(1.0f), -cameraTarget);

    // Update view matrix
    viewMatrix = rotPivotViewX * viewMatrix * rotPivotWorldUp;

    // Decode eye, target, and up from view matrix
    glm::mat4 invViewMatrix = glm::inverse(viewMatrix);
    cameraPosition = glm::vec3(invViewMatrix[3]);
    cameraTarget = cameraPosition - glm::vec3(invViewMatrix[2]) * glm::length(cameraTarget - cameraPosition);
    cameraUp = glm::vec3(invViewMatrix[1]);

    redraw();
}

void MyGlWindow::pan(float dx, float dy) {
    glm::vec3 direction = glm::normalize(cameraPosition - cameraTarget);
    glm::vec3 right = glm::normalize(glm::cross(cameraUp, direction));
    glm::vec3 up = glm::cross(direction, right);

    cameraPosition += right * dx + up * dy;
    cameraTarget += right * dx + up * dy;

    redraw();
}

void MyGlWindow::zoom(float delta) {
    distance -= delta;
    if (distance < 1.0f) distance = 1.0f; // Prevent zooming too close
    cameraPosition = glm::normalize(cameraPosition - cameraTarget) * distance + cameraTarget;
    redraw();
}





void MyGlWindow::drawSelectionRectangle() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w(), 0, h(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(selectionStartX, h() - selectionStartY);
    glVertex2i(selectionEndX, h() - selectionStartY);
    glVertex2i(selectionEndX, h() - selectionEndY);
    glVertex2i(selectionStartX, h() - selectionEndY);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void MyGlWindow::selectObject(int x, int y) {
    // Convert screen coordinates to normalized device coordinates
    float ndcX = (2.0f * x) / w() - 1.0f;
    float ndcY = 1.0f - (2.0f * y) / h();
    glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

    // Convert to view space
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), (float)w() / (float)h(), 0.1f, 100.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Convert to world space
    glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
    rayWorld = glm::normalize(rayWorld);

    // Check for intersection with objects
    bool objectSelected = false;
    for (auto& mesh : meshes) {
        glm::vec3 intersectionPoint;
        if (rayIntersectsMesh(cameraPosition, rayWorld, mesh, intersectionPoint)) {
            mesh.isSelected = true;
            objectSelected = true;
        } else {
            if (!isCtrlPressed) {
                mesh.isSelected = false;
            }
        }
    }

    if (!objectSelected && !isCtrlPressed) {
        for (auto& mesh : meshes) {
            mesh.isSelected = false;
        }
    }
}


bool MyGlWindow::rayIntersectsMesh(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const Mesh& mesh, glm::vec3& intersectionPoint) {
    if (!rayIntersectsAABB(rayOrigin, rayDirection, mesh.minVertex, mesh.maxVertex)) {
        return false;
    }

    // If the ray intersects the bounding box, check for intersection with individual triangles
    for (const auto& face : mesh.faces) {
        glm::vec3 v0 = glm::vec3(mesh.vertices[face.x * 3], mesh.vertices[face.x * 3 + 1], mesh.vertices[face.x * 3 + 2]);
        glm::vec3 v1 = glm::vec3(mesh.vertices[face.y * 3], mesh.vertices[face.y * 3 + 1], mesh.vertices[face.y * 3 + 2]);
        glm::vec3 v2 = glm::vec3(mesh.vertices[face.z * 3], mesh.vertices[face.z * 3 + 1], mesh.vertices[face.z * 3 + 2]);

        if (rayIntersectsTriangle(rayOrigin, rayDirection, v0, v1, v2, intersectionPoint)) {
            return true;
        }
    }

    return false;
}



bool MyGlWindow::rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& minVertex, const glm::vec3& maxVertex) {
    float tmin = (minVertex.x - rayOrigin.x) / rayDirection.x;
    float tmax = (maxVertex.x - rayOrigin.x) / rayDirection.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (minVertex.y - rayOrigin.y) / rayDirection.y;
    float tymax = (maxVertex.y - rayOrigin.y) / rayDirection.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax)) return false;

    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (minVertex.z - rayOrigin.z) / rayDirection.z;
    float tzmax = (maxVertex.z - rayOrigin.z) / rayDirection.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax)) return false;

    return true;
}

bool MyGlWindow::rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, glm::vec3& intersectionPoint) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    // Compute triangle normal
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    // Check if the ray is facing the triangle
    if (glm::dot(normal, rayDirection) > 0) {
        return false; // Triangle is facing away from the ray
    }

    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) return false; // This ray is parallel to this triangle.

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDirection, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * glm::dot(edge2, q);
    if (t > EPSILON) { // ray intersection
        intersectionPoint = rayOrigin + rayDirection * t;
        return true;
    } else { // This means that there is a line intersection but not a ray intersection.
        return false;
    }
}


void MyGlWindow::selectObjectsInRectangle() {
    glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), (float)w() / (float)h(), 0.1f, 100.0f);

    // Define the selection rectangle in normalized device coordinates
    float xMin = std::min(selectionStartX, selectionEndX);
    float xMax = std::max(selectionStartX, selectionEndX);
    float yMin = std::min(selectionStartY, selectionEndY);
    float yMax = std::max(selectionStartY, selectionEndY);

    // Normalize coordinates
    xMin = (2.0f * xMin) / w() - 1.0f;
    xMax = (2.0f * xMax) / w() - 1.0f;
    yMin = 1.0f - (2.0f * yMin) / h();
    yMax = 1.0f - (2.0f * yMax) / h();

    // Iterate over all meshes
    for (auto& mesh : meshes) {
        bool isInside = false;

        // Check AABB corners
        glm::vec3 corners[8] = {
            mesh.minVertex,
            glm::vec3(mesh.maxVertex.x, mesh.minVertex.y, mesh.minVertex.z),
            glm::vec3(mesh.maxVertex.x, mesh.maxVertex.y, mesh.minVertex.z),
            glm::vec3(mesh.minVertex.x, mesh.maxVertex.y, mesh.minVertex.z),
            glm::vec3(mesh.minVertex.x, mesh.minVertex.y, mesh.maxVertex.z),
            glm::vec3(mesh.maxVertex.x, mesh.minVertex.y, mesh.maxVertex.z),
            glm::vec3(mesh.maxVertex.x, mesh.maxVertex.y, mesh.maxVertex.z),
            glm::vec3(mesh.minVertex.x, mesh.maxVertex.y, mesh.maxVertex.z),
        };

        for (int i = 0; i < 8; ++i) {
            glm::vec4 clipSpacePos = projectionMatrix * viewMatrix * glm::vec4(corners[i], 1.0f);
            glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

            if (ndcSpacePos.x >= xMin && ndcSpacePos.x <= xMax && ndcSpacePos.y >= yMin && ndcSpacePos.y <= yMax) {
                isInside = true;
                break;
            }
        }

        if (isInside) {
            mesh.isSelected = true;
        } else if (!isCtrlPressed) {
            mesh.isSelected = false;
        }
    }
}

int MyGlWindow::handle(int event) {

    int key = Fl::event_key();

    static int lastX, lastY;
    static bool middleMouseButton = false;



    static int currentUVChannel = 0;
    static int totalUVChannels = 2;

    switch (event) {
        case FL_PUSH: {

            if (Fl::event_button() == FL_MIDDLE_MOUSE) {
                middleMouseButton = true;
                lastX = Fl::event_x();
                lastY = Fl::event_y();
            } else if (Fl::event_button() == FL_LEFT_MOUSE) {
                if (!isCtrlPressed) {
                    for (auto& mesh : meshes) {
                        mesh.isSelected = false;
                    }
                }
                drawingSelectionBox = true;
                selectionStartX = Fl::event_x();
                selectionStartY = Fl::event_y();
                selectionEndX = selectionStartX;
                selectionEndY = selectionStartY;
                selectObject(selectionStartX, selectionStartY);
            }
            return 1;
        }

        case FL_DRAG: {

            if (middleMouseButton) {
                int dx = Fl::event_x() - lastX;
                int dy = Fl::event_y() - lastY;

                if (showUVs) {
                    panOffset += glm::vec2(-dx * 0.007f,- dy * -0.007f);
                } else if (!showUVs && isAltPressed) {
                    orbit(-dx, dy);
                } else {
                    pan(-dx * 0.01f, dy * 0.01f);
                }
                lastX = Fl::event_x();
                lastY = Fl::event_y();
                redraw();
            } else if (drawingSelectionBox) {
                selectionEndX = Fl::event_x();
                selectionEndY = Fl::event_y();
                redraw(); // Ensure redraw is called to update the selection rectangle
            }
            return 1;
        }
        case FL_RELEASE: {
            if (Fl::event_button() == FL_MIDDLE_MOUSE) {
                middleMouseButton = false;
            } else if (Fl::event_button() == FL_LEFT_MOUSE) {
                drawingSelectionBox = false;
                selectObjectsInRectangle();
                redraw(); // Ensure redraw is called to clear the selection rectangle
            }
            return 1;
        }



        case FL_MOUSEWHEEL: {
            if (showUVs) {
                zoomLevel = glm::clamp(zoomLevel + (Fl::event_dy() * 0.1f), 0.1f, 10.0f);
            } else {
                zoom((Fl::event_dy() * -0.1f) * glm::length(cameraPosition - cameraTarget));
            }
            redraw();
            return 1;
        }
        case FL_SHORTCUT: {
            switch (key) {

                case FL_Page_Up:
                    currentMaterialID = currentMaterialID + 1;

                    break;
                case FL_Page_Down:
                    currentMaterialID = min(currentMaterialID - 1, 0);
                    break;
                case FL_Alt_L:
                    isAltPressed = true;
                    break;
                case FL_Control_L:
                    isCtrlPressed = true;
                    break;
                case (FL_F + 2): // F2
                    showLegend = !showLegend;
                    break;
                case FL_Escape:
                    exit(0);
                    break;
                case 'b':
                case 'B':
                    drawGradientBackground = !drawGradientBackground;
                    break;
                case 'g':
                case 'G':
                    showGrid = !showGrid;
                    break;
                case 'u':
                case 'U':
                    orthographic = !orthographic;
                    break;
                case 'w':
                case 'W':
                    currentTool = MOVE;
                    break;
                case 'E':
                    currentTool = ROTATE;
                    break;
                case 'R':
                    currentTool = SCALE;
                    break;
                case 'j':
                case 'J':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Selected) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Selected);
                        } else {
                            mesh.addRenderMode(Mesh::Selected);
                        }
                    }
                    break;
                case '1':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Wireframe) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Wireframe);
                            mesh.addRenderMode(Mesh::Normal);
                        } else {
                            mesh.addRenderMode(Mesh::Wireframe);
                            mesh.removeRenderMode(Mesh::Normal);
                        }
                    }
                    break;
                case '2':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::EdgeWires) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::EdgeWires);
                        } else {
                            mesh.addRenderMode(Mesh::EdgeWires);
                        }
                    }
                    break;
                case '3':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Vertices) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Vertices);
                            mesh.addRenderMode(Mesh::Normal);
                            mesh.removeRenderMode(Mesh::Wireframe);
                        } else {
                            mesh.addRenderMode(Mesh::Vertices);
                            mesh.removeRenderMode(Mesh::Normal);
                            mesh.addRenderMode(Mesh::Wireframe);
                        }
                    }
                    break;
                case '4':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Normals) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Normals);
                        } else {
                            mesh.addRenderMode(Mesh::Normals);
                        }
                    }
                    break;
                case '5':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Skin) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Skin);
                        } else {
                            mesh.addRenderMode(Mesh::Skin);
                        }
                    }
                    break;
                case '6':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Material) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Material);
                        } else {
                            mesh.addRenderMode(Mesh::Material);
                        }
                    }
                    break;
                case '7':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Faceted) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Faceted);
                        } else {
                            mesh.addRenderMode(Mesh::Faceted);
                        }
                    }
                    break;
                case '8':
                    for (auto& mesh : meshes) {
                        mesh.toggleBackfaceCull();
                    }
                    break;
                case '9':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::XRay) != mesh.renderModes.end()) {
                            mesh.addRenderMode(Mesh::Normal);
                            mesh.removeRenderMode(Mesh::XRay);
                        } else {
                            mesh.removeRenderMode(Mesh::Normal);
                            mesh.addRenderMode(Mesh::XRay);
                        }
                    }
                    break;
                case '0':
                    currentMaterialID = 0;
                    showUVs = !showUVs;
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::UV) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::UV);
                        } else {
                            mesh.addRenderMode(Mesh::UV);
                        }
                    }
                    break;
                case 'p':
                case 'P':
                    for (auto& mesh : meshes) {
                        if (std::find(mesh.renderModes.begin(), mesh.renderModes.end(), Mesh::Pivot) != mesh.renderModes.end()) {
                            mesh.removeRenderMode(Mesh::Pivot);
                        } else {
                            mesh.addRenderMode(Mesh::Pivot);
                        }
                    }
                    break;
            }
            redraw();
        }
        case FL_KEYDOWN: {
            if (key == FL_Alt_L) {
                isAltPressed = true;
            }
            if (key == FL_Control_L) {
                isCtrlPressed = true;
            }

            return 1;
        }

        case FL_KEYUP: {
            if (Fl::event_key() == FL_Alt_L) {
                isAltPressed = false;
            } else if (Fl::event_key() == FL_Control_L) {
                isCtrlPressed = false;
            }
            return 1;
        }
    }
    return Fl_Gl_Window::handle(event);
}


void MyGlWindow::resize(int x, int y, int w, int h) {
    Fl_Gl_Window::resize(x, y, w, h);
    glViewport(0, 0, w, h);
    redraw();
}

Mesh MyGlWindow::addMesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& faces, const std::vector<int>& materialIDs, const std::vector<glm::vec2>& tverts, const std::vector<Materialm>& materials, const std::vector<glm::vec3>& normals) {
    // Validate vertices
    if (vertices.empty()) {
        std::cerr << "Error: Vertices array is empty." << std::endl;
        return Mesh(); // Return an empty mesh or handle error as needed
    }

    // Validate faces
    if (faces.empty()) {
        std::cerr << "Error: Faces array is empty." << std::endl;
        return Mesh(); // Return an empty mesh or handle error as needed
    }

    // Validate material IDs
    for (int id : materialIDs) {
        if (id < 0 || id >= static_cast<int>(materials.size())) {
            std::cerr << "Error: Invalid material ID: " << id << std::endl;
            return Mesh(); // Return an empty mesh or handle error as needed
        }
    }

    // Validate texture vertices
    if (!tverts.empty() && tverts.size() != vertices.size()) {
        std::cerr << "Error: Texture vertices array size does not match vertices array size." << std::endl;
        return Mesh(); // Return an empty mesh or handle error as needed
    }

    // Validate normals
    if (!normals.empty() && normals.size() != vertices.size()) {
        std::cerr << "Error: Normals array size does not match vertices array size." << std::endl;
        return Mesh(); // Return an empty mesh or handle error as needed
    }

    // Create and add the new mesh
    Mesh newMesh(vertices, faces, materialIDs, tverts, materials, normals);

    // Store the new mesh
    meshes.push_back(newMesh);
    return newMesh;
}

void MyGlWindow::loadMeshTextures() {
    make_current(); // Ensure context is current before loading textures
            //std::cout << "NumMeshes: \t" << meshes.size() << std::endl;
    for (auto& mesh : meshes) {
        mesh.loadTextures();
    }
}

void MyGlWindow::zoomExtents() {
    if (meshes.empty()) return;

    // Initialize bounding box with the first mesh
    glm::vec3 minVertex = meshes[0].minVertex;
    glm::vec3 maxVertex = meshes[0].maxVertex;

    // Extend the bounding box to include all meshes
    for (const auto& mesh : meshes) {
        minVertex = glm::min(minVertex, mesh.minVertex);
        maxVertex = glm::max(maxVertex, mesh.maxVertex);
    }

    // Calculate the center and size of the bounding box
    glm::vec3 sceneCenter = (minVertex + maxVertex) * 0.5f;
    glm::vec3 sceneSize = maxVertex - minVertex;
    float sceneRadius = glm::length(sceneSize) * 0.5f;

    // Calculate the required distance to fit the scene within the view frustum
    float fovY = glm::radians(45.0f); // Adjust based on your field of view
    float aspectRatio = static_cast<float>(w()) / static_cast<float>(h());
    float distance = sceneRadius / std::tan(fovY / 2.0f);

    // Maintain the camera orientation while adjusting the distance
    glm::vec3 direction = glm::normalize(cameraTarget - cameraPosition);
    cameraPosition = sceneCenter - direction * distance;
    cameraTarget = sceneCenter;

    redraw(); // Redraw the scene with the updated camera
}

GLuint MyGlWindow::createProceduralCubemap() {
    const int faceSize = 512;
    GLubyte data[faceSize][faceSize][4]; // Use 4 channels: RGBA

    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

    for (GLuint i = 0; i < 6; ++i) {
        // Clear the face to black and fully transparent
        for (int y = 0; y < faceSize; ++y) {
            for (int x = 0; x < faceSize; ++x) {
                data[y][x][0] = 0;
                data[y][x][1] = 0;
                data[y][x][2] = 0;
                data[y][x][3] = 0;
            }
        }

        // Draw a small spec ball on face 0
        if (i == 0) {
            const int dotX = faceSize / 2;
            const int dotY = faceSize / 2;
            const int dotRadius = 50;

            for (int y = 0; y < faceSize; ++y) {
                for (int x = 0; x < faceSize; ++x) {
                    // Calculate distance from the center of the dot
                    int dx = x - dotX;
                    int dy = y - dotY;
                    float distance = sqrt(dx * dx + dy * dy);

                    // Set pixel color and alpha based on distance to the center of the dot
                    if (distance <= dotRadius) {
                        GLubyte intensity = 255 - static_cast<GLubyte>((distance / dotRadius) * 255);
                        data[y][x][0] = intensity;
                        data[y][x][1] = intensity;
                        data[y][x][2] = intensity;
                        data[y][x][3] = 255; // Fully opaque within the dot radius
                    }
                }
            }
        }

        // Draw a large spec ball on face 1
        if (i == 1) {
            const int dotX = faceSize / 2;
            const int dotY = faceSize / 2;
            const int dotRadius = 200;

            for (int y = 0; y < faceSize; ++y) {
                for (int x = 0; x < faceSize; ++x) {
                    // Calculate distance from the center of the dot
                    int dx = x - dotX;
                    int dy = y - dotY;
                    float distance = sqrt(dx * dx + dy * dy);

                    // Set pixel color and alpha based on distance to the center of the dot
                    if (distance <= dotRadius) {
                        GLubyte intensity = 255 - static_cast<GLubyte>((distance / dotRadius) * 255);
                        data[y][x][0] = intensity;
                        data[y][x][1] = intensity;
                        data[y][x][2] = intensity;
                        data[y][x][3] = 255; // Fully opaque within the dot radius
                    }
                }
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, faceSize, faceSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return cubemap;
}

