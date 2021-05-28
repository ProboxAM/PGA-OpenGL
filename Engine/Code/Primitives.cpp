#include "engine.h"
#include "Primitives.h"

u32 LoadSphere(App* app)
{

    int H = 32;
    int V = 16;

    static const float pi = 3.1416f;
    std::vector<float> vertices = {
    };

    for (int h = 0; h < H; ++h) {
        for (int v = 0; v < V + 1; ++v) {

            float nh = float(h) / H;
            float nv = float(v) / V - 0.5f;
            float angleh = 2 * pi * nh;
            float anglev = -pi * nv;
            vertices.push_back(sinf(angleh) * cosf(anglev));
            vertices.push_back(-sinf(anglev));
            vertices.push_back(cosf(angleh) * cosf(anglev));
            vertices.push_back(sinf(angleh) * cosf(anglev));
            vertices.push_back(-sinf(anglev));
            vertices.push_back(cosf(angleh) * cosf(anglev));
            vertices.push_back( 0.0 );
            vertices.push_back( 0.0 );

        }
    }
    
    std::vector<u32> indices = {
    };

    for (unsigned int h = 0; h < H; ++h) {
        for (unsigned int v = 0; v < V; ++v) {
            indices.push_back((h + 0) * (V + 1) + v);
            indices.push_back(((h + 1) % H) * (V + 1) + v);
            indices.push_back(((h + 1) % H) * (V + 1) + v + 1);
            indices.push_back((h + 0) * (V + 1) + v);
            indices.push_back(((h + 1) % H) * (V + 1) + v + 1);
            indices.push_back((h + 0) * (V + 1) + v + 1);
        }
    }

    Mesh myMesh = {};

    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 6 * sizeof(float) });
    vertexBufferLayout.stride = 8 * sizeof(float);

    Submesh submesh = {};
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(vertices);
    submesh.indices.swap(indices);

    myMesh.submeshes.push_back(submesh);


    ////Geometry
    float verticesOffset = 0.0f;
    float indicesOffset = 0.0f;

    glGenBuffers(1, &myMesh.vertexBufferHandle);
    glGenBuffers(1, &myMesh.indexBufferHandle);

    for (int i = 0; i < myMesh.submeshes.size(); ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, myMesh.vertexBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * myMesh.submeshes[i].vertices.size(), &myMesh.submeshes[i].vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myMesh.indexBufferHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * myMesh.submeshes[i].indices.size(), &myMesh.submeshes[i].indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        submesh.vertexOffset = verticesOffset;
        submesh.indexOffset = indicesOffset;

        verticesOffset += sizeof(float) * myMesh.submeshes[i].vertices.size();
        indicesOffset += sizeof(u32) * myMesh.submeshes[i].indices.size();
    }

    Model myModel = {};
    Material myMat = {};

    myModel.meshIdx = app->meshes.size();
    app->meshes.push_back(myMesh);

    u32 modelIdx = app->models.size();

    myMat.albedo = vec3(1.0f, 1.0f, 1.0f);
    myMat.albedoTextureIdx = app->whiteTexIdx;

    u32 materialIdx = app->materials.size();
    app->materials.push_back(myMat);
    myModel.materialIdx.push_back(materialIdx);

    app->models.push_back(myModel);

    return modelIdx;
}

