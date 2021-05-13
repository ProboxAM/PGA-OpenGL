///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTextCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTextCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
    oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

#ifdef TEXTURED_DEPTH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTextCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTextCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
    float d = texture(uTexture, vTexCoord).r;
    oColor = vec4(1-pow(d,100), 1-pow(d,100), 1-pow(d,100), 1.0f);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

struct Light
{
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec3 vPosition; //In worldspace
out vec3 vNormal; //In worldspace
out vec2 vTexCoord;
out vec3 vViewDir;

void main()
{
    vTexCoord = aTextCoord;
    vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
    vViewDir = uCameraPosition - vPosition;
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 vPosition; //In worldspace
in vec3 vNormal; //In worldspace
in vec2 vTexCoord;
in vec3 vViewDir; // In worldspace

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 diffuseColor;
layout(location = 2) out vec4 normalColor;

void main()
{
    vec3 finalColor;
    vec3 textureColor = vec3(texture(uTexture, vTexCoord));

    //TODO: Sum all lights
    for(unsigned int i = 0u; i < uLightCount; ++i)
    {
        switch(uLight[i].type)
        {            
            case 0u: 
            {
                //Directional
                float cosAngle = max(dot(vNormal, -uLight[i].direction), 0.0); 
                vec3 ambient = 0.1 * uLight[i].color;
                vec3 diffuse = 0.6 * uLight[i].color * cosAngle;

                finalColor += (ambient + diffuse) * textureColor;
            } break;
            case 1u:
            {
                //Point
                vec3 pointDirection = normalize(uLight[i].position - vPosition);
                float cosAngle = max(dot(vNormal, pointDirection), 0.0); 

                float distance = length(uLight[i].position - vPosition);
                float attenuation = 1.0 / (0.25 + 0.5 * distance + 
                                     0.25 * (distance * distance));

                vec3 ambient = (0.2 * uLight[i].color) * attenuation;
                vec3 diffuse = (0.6 * uLight[i].color * cosAngle) * attenuation;

                finalColor += (ambient + diffuse) * textureColor;
            } break;
        }
    }

    oColor = vec4(finalColor, 1.0);
    diffuseColor = vec4(textureColor, 1.0);
    normalColor = vec4(vNormal * 0.5f + 0.5f, 1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef G_BUFFER_SHADER

struct Light
{
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextCoord;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec3 vPosition; //In worldspace
out vec3 vNormal; //In worldspace
out vec2 vTexCoord;

void main()
{
    vTexCoord = aTextCoord;
    vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 vPosition; //In worldspace
in vec3 vNormal; //In worldspace
in vec2 vTexCoord;

uniform sampler2D uTexture;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gNormal;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vec4(vPosition , 1.0);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec4(normalize(vNormal), 1.0);
    // and the diffuse per-fragment color
    gAlbedo = texture(uTexture, vTexCoord);
} 

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_LIGHTING_PASS

struct Light
{
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextCoord;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTextCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

out vec4 oColor;
  
in vec2 vTexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;

void main()
{             
    // retrieve data from G-buffer
    vec3 Position = texture(gPosition, vTexCoord).rgb;
    vec3 Normal = texture(gNormal, vTexCoord).rgb;
    vec3 Diffuse = texture(gDiffuse, vTexCoord).rgb;
    
    // then calculate lighting as usual
    vec3 finalColor;

    //TODO: Sum all lights
    for(unsigned int i = 0u; i < uLightCount; ++i)
    {
        switch(uLight[i].type)
        {            
            case 0u: 
            {
                //Directional
                float cosAngle = max(dot(Normal, -uLight[i].direction), 0.0); 
                vec3 ambient = 0.1 * uLight[i].color;
                vec3 diffuse = 0.8 * uLight[i].color * cosAngle;

                finalColor += (ambient + diffuse) * Diffuse;
            } break;
            case 1u:
            {
                //Point
                vec3 pointDirection = normalize(uLight[i].position - Position);
                float cosAngle = max(dot(Normal, pointDirection), 0.0); 

                float distance = length(uLight[i].position - Position);
                float attenuation = 1.0 / (0.25 + 0.5 * distance + 
                                     0.25 * (distance * distance));

                vec3 ambient = (0.2 * uLight[i].color) * attenuation;
                vec3 diffuse = (0.6 * uLight[i].color * cosAngle) * attenuation;

                finalColor += (ambient + diffuse) * Diffuse;
            } break;
        }
    }

    oColor = vec4(finalColor, 1.0);

} 

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.