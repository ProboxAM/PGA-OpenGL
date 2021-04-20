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

#define BINDING(b) b

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

u32 Align(u32 value, u32 alignement) {
    return (value + alignement - 1) & ~(alignement - 1);
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

    //Load Models/Primitives
    app->quadIdx = LoadQuad(app);
    app->sphereIdx = LoadSphere(app);
    app->patrickIdx = LoadModel(app, "Patrick/Patrick.obj");

    //Camera
    app->camera.position = { -3, 5, -15 };


    //Create entities
    app->entities.push_back(Entity{ TransformPositionScale({5, 0, 0}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    app->entities.push_back(Entity{ TransformPositionScale({0, 0, 3}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    app->entities.push_back(Entity{ TransformPositionScale({-5, 0, 0}, {1.0, 1.0, 1.0}), app->patrickIdx }); //Patrick
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 180, { 0, 1, 0 });
    app->entities.push_back(Entity{ TransformPositionScale({0, -5, 0}, {100.0, 1.0, 100.0}), app->quadIdx}); //Floor
    app->entities.back().worldMatrix = TransformRotation(app->entities.back().worldMatrix, 88, { 1, 0, 0 });
    app->entities.push_back(Entity{ TransformPositionScale({-5, 5, 0}, {1.0, 1.0, 1.0}), app->sphereIdx}); //Sphere


    std::string shaderMode = "SHOW_TEXTURED_MESH";

    //Program
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", shaderMode.c_str());
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
   
    GLint attributeCount = 0;
    glGetProgramiv(texturedGeometryProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount); //Obtain attribute count of the program
    for (GLuint i = 0; i < attributeCount; ++i) //Loop through each attribute
    {
        GLchar attributeName[16];
        GLenum attributeType;
        GLsizei attributeNameLength;
        GLint attributeSize;

        //Obtain attribute properties
        glGetActiveAttrib(texturedGeometryProgram.handle, i, ARRAY_COUNT(attributeName), &attributeNameLength, &attributeSize, &attributeType, attributeName);
        
        //Obtain attribute location
        u8 attributeLocation = glGetAttribLocation(texturedGeometryProgram.handle, attributeName);

        //Fill attribute
        texturedGeometryProgram.vertexInputLayout.attributes.push_back({ attributeLocation, GetSizeFromType(attributeType) });
    }

    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    glGenBuffers(1, &app->uniformBufferHandle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBufferHandle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    app->mode = Mode_TexturedQuad;
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::End();
}

void Update(App* app)
{
    glm::mat4 projection, view;
    // You can handle app->input keyboard/mouse here
    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    vec3 upVector = { 0, 1, 0 };
    projection = glm::perspective(glm::radians(app->camera.vfov), aspectRatio, app->camera.nearPlane, app->camera.farPlane);
    view = glm::lookAt(app->camera.position, app->camera.target, upVector);

    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBufferHandle);
    u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    u32 bufferHead = 0;

    for(Entity &e : app->entities)
    {
        bufferHead = Align(bufferHead, app->uniformBlockAlignment);
        e.localParamsOffset = bufferHead;

        memcpy(bufferData + bufferHead, glm::value_ptr(e.worldMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        glm::mat4 worldViewProjectionMatrix = projection * view * e.worldMatrix;

        memcpy(bufferData + bufferHead, glm::value_ptr(worldViewProjectionMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        e.localParamsSize = bufferHead - e.localParamsOffset;
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
                glUseProgram(texturedMeshProgram.handle);

                for(const Entity &entity : app->entities)
                {
                    Model& model = app->models[entity.modelIndex];
                    Mesh& mesh = app->meshes[model.meshIdx];

                    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBufferHandle, entity.localParamsOffset, entity.localParamsSize);

                    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                    {
                        GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                        glBindVertexArray(vao);

                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                        u32 submeshMaterialIdx = model.materialIdx[i];
                        Material& submeshMaterial = app->materials[submeshMaterialIdx];
                        glUniform1i(app->programUniformTexture, 0);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

                        Submesh& submesh = mesh.submeshes[i];
                        glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    }

                    glBindVertexArray(0);
                }

                
                glUseProgram(0);
            }
            break;

        default:;
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

