//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include "Primitives.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "assimp_model_loading.h"
#include "buffer_management.h"

#define BINDING(b) b
#define NO_TEXTURE_ATTACHED 69

void ForwardRender(App* app);
void DeferredRender(App* app);
void PositionRender(App* app);
void DiffuseRender(App* app);
void NormalRender(App* app);
void DepthRender(App* app);
void FinalRender(App* app);
void GeometryPass(App* app);
void LightPass(App* app);
void StencilPass(App* app, unsigned int lightIndex);
void PointLightPass(App* app, unsigned int lightIndex);
void DirectionalLightPass(App* app);
void PointLightDraw(App* app);
float CalcPointLightRadius(const Light& Light);
u32 GenerateCustomMaterial(App* app, u32 base, u32 normal, u32 bump);

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u8 GetSizeFromType(GLenum type) {
    switch (type)
    {
    case GL_FLOAT: return 1;  break;
    case GL_FLOAT_VEC2: return 2;  break;
    case GL_FLOAT_VEC3: return 3;  break;
    case GL_FLOAT_VEC4: return 4;  break;
    default:
        break;
    }
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

u32 InitProgram(App* app, const char* filepath, const char* programName)
{
    u32 programIdx = LoadProgram(app, "shaders.glsl", programName);
    Program& program = app->programs[programIdx];

    GLint attributeCount = 0;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount); //Obtain attribute count of the program
    for (GLuint i = 0; i < attributeCount; ++i) //Loop through each attribute
    {
        GLchar attributeName[16];
        GLenum attributeType;
        GLsizei attributeNameLength;
        GLint attributeSize;

        //Obtain attribute properties
        glGetActiveAttrib(program.handle, i, ARRAY_COUNT(attributeName), &attributeNameLength, &attributeSize, &attributeType, attributeName);

        //Obtain attribute location
        u8 attributeLocation = glGetAttribLocation(program.handle, attributeName);

        //Fill attribute
        program.vertexInputLayout.attributes.push_back({ attributeLocation, GetSizeFromType(attributeType) });
    }

    return programIdx;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

glm::mat4 TransformScale(const vec3& scaleFactors)
{
    glm::mat4 transform = scale(scaleFactors);
    return scale(scaleFactors);
}

glm::mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
    glm::mat4 transform = translate(pos);
    transform = scale(transform, scaleFactors);
    return transform;
}

glm::mat4 TransformRotation(const glm::mat4& matrix, float angle, vec3 axis)
{
    float radians = glm::radians(angle);
    glm::mat4 transform = glm::rotate(matrix, radians, axis);
    return transform;
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    //Try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    GLuint vaoHandle = 0;

    //Create a new vao for this submesh/program
    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        // We have to link all vertex inputs attributes to attributes in the vertex buffer
        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
        {
            bool attributeWasLinked = false;

            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
            {
                if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
                {
                    const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                    const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                    const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
                    const u32 stride = submesh.vertexBufferLayout.stride;
                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }
            assert(attributeWasLinked); // The submesh should provide an attribute for each vertex inputs
        }
        glBindVertexArray(0);
    }

    //Store it in the list of vaos for this submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void Init(App* app)
{
    if (GLVersion.major > 4 || GLVersion.major == 4 && GLVersion.minor >= 3) {
        glDebugMessageCallback(OnGlError, app);
    }

    glEnable(GL_DEPTH_TEST);
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    //Load Textures
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
    app->brickBaseTexIdx = LoadTexture2D(app, "Bricks_Base.jpg");
    app->brickNormalTexIdx = LoadTexture2D(app, "Bricks_Normal.jpg");
    app->brickBumpTexIdx = LoadTexture2D(app, "Bricks_Bump.jpg");
    app->woodBaseTexIdx = LoadTexture2D(app, "Wood_Base.png");
    app->woodNormalTexIdx = LoadTexture2D(app, "Wood_Normal.png");
    app->woodHeightTexIdx = LoadTexture2D(app, "Wood_Height.png");

    //Load Models/Primitives
    app->quadIdx = LoadCube(app);
    app->sphereIdx = LoadSphere(app);
    app->patrickIdx = LoadModel(app, "Patrick/Patrick.obj");
    app->cubeIdx = LoadCube(app);
    app->cubeBumpIdx = LoadCube(app);
    app->models[app->cubeIdx].materialIdx[0] = GenerateCustomMaterial(app, app->woodBaseTexIdx, app->woodNormalTexIdx, NO_TEXTURE_ATTACHED);
    app->models[app->cubeBumpIdx].materialIdx[0] = GenerateCustomMaterial(app, app->woodBaseTexIdx, app->woodNormalTexIdx, app->woodHeightTexIdx);

    //Create lights
    //app->lights.push_back(Light{ LightType_Directional, {0.15, 0.15, 0.15}, {-1.0, -1.0, 0.0}, {0.0, 0.0, 0.0} }); //side directional
    const unsigned int NR_LIGHTS = 14;
    srand(app->deltaTime);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = ((rand() % 100) / 100.0) * 50.0 - 25.0;
        float yPos = ((rand() % 100) / 100.0) * 3.0;
        float zPos = ((rand() % 100) / 100.0) * 50.0 - 25.0;
        vec3 lightPosition = glm::vec3(xPos, yPos, zPos);
        // also calculate random color
        float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        vec3 lightColor = glm::vec3(rColor, gColor, bColor);

        app->lights.push_back(Light{ LightType_Point, lightColor, {0.0, 0.0, 0.0}, lightPosition });
    }

    //Program
    app->texturedGeometryProgramIdx = InitProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    app->texturedQuadProgramIdx = InitProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    app->depthProgramIdx = InitProgram(app, "shaders.glsl", "TEXTURED_DEPTH");
    app->gProgramIdx = InitProgram(app, "shaders.glsl", "G_BUFFER_SHADER");
    app->deferredDirectionalProgramIdx = InitProgram(app, "shaders.glsl", "DEFERRED_DIRECTIONAL_LIGHTING_PASS");
    app->deferredPointProgramIdx = InitProgram(app, "shaders.glsl", "DEFERRED_POINT_LIGHTING_PASS");
    app->pointLightDrawProgramIdx = InitProgram(app, "shaders.glsl", "POINT_LIGHT_DEBUG");
    app->reliefMappingIdx = InitProgram(app, "shaders.glsl", "RELIEF_MAPPING");
    app->gProgramNormalMappingIdx = InitProgram(app, "shaders.glsl", "G_BUFFER_NORMAL_MAPPING");
    app->nullGeometryIdx = InitProgram(app, "shaders.glsl", "NULL_GEOMETRY");

    ////////////////////////////////
    app->programUniformTexture = glGetUniformLocation(app->programs[app->texturedGeometryProgramIdx].handle, "uTexture");
    app->quadProgramUniformTexture = glGetUniformLocation(app->programs[app->texturedQuadProgramIdx].handle, "uTexture");
    app->depthProgramUniformTexture = glGetUniformLocation(app->programs[app->depthProgramIdx].handle, "uTexture");
    app->gProgramUniformTexture = glGetUniformLocation(app->programs[app->gProgramIdx].handle, "uTexture");

    glUseProgram(app->programs[app->gProgramNormalMappingIdx].handle);
    glUniform1i(glGetUniformLocation(app->programs[app->gProgramNormalMappingIdx].handle, "uTexture"), 0);
    glUniform1i(glGetUniformLocation(app->programs[app->gProgramNormalMappingIdx].handle, "uNormalMap"), 1);

    glUseProgram(app->programs[app->reliefMappingIdx].handle);
    glUniform1i(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "uTexture"), 0);
    glUniform1i(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "uNormalMap"), 1);
    glUniform1i(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "uHeightMap"), 2);

    glUseProgram(app->programs[app->deferredDirectionalProgramIdx].handle);
    glUniform1i(glGetUniformLocation(app->programs[app->deferredDirectionalProgramIdx].handle, "gPosition"), 0);
    glUniform1i(glGetUniformLocation(app->programs[app->deferredDirectionalProgramIdx].handle, "gNormal"), 1);
    glUniform1i(glGetUniformLocation(app->programs[app->deferredDirectionalProgramIdx].handle, "gDiffuse"), 2);

    glUseProgram(app->programs[app->deferredPointProgramIdx].handle);
    glUniform1i(glGetUniformLocation(app->programs[app->deferredPointProgramIdx].handle, "gPosition"), 0);
    glUniform1i(glGetUniformLocation(app->programs[app->deferredPointProgramIdx].handle, "gNormal"), 1);
    glUniform1i(glGetUniformLocation(app->programs[app->deferredPointProgramIdx].handle, "gDiffuse"), 2);

    glUseProgram(0);

    //Create render targets
    CreateFrameBufferObjects(app);

    //Uniform buffer
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBufferAlignment);

    app->cbuffer = CreateConstantBuffer(app->maxUniformBufferSize);
    app->lightsBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    //Create entities
    //app->entities.push_back(Entity{ TransformPositionScale({10, 3, 0}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({0, 3, 0}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({-10, 3, 0}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({10, 3, -10}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({0, 3, -10}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({-10, 3, -10}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({10, 3, 10}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({0, 3, 10}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    //app->entities.push_back(Entity{ TransformPositionScale({-10, 3, 10}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    //app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    app->entities.push_back(Entity{ TransformPositionScale({-1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}), app->cubeIdx, app->gProgramNormalMappingIdx }); //Cube
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });

    app->entities.push_back(Entity{ TransformPositionScale({-3.0, 1.0, 0.0}, {1.0, 1.0, 1.0}), app->cubeBumpIdx, app->reliefMappingIdx }); //Cube
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });

    app->entities.push_back(Entity{ TransformPositionScale({1, 1.0, 0.0}, {1.0, 1.0, 1.0}), app->cubeIdx, app->gProgramIdx }); //Cube
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });

    app->entities.push_back(Entity{ TransformPositionScale({-3.0, 1.0, 0}, {0.2, 0.2, 0.2}), app->sphereIdx, app->gProgramIdx }); //Floor

    app->entities.push_back(Entity{ TransformPositionScale({0, -0.5, 0}, {100.0, 1.0, 100.0}), app->quadIdx, app->gProgramIdx }); //Floor
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, -90, { 1, 0, 0 });

    app->mode = Mode::Mode_Deferred;
}

void Gui(App* app)
{
    ImGui::BeginMainMenuBar();
    {
        static const char* modeSelections[]{ "Forward", "Deferred"};
        static int selectedMode = app->mode;

        if (ImGui::Combo("Render Mode", &selectedMode, modeSelections, ARRAY_COUNT(modeSelections)))
        {
            app->mode = (Mode)selectedMode;
        }


        if (app->mode == Mode::Mode_Deferred)
        {
            static const char* selections[]{ "Position", "Diffuse", "Normals", "Depth", "Final" };
            static int selectedTarget = app->renderTarget;

            if (ImGui::Combo("RenderTarget", &selectedTarget, selections, ARRAY_COUNT(selections)))
            {
                app->renderTarget = (RenderTarget)selectedTarget;
            }
        }
    }
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::EndMainMenuBar();
}

void Update(App* app)
{
    glm::mat4 projection, view;
    // You can handle app->input keyboard/mouse here

    //////////////////////////////////////////KEYBOARD///////////////////////////////////////////
    if (app->input.keys[K_W] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessKeyboard(Camera_Movement::FORWARD, app->deltaTime);
    }
    if (app->input.keys[K_S] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessKeyboard(Camera_Movement::BACKWARD, app->deltaTime);
    }
    if (app->input.keys[K_A] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessKeyboard(Camera_Movement::LEFT, app->deltaTime);
    }
    if (app->input.keys[K_D] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessKeyboard(Camera_Movement::RIGHT, app->deltaTime);
    }
    if (app->input.keys[K_SPACE] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessKeyboard(Camera_Movement::UP, app->deltaTime);
    }
    if (app->input.keys[K_SHIFT] == ButtonState::BUTTON_PRESS) {
        app->camera.ProcessSpeed(true);
    }
    if (app->input.keys[K_SHIFT] == ButtonState::BUTTON_RELEASE) {
        app->camera.ProcessSpeed(false);
    }

    if (app->input.keys[K_I] == ButtonState::BUTTON_PRESSED) {
        app->lights[0].position += vec3(0.0, 0.1, 0.0);
    }
    if (app->input.keys[K_K] == ButtonState::BUTTON_PRESSED) {
        app->lights[0].position -= vec3(0.0, 0.1, 0.0);
    }
    if (app->input.keys[K_J] == ButtonState::BUTTON_PRESSED) {
        app->lights[0].position += vec3(0.1, 0.0, 0.0);
    }
    if (app->input.keys[K_L] == ButtonState::BUTTON_PRESSED) {
        app->lights[0].position -= vec3(0.1, 0.0, 0.0);
    }

    ////////////////////////////////////////////MOUSE/////////////////////////////////////////////
    if (app->input.mouseButtons[1] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessMouseMovement(app->input.mouseDelta.x, -app->input.mouseDelta.y);
        app->camera.Orbit = false;
    }

    if (app->input.mouseButtons[0] == ButtonState::BUTTON_PRESSED) {
        app->camera.ProcessArcBallMovement(-app->input.mouseDelta.x, -app->input.mouseDelta.y, app->displaySize.x, app->displaySize.y);
        app->camera.Orbit = true;
    }

    if (app->input.mouseButtons[0] == ButtonState::BUTTON_RELEASE) {
        app->camera.Orbit = false;
    }

    //GLOBAL AND LOCAL CBUFFER
    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    vec3 upVector = { 0, 1, 0 };
    projection = glm::perspective(glm::radians(app->camera.Zoom), aspectRatio, app->camera.NearPlane, app->camera.FarPlane);

    view = app->camera.GetViewMatrix();

    MapBuffer(app->cbuffer, GL_WRITE_ONLY);

    // -- Global params
    app->globalParamsOffset = app->cbuffer.head;
    PushVec3(app->cbuffer, app->camera.Position);
    PushUInt(app->cbuffer, app->lights.size());

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cbuffer, sizeof(vec4));

        Light& light = app->lights[i];
        PushUInt(app->cbuffer, light.type);
        PushVec3(app->cbuffer, light.color);
        PushVec3(app->cbuffer, light.direction);
        PushVec3(app->cbuffer, light.position);
    }

    app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

    // -- Local params
    for(Entity &e : app->entities)
    {
        AlignHead(app->cbuffer, app->uniformBufferAlignment);
        glm::mat4 world = e.worldMatrix;
        glm::mat4 worldView = view * world;
        glm::mat4 worldViewProjection = projection * view * world;

        e.localParamsOffset = app->cbuffer.head;
        PushMat4(app->cbuffer, world);
        PushMat4(app->cbuffer, worldView);
        PushMat4(app->cbuffer, worldViewProjection);
        e.localParamsSize = app->cbuffer.head - e.localParamsOffset;
    }

    UnmapBuffer(app->cbuffer);

    MapBuffer(app->lightsBuffer, GL_WRITE_ONLY);
    // -- Light params
    for (Light& light : app->lights)
    {
        if (light.type != LightType_Point) //Point Light
            continue;

        AlignHead(app->lightsBuffer, app->uniformBufferAlignment);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(CalcPointLightRadius(light))); //this is for sphere light volume, makes sphere size same as radius of light
        //model = glm::scale(model, glm::vec3(0.25f)); //small size for forward rendering sphere
        glm::mat4 worldViewProjection = projection * view * model;

        light.localParamsOffset = app->lightsBuffer.head;
        PushMat4(app->lightsBuffer, worldViewProjection);
        light.localParamsSize = app->lightsBuffer.head - light.localParamsOffset;
    }

    UnmapBuffer(app->lightsBuffer);
}