u32 LoadQuad(App* app)
{
    std::vector<float> vertices = {
    -1.0, -1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, // bottom-left
    1.0, -1.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, // bottom-right
    1.0, 1.0, 0.0, 0.0, 0.0, -1.0, 1.0, 1.0, // top-right
    -1.0, 1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0 // top-left
    };

    std::vector<u32> indices = {
        0, 1, 2,
        0, 2, 3
    };

    Mesh myMesh = {};

    //create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 6 * sizeof(float) });
    vertexBufferLayout.stride = 8 * sizeof(float);

    //add the submesh into the mesh
    Submesh submesh = {};
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(vertices);
    submesh.indices.swap(indices);

    myMesh.submeshes.push_back(submesh);

    ////Geometry
    float verticesOffset = 0.0f;
    float indicesOffset = 0.0f;

    glGenBuffers(1, &myMesh.vertexBufferHandle);
    glGenBuffers(1, &myMesh.indexBufferHandle);

    for (int i = 0; i < myMesh.submeshes.size(); ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, myMesh.vertexBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * myMesh.submeshes[i].vertices.size(), &myMesh.submeshes[i].vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myMesh.indexBufferHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * myMesh.submeshes[i].indices.size(), &myMesh.submeshes[i].indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        submesh.vertexOffset = verticesOffset;
        submesh.indexOffset = indicesOffset;

        verticesOffset += sizeof(float) * myMesh.submeshes[i].vertices.size();
        indicesOffset += sizeof(u32) * myMesh.submeshes[i].indices.size();
    }

    Model myModel;

    myModel.meshIdx = app->meshes.size();
    app->meshes.push_back(myMesh);

    u32 modelIdx = app->models.size();

    Material myMat;
    myMat.albedo = vec3(1.0f, 1.0f, 1.0f);
    myMat.albedoTextureIdx = app->whiteTexIdx;

    u32 materialIdx = app->materials.size();
    app->materials.push_back(myMat);
    myModel.materialIdx.push_back(materialIdx);

    app->models.push_back(myModel);

    return modelIdx;

}

u32 LoadCube(App* app)
{
    // positions
    glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
    glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
    glm::vec3 pos3(1.0f, -1.0f, 0.0f);
    glm::vec3 pos4(1.0f, 1.0f, 0.0f);
    // texture coordinates
    glm::vec2 uv1(0.0f, 1.0f);
    glm::vec2 uv2(0.0f, 0.0f);
    glm::vec2 uv3(1.0f, 0.0f);
    glm::vec2 uv4(1.0f, 1.0f);
    // normal vector
    glm::vec3 nm(0.0f, 0.0f, 1.0f);

    // calculate tangent/bitangent vectors of both triangles
    glm::vec3 tangent1, bitangent1;
    glm::vec3 tangent2, bitangent2;
    // triangle 1
    // ----------
    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent1 = glm::normalize(tangent1);

    bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    bitangent1 = glm::normalize(bitangent1);

    // triangle 2
    // ----------
    edge1 = pos3 - pos1;
    edge2 = pos4 - pos1;
    deltaUV1 = uv3 - uv1;
    deltaUV2 = uv4 - uv1;

    f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent2 = glm::normalize(tangent2);


    bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    bitangent2 = glm::normalize(bitangent2);


    std::vector<float> vertices = {
        // positions            // normal         // texcoords  // tangent                          // bitangent
        pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
        pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
        pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
        pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
    };

    std::vector<u32> indices = {
    1, 2, 3,
    1, 3, 0
    };

    Mesh myMesh = {};

    //create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 6 * sizeof(float) });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, 8 * sizeof(float) });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, 11 * sizeof(float) });
    vertexBufferLayout.stride = 14 * sizeof(float);

    //add the submesh into the mesh
    Submesh submesh = {};
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(vertices);
    submesh.indices.swap(indices);

    myMesh.submeshes.push_back(submesh);

    ////Geometry
    float verticesOffset = 0.0f;
    float indicesOffset = 0.0f;

    glGenBuffers(1, &myMesh.vertexBufferHandle);
    glGenBuffers(1, &myMesh.indexBufferHandle);

    for (int i = 0; i < myMesh.submeshes.size(); ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, myMesh.vertexBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * myMesh.submeshes[i].vertices.size(), &myMesh.submeshes[i].vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myMesh.indexBufferHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * myMesh.submeshes[i].indices.size(), &myMesh.submeshes[i].indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        submesh.vertexOffset = verticesOffset;
        submesh.indexOffset = indicesOffset;

        verticesOffset += sizeof(float) * myMesh.submeshes[i].vertices.size();
        indicesOffset += sizeof(u32) * myMesh.submeshes[i].indices.size();
    }

    Model myModel;

    myModel.meshIdx = app->meshes.size();
    app->meshes.push_back(myMesh);

    u32 modelIdx = app->models.size();

    Material myMat;
    myMat.albedo = vec3(1.0f, 1.0f, 1.0f);
    myMat.albedoTextureIdx = app->whiteTexIdx;

    u32 materialIdx = app->materials.size();
    app->materials.push_back(myMat);
    myModel.materialIdx.push_back(materialIdx);

    app->models.push_back(myModel);

    return modelIdx;
}
