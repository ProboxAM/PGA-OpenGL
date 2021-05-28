//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "Camera.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Buffer
{
    GLuint handle;
    GLenum type;
    u32 size; 
    u32 head;
    void* data; //mapped data
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalTextureIdx = 69;
    u32         bumpTextureIdx;
};

struct VertexV3V2 {
    glm::vec3 pos;
    glm::vec2 ucv;
};

struct VertexBufferAttribute {
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout {
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Model
{
    u32              meshIdx;
    std::vector<u32> materialIdx;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32>   indices;
    u32                vertexOffset;
    u32                indexOffset;

    std::vector<Vao>   vaos;
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    int type;
    vec3 color; 
    vec3 direction;
    vec3 position;

    u32       localParamsOffset;
    u32       localParamsSize;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    VertexShaderLayout vertexInputLayout;
    u64                lastWriteTimestamp; // What is this for?
};

struct Entity
{
    glm::mat4 worldMatrix;
    u32       modelIndex;
    u32       programIdx;
    u32       localParamsOffset;
    u32       localParamsSize;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Primitive,
    Mode_AssimpModel,
    Mode_Count
};

enum RenderTarget
{
    RT_Position,
    RT_Diffuse,
    RT_Normals,
    RT_Depth,
    RT_Final
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    // Camera
    Camera camera = Camera({0.0f, 8.0f, -45.0f});

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material> materials;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;
    std::vector<Program>  programs;
    std::vector<Entity>   entities;
    std::vector<Light>    lights;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedQuadProgramIdx;
    u32 depthProgramIdx;
    u32 gProgramIdx;
    u32 deferredDirectionalProgramIdx;
    u32 deferredPointProgramIdx;
    u32 pointLightDrawProgramIdx;
    u32 gProgramNormalMappingIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;
    u32 brickBaseTexIdx;
    u32 brickNormalTexIdx;
    u32 brickBumpTexIdx;

    // model index
    u32 patrickIdx;
    u32 cubeIdx;

    // model primitives index
    u32 sphereIdx;
    u32 quadIdx;

    // Mode
    Mode mode;
    RenderTarget renderTarget = RenderTarget::RT_Final;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
    GLuint quadProgramUniformTexture;
    GLuint depthProgramUniformTexture;
    GLuint gProgramUniformTexture;

    // Uniform buffer
    Buffer cbuffer, lightsBuffer;
    GLint maxUniformBufferSize, uniformBufferAlignment;

    GLuint globalParamsOffset; //offset for global params in uniform buffer
    GLuint globalParamsSize; //size of global params in uniform buffer

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    //Framebuffer object stuff
    GLuint positionAttachmentHandle;
    GLuint diffuseAttachmentHandle;
    GLuint normalsAttachmentHandle;
    GLuint depthAttachmentHandle;

    GLuint framebufferHandle;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParan);

void CreateFrameBufferObjects(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

glm::mat4 TransformScale(const vec3& scaleFactors);
glm::mat4 TransformPositionScale(const vec3 &pos, const vec3& scaleFactors);