void Render(App* app)
{
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    switch (app->mode)
    {
        case Mode_Deferred: DeferredRender(app); break;
        case Mode_Forward: ForwardRender(app); break;
    }  
}

void ForwardRender(App* app)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    Program& texturedMeshProgram = app->programs[app->texturedGeometryProgramIdx];
    glUseProgram(texturedMeshProgram.handle);
    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

    for (const Entity& entity : app->entities)
    {
        Model& model = app->models[entity.modelIndex];
        Mesh& mesh = app->meshes[model.meshIdx];

        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
            glBindVertexArray(vao);

            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
        }

        glBindVertexArray(0);
    }

    glUseProgram(0);
}

void DeferredRender(App* app)
{
    //Geomtry Pass
    GeometryPass(app);
    //Light Pass
    LightPass(app);

    //Add depth from gBuffer to the default fbo
    //glBindFramebuffer(GL_READ_FRAMEBUFFER, app->framebufferHandle);
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    //glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);   
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
   

    //Quad Render for Selected Texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    switch (app->renderTarget)
    {
    case RenderTarget::RT_Position:
    {
        PositionRender(app);
    }break;
    case RenderTarget::RT_Diffuse:
    {
        DiffuseRender(app);
    }break;
    case RenderTarget::RT_Depth:
    {
        DepthRender(app);
    }break;
    case RenderTarget::RT_Normals:
    {
        NormalRender(app);
    }break;
    case RenderTarget::RT_Final:
    {
        FinalRender(app);
    }break;
    }
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParan) {


    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    ELOG("OpenGL debug message : % s", message);

    switch (source)
    {
        case GL_DEBUG_SOURCE_API: ELOG(" - source: GL_DEBUG_SOURCE_API"); break; // Calls to the OpenGL API
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break; // Calls to a window-system API
        case GL_DEBUG_SOURCE_SHADER_COMPILER: ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break; // A compiler for a shading language
        case GL_DEBUG_SOURCE_THIRD_PARTY: ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break; // An application associated with OpenGL
        case GL_DEBUG_SOURCE_APPLICATION: ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break; // Generated by the user of this applicat
        case GL_DEBUG_SOURCE_OTHER: ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break; // Some source that isn't one of these

    }
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break; // An error, typically from the API
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR : ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED BEHAVIOR"); break; // Some behavior marked deprecated |
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR : ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break; // Something has invoked undefined bi
        case GL_DEBUG_TYPE_PORTABILITY: ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break; // Some functionality the user relies upon
        case GL_DEBUG_TYPE_PERFORMANCE: ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break; // Code has triggered possible performance
        case GL_DEBUG_TYPE_MARKER: ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break; // Command stream annotation
        case GL_DEBUG_TYPE_PUSH_GROUP: ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break; // Group pushing
        case GL_DEBUG_TYPE_POP_GROUP: ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break; // foo
        case GL_DEBUG_TYPE_OTHER: ELOG(" - type: GL_DEBUG_TYPE_OTHER"); break; // Some type that isn‘t one of these
    }

    switch (severity) 
    {
        case GL_DEBUG_SEVERITY_HIGH: ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break; // All OpenGL Errors, shader compilation/link
        case GL_DEBUG_SEVERITY_MEDIUM: ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break; // Major performance warnings, shader compi
        case GL_DEBUG_SEVERITY_LOW: ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break; // Redundant state change performance warning,
        case GL_DEBUG_SEVERITY_NOTIFICATION: ELOG(" - severity: GL_DEBUG_SEVERITY_NOTIFICATION"); break; // Anything that isn‘t an error or pei
    }
}

