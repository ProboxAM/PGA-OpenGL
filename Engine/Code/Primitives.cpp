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
    return u32();
}
