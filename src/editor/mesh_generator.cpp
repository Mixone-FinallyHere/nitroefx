#include "mesh_generator.h"

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>


GeneratedMesh MeshGenerator::generateBox(const glm::vec3& extent) {
    GeneratedMesh mesh;
    mesh.vertices = {
        {-extent.x, -extent.y, -extent.z},
        { extent.x, -extent.y, -extent.z},
        { extent.x,  extent.y, -extent.z},
        {-extent.x,  extent.y, -extent.z},
        {-extent.x, -extent.y,  extent.z},
        { extent.x, -extent.y,  extent.z},
        { extent.x,  extent.y,  extent.z},
        {-extent.x,  extent.y,  extent.z}
    };

    mesh.indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 1, 5, 5, 4, 0,
        2, 3, 7, 7, 6, 2,
        0, 3, 7, 7, 4, 0,
        1, 2, 6, 6, 5, 1
    };

    return mesh;
}

GeneratedMesh MeshGenerator::generateSphere(float radius, int segments, int rings) {
    GeneratedMesh mesh;
    mesh.vertices.reserve((segments + 1) * (rings + 1));

    // Vertices
    for (int i = 0; i <= rings; ++i) {
        float theta = glm::pi<float>() * i / rings;
        float sinTheta = glm::sin(theta);
        float cosTheta = glm::cos(theta);

        for (int j = 0; j <= segments; ++j) {
            float phi = 2.0f * glm::pi<float>() * j / segments;
            float sinPhi = glm::sin(phi);
            float cosPhi = glm::cos(phi);

            mesh.vertices.push_back({
                radius * sinTheta * cosPhi,
                radius * cosTheta,
                radius * sinTheta * sinPhi
            });
        }
    }

    // Triangles 
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = (i * (segments + 1)) + j;
            int second = first + segments + 1;

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }

    return mesh;
}

GeneratedMesh MeshGenerator::generateCylinder(float radius, float height, int segments) {
    GeneratedMesh mesh;
    float halfHeight = height * 0.5f;

    // Vertices
    // Bottom circle
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = radius * glm::cos(angle);
        float z = radius * glm::sin(angle);
        mesh.vertices.push_back({x, halfHeight, z});
    }

    // Top circle
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = radius * glm::cos(angle);
        float z = radius * glm::sin(angle);
        mesh.vertices.push_back({x, -halfHeight, z});
    }

    // Center points for caps
    int bottomCenterIndex = static_cast<int>(mesh.vertices.size());
    mesh.vertices.push_back({0.0f, halfHeight, 0.0f});
    int topCenterIndex = static_cast<int>(mesh.vertices.size());
    mesh.vertices.push_back({0.0f, -halfHeight, 0.0f});

    // Side faces
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        int bottom0 = i;
        int bottom1 = next;
        int top0 = i + segments;
        int top1 = next + segments;

        // First triangle
        mesh.indices.push_back(bottom0);
        mesh.indices.push_back(top0);
        mesh.indices.push_back(top1);

        // Second triangle
        mesh.indices.push_back(bottom0);
        mesh.indices.push_back(top1);
        mesh.indices.push_back(bottom1);
    }

    // Bottom cap
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        mesh.indices.push_back(bottomCenterIndex);
        mesh.indices.push_back(i);
        mesh.indices.push_back(next);
    }

    // Top cap
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        mesh.indices.push_back(topCenterIndex);
        mesh.indices.push_back(next + segments);
        mesh.indices.push_back(i + segments);
    }

    return mesh;
}

GeneratedMesh MeshGenerator::generateHemisphere(float radius, int segments, int rings) {
    GeneratedMesh mesh;
    mesh.vertices.reserve((segments + 1) * (rings + 1) + 1);

    // Only go from 0 to pi/2 (half sphere)
    for (int i = 0; i <= rings; ++i) {
        float theta = (glm::pi<float>() * 0.5f) * i / rings;
        float sinTheta = glm::sin(theta);
        float cosTheta = glm::cos(theta);

        for (int j = 0; j <= segments; ++j) {
            float phi = 2.0f * glm::pi<float>() * j / segments;
            float sinPhi = glm::sin(phi);
            float cosPhi = glm::cos(phi);

            mesh.vertices.push_back({
                radius * sinTheta * cosPhi,
                -radius * cosTheta,
                radius * sinTheta * sinPhi
            });
        }
    }

    // Triangles
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = (i * (segments + 1)) + j;
            int second = first + segments + 1;

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);

            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }

    // Cap the bottom (flat face)
    int baseCenterIndex = static_cast<int>(mesh.vertices.size());
    mesh.vertices.push_back({ 0.0f, 0.0f, 0.0f });
    int baseStart = rings * (segments + 1);
    for (int j = 0; j < segments; ++j) {
        int next = j + 1;
        mesh.indices.push_back(baseCenterIndex);
        mesh.indices.push_back(baseStart + next);
        mesh.indices.push_back(baseStart + j);
    }

    return mesh;
}

GeneratedMesh MeshGenerator::generateCircle(float radius, int segments) {
    GeneratedMesh mesh;
    mesh.vertices.reserve(segments + 1);

    // Center vertex
    mesh.vertices.push_back({ 0.0f, 0.0f, 0.0f });

    // Perimeter vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = radius * glm::cos(angle);
        float z = radius * glm::sin(angle);
        mesh.vertices.push_back({ x, 0.0f, z });
    }

    // Indices (triangle fan)
    for (int i = 1; i <= segments; ++i) {
        int next = (i % segments) + 1;
        mesh.indices.push_back(0);      // center
        mesh.indices.push_back(i);
        mesh.indices.push_back(next);
    }

    return mesh;
}