void CreateFrameBufferObjects(App* app)
{
    //Framebuffer creation

    //Position render target
    glGenTextures(1, &app->positionAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    //Diffuse render target
    glGenTextures(1, &app->diffuseAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    //Normals render target
    glGenTextures(1, &app->normalsAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    //Final render target
    glGenTextures(1, &app->finalAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->finalAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    //Depth render target              
    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_STENCIL,
                    GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    //frame buffer creation and render target attachments
    glGenFramebuffers(1, &app->framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->positionAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->diffuseAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->normalsAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->finalAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, app->depthAttachmentHandle, 0);
    
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER); 
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
            case GL_FRAMEBUFFER_UNDEFINED : ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER : ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER : ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
            case GL_FRAMEBUFFER_UNSUPPORTED : ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE : ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS : ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
            default: ELOG("Unknown framebuifer status error"); break;
        }
    }
        
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void GeometryPass(App* app)
{
    //Render on this framebuffer render targets
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

    GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    // TODO: Draw your textured quad here!
    // - clear the framebuffer
    // - set the viewport
    // - set the blending state
    // - bind the texture into unit 0
    // - bind the program 
    //   (...and make its texture sample from unit 0)
    // - bind the vao
    // - glDrawElements() !!!

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    for (const Entity& entity : app->entities)
    {

        Model& model = app->models[entity.modelIndex];
        Mesh& mesh = app->meshes[model.meshIdx];

        Program& texturedMeshProgram = app->programs[entity.programIdx];
        glUseProgram(texturedMeshProgram.handle);

        //glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
            glBindVertexArray(vao);

            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

            if (submeshMaterial.normalTextureIdx != NO_TEXTURE_ATTACHED) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.normalTextureIdx].handle);
            }

            if (submeshMaterial.bumpTextureIdx != NO_TEXTURE_ATTACHED) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.bumpTextureIdx].handle);
                glUniform3f(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "uCameraPos"),
                    app->camera.Position.x, app->camera.Position.y, app->camera.Position.z);
                glUniform1f(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "uHeightScale"), 0.1f);
                glUniform1f(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "zNear"), app->camera.NearPlane);
                glUniform1f(glGetUniformLocation(app->programs[app->reliefMappingIdx].handle, "zFar"), app->camera.FarPlane);
            }

            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
        }

        glBindVertexArray(0);
    }

    glUseProgram(0);
}

