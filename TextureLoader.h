#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include <QImage>
#include <QOpenGLFunctions>

#include <glm/glm.hpp>
#include <GL/gl.h>

#include <array>
#include <vector>

struct UV {
    float u;
    float v;
};

// Returns 4 UVs in the order: bottom-left, bottom-right, top-right, top-left
std::vector<UV> getTileUVs(
    int tileIndex,
    int tileWidth,
    int tileHeight,
    int atlasWidth,
    int atlasHeight
    ) {
    int tilesPerRow = atlasWidth / tileWidth;
    int col = tileIndex % tilesPerRow;
    int row = tileIndex / tilesPerRow;

    float uMin = col * tileWidth / static_cast<float>(atlasWidth);
    float vMin = row * tileHeight / static_cast<float>(atlasHeight);
    float uMax = (col + 1) * tileWidth / static_cast<float>(atlasWidth);
    float vMax = (row + 1) * tileHeight / static_cast<float>(atlasHeight);

    // Flip vertically to match OpenGL
    vMin = 1.0f - vMin;
    vMax = 1.0f - vMax;
    std::swap(vMin, vMax);

    return {
        {uMin, vMin}, // bottom-left
        {uMax, vMin}, // bottom-right
        {uMax, vMax}, // top-right
        {uMin, vMax}  // top-left
    };
}

struct Vertex {
    float x, y, z;
    UV uv;
};

std::vector<Vertex> getVertexes() {

    // Centered cube with size 1.0
    float size = 0.5f;
    std::vector<Vertex> cubeVertices;

    int faceTiles[6] = {0, 1, 2, 3, 4, 5}; // tile indices for each face

    // Utility lambda to push a quad face with its UVs
    auto addFace = [&](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, std::vector<UV>& uvs) {
        cubeVertices.push_back({v0.x, v0.y, v0.z, {uvs[0].u, uvs[0].v}});
        cubeVertices.push_back({v1.x, v1.y, v1.z, {uvs[1].u, uvs[1].v}});
        cubeVertices.push_back({v2.x, v2.y, v2.z, {uvs[2].u, uvs[2].v}});

        cubeVertices.push_back({v0.x, v0.y, v0.z, {uvs[0].u, uvs[0].v}});
        cubeVertices.push_back({v2.x, v2.y, v2.z, {uvs[2].u, uvs[2].v}});
        cubeVertices.push_back({v3.x, v3.y, v3.z, {uvs[3].u, uvs[3].v}});
    };

    // Define cube face vertices in counter-clockwise order (OpenGL default)
    std::array<std::array<glm::vec3, 4>, 6> faceVertices = {
        // Front
        std::array<glm::vec3, 4>{glm::vec3(-size, -size,  size), glm::vec3( size, -size,  size),
                                 glm::vec3( size,  size,  size), glm::vec3(-size,  size,  size)},
        // Back
        std::array<glm::vec3, 4>{glm::vec3( size, -size, -size), glm::vec3(-size, -size, -size),
                                 glm::vec3(-size,  size, -size), glm::vec3( size,  size, -size)},
        // Left
        std::array<glm::vec3, 4>{glm::vec3(-size, -size, -size), glm::vec3(-size, -size,  size),
                                 glm::vec3(-size,  size,  size), glm::vec3(-size,  size, -size)},
        // Right
        std::array<glm::vec3, 4>{glm::vec3( size, -size,  size), glm::vec3( size, -size, -size),
                                 glm::vec3( size,  size, -size), glm::vec3( size,  size,  size)},
        // Top
        std::array<glm::vec3, 4>{glm::vec3(-size,  size,  size), glm::vec3( size,  size,  size),
                                 glm::vec3( size,  size, -size), glm::vec3(-size,  size, -size)},
        // Bottom
        std::array<glm::vec3, 4>{glm::vec3(-size, -size, -size), glm::vec3( size, -size, -size),
                                 glm::vec3( size, -size,  size), glm::vec3(-size, -size,  size)}
    };

    // Add each face with its corresponding tile UVs
    for (int i = 0; i < 6; ++i) {
        auto uvs = getTileUVs(faceTiles[i], 128, 128, 1024, 1024);
        addFace(faceVertices[i][0], faceVertices[i][1],
                faceVertices[i][2], faceVertices[i][3], uvs);
    }

    return cubeVertices;
}

GLuint loadTexture(QString filename) {
    QImage image(filename);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load image
    int width, height, nrChannels;
    const unsigned char *data = image.constBits();

    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, nrChannels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

}

#endif // TEXTURELOADER_H