void LightPass(App* app)
{
    glDepthMask(GL_FALSE);

    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glClear(GL_COLOR_BUFFER_BIT);

    //Point light with stencil
    glEnable(GL_STENCIL_TEST);

    for (unsigned int i = 0; i < app->lights.size(); i++) {
        if (app->lights[i].type == LightType_Point) //Point Light
        {
            //Stencil pass for sphere light volume
            StencilPass(app, i);
            //Point pass using sphere light volume, not working currently because of depth of volume issues
            PointLightPass(app, i);
        }
    }

    glDisable(GL_STENCIL_TEST);
   
    //Directional pass using a quad
    DirectionalLightPass(app);
}

void PositionRender(App* app)
{
    Program& program = app->programs[app->texturedQuadProgramIdx];
    glUseProgram(program.handle);

    Mesh& mesh = app->meshes[app->quadIdx];
    GLuint vao = FindVAO(mesh, 0, program);
    glBindVertexArray(vao);

    glUniform1i(app->quadProgramUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);

    Submesh& submesh = mesh.submeshes[0];
    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

    glBindVertexArray(0);
    glUseProgram(0);
}

void DiffuseRender(App* app)
{
    Program& program = app->programs[app->texturedQuadProgramIdx];
    glUseProgram(program.handle);

    Mesh& mesh = app->meshes[app->quadIdx];
    GLuint vao = FindVAO(mesh, 0, program);
    glBindVertexArray(vao);

    glUniform1i(app->quadProgramUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);

    Submesh& submesh = mesh.submeshes[0];
    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

    glBindVertexArray(0);
    glUseProgram(0);
}

void DepthRender(App* app)
{
    Program& program = app->programs[app->depthProgramIdx];
    glUseProgram(program.handle);

    Mesh& mesh = app->meshes[app->quadIdx];
    GLuint vao = FindVAO(mesh, 0, program);
    glBindVertexArray(vao);

    glUniform1i(app->depthProgramUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);

    Submesh& submesh = mesh.submeshes[0];
    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

    glBindVertexArray(0);
    glUseProgram(0);
}

void NormalRender(App* app)
{
    Program& program = app->programs[app->texturedQuadProgramIdx];
    glUseProgram(program.handle);

    Mesh& mesh = app->meshes[app->quadIdx];
    GLuint vao = FindVAO(mesh, 0, program);
    glBindVertexArray(vao);

    glUniform1i(app->quadProgramUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);

    Submesh& submesh = mesh.submeshes[0];
    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

    glBindVertexArray(0);
    glUseProgram(0);
}

void FinalRender(App* app)
{
    Program& program = app->programs[app->texturedQuadProgramIdx];
    glUseProgram(program.handle);

    Mesh& mesh = app->meshes[app->quadIdx];
    GLuint vao = FindVAO(mesh, 0, program);
    glBindVertexArray(vao);

    glUniform1i(app->quadProgramUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->finalAttachmentHandle);

    Submesh& submesh = mesh.submeshes[0];
    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

    glBindVertexArray(0);
    glUseProgram(0);
}

void StencilPass(App* app, unsigned int lightIndex)
{
    Program& program = app->programs[app->nullGeometryIdx];
    glUseProgram(program.handle);
   
    // Disable color/depth write and enable stencil
    glDrawBuffer(GL_NONE);
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_CULL_FACE);

    glClear(GL_STENCIL_BUFFER_BIT);

    // We need the stencil test to be enabled but we want it
    // to succeed always. Only the depth test matters.
    glStencilFunc(GL_ALWAYS, 0, 0);

    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

    //Render Sphere

    Mesh point_mesh = app->meshes[app->models[app->sphereIdx].meshIdx];
    Submesh point_submesh = point_mesh.submeshes[0];

    GLuint pointVao = FindVAO(point_mesh, 0, program);
    glBindVertexArray(pointVao);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->lightsBuffer.handle, app->lights[lightIndex].localParamsOffset, app->lights[lightIndex].localParamsSize);

    glDrawElements(GL_TRIANGLES, point_submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)point_submesh.indexOffset);

    glBindVertexArray(0);
}

void PointLightPass(App* app, unsigned int lightIndex)
{
    glDrawBuffer(GL_COLOR_ATTACHMENT3);

    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    //Render Point Lights into a Sphere Light Volume using the gBuffer textures
    Program& program = app->programs[app->deferredPointProgramIdx];
    glUseProgram(app->programs[app->deferredPointProgramIdx].handle);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

    Mesh point_mesh = app->meshes[app->models[app->sphereIdx].meshIdx];
    Submesh point_submesh = point_mesh.submeshes[0];

    GLuint pointVao = FindVAO(point_mesh, 0, program);
    glBindVertexArray(pointVao);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->lightsBuffer.handle, app->lights[lightIndex].localParamsOffset, app->lights[lightIndex].localParamsSize);
    glUniform2f(glGetUniformLocation(program.handle, "gScreenSize"), (float)app->displaySize.x, (float)app->displaySize.y);
    glUniform1ui(glGetUniformLocation(program.handle, "gLightIndex"), lightIndex);
      
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);

    glDrawElements(GL_TRIANGLES, point_submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)point_submesh.indexOffset);

    glBindVertexArray(0);

    glUseProgram(0);

    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
}

void DirectionalLightPass(App* app)
{
    glDrawBuffer(GL_COLOR_ATTACHMENT3);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    //Render directional light into a quad using gBuffer textures
    Program& program = app->programs[app->deferredDirectionalProgramIdx];
    glUseProgram(program.handle);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

    Mesh& mesh = app->meshes[app->quadIdx];
    GLuint vao = FindVAO(mesh, 0, program);
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);

    Submesh& submesh = mesh.submeshes[0];
    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);
}

void PointLightDraw(App* app)
{
    //Render point lights as a small sphere to show where light is positioned.
    glDisable(GL_BLEND);

    Program& program = app->programs[app->pointLightDrawProgramIdx];
    glUseProgram(program.handle);

    Mesh point_mesh = app->meshes[app->models[app->sphereIdx].meshIdx];
    for (const Light& light : app->lights)
    {
        if (light.type == 1) //Point Light
        {
            glUniform3f(glGetUniformLocation(program.handle, "lightColor"),
                light.color.r, light.color.g, light.color.b);
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->lightsBuffer.handle, light.localParamsOffset, light.localParamsSize);
           
            GLuint pointVao = FindVAO(point_mesh, 0, app->programs[app->pointLightDrawProgramIdx]);
            glBindVertexArray(pointVao);

            Submesh point_submesh = point_mesh.submeshes[0];
            glDrawElements(GL_TRIANGLES, point_submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)point_submesh.indexOffset);

            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
}

float CalcPointLightRadius(const Light& Light)
{
    float constant = 1.0;
    float linear = 0.14;
    float quadratic = 0.07;
    float lightMax = std::fmaxf(std::fmaxf(Light.color.r, Light.color.g), Light.color.b);

    float radius =
        (-linear + std::sqrtf(linear * linear - 4.0f * quadratic * (constant - (256.0f / 5.0f) * lightMax)))
        / (2.0f * quadratic);
    
    return radius;
}

u32 GenerateCustomMaterial(App* app, u32 base, u32 normal, u32 bump) {
    Material myMat;
    myMat.albedo = vec3(1.0f, 1.0f, 1.0f);
    myMat.albedoTextureIdx = base;
    myMat.normalTextureIdx = normal;
    myMat.bumpTextureIdx = bump;

    u32 materialIdx = app->materials.size();
    app->materials.push_back(myMat);

    return materialIdx;
}
