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
    oColor = vec4(1-pow(d,100), 1-pow(d,100), 1-pow(d,100), 1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

struct Light
{
    uint type;
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
    uint uLightCount;
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

    mat3 normalMatrix = transpose(inverse(mat3(uWorldMatrix)));
    vNormal = normalMatrix * aNormal;
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
    uint uLightCount;
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
    for(uint i = 0; i < uLightCount; ++i)
    {
        switch(uLight[i].type)
        {            
            case 0: 
            {
                //Directional
                float cosAngle = max(dot(vNormal, -uLight[i].direction), 0.0); 
                vec3 ambient = 0.1 * uLight[i].color;
                vec3 diffuse = 0.6 * uLight[i].color * cosAngle;

                finalColor += (ambient + diffuse) * textureColor;
            } break;
            case 1:
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
    normalColor = vec4(vNormal * 0.5 + 0.5, 1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef G_BUFFER_SHADER

struct Light
{
    uint type;
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
    uint uLightCount;
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
    vPosition = (uWorldMatrix * vec4(aPosition, 1.0)).xyz;

    mat3 normalMatrix = transpose(inverse(mat3(uWorldMatrix)));
    vNormal = normalMatrix * aNormal;

    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 vPosition; //In worldspace
in vec3 vNormal; //In worldspace
in vec2 vTexCoord;

uniform sampler2D uTexture;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec3 gNormal;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    uint uLightCount;
    Light uLight[16];
};

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vPosition;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(vNormal);
    // and the diffuse per-fragment color
    gAlbedo = texture(uTexture, vTexCoord);
} 

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_DIRECTIONAL_LIGHTING_PASS

struct Light
{
    uint type;
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
    uint uLightCount;
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
    uint uLightCount;
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
    for(uint i = 0; i < uLightCount; ++i)
    {
        if(uLight[i].type == 0)
        {            
            //Directional
            float cosAngle = max(dot(Normal, -uLight[i].direction), 0.0); 
            vec3 ambient = 0.1 * uLight[i].color;
            vec3 diffuse = 0.8 * uLight[i].color * cosAngle;

            finalColor += (ambient + diffuse) * Diffuse;
        }
    }

    oColor = vec4(finalColor, 1.0);
} 

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_POINT_LIGHTING_PASS

struct Light
{
    uint type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
uniform mat4 uWorldViewProjectionMatrix;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    uint uLightCount;
    Light uLight[16];
};

void main()
{
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec2 gScreenSize;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    uint uLightCount;
    Light uLight[16];
};

out vec4 oColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;

void main()
{             
    vec2 vTexCoord = gl_FragCoord.xy / gScreenSize;

    // retrieve data from G-buffer
    vec3 Position = texture(gPosition, vTexCoord).rgb;
    vec3 Normal = normalize(texture(gNormal, vTexCoord).rgb);
    vec3 Diffuse = texture(gDiffuse, vTexCoord).rgb;
    
    // then calculate lighting as usual
    vec3 finalColor;

    //TODO: Sum all lights
    for(uint i = 0; i < uLightCount; ++i)
    {
        if(uLight[i].type == 1)
        {            
            //Point          
            // diffuse
            vec3 lightDir = normalize(uLight[i].position - Position);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * uLight[i].color;
          
            // attenuation
            float distance = length(uLight[i].position - Position);
            float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * (distance * distance));
            diffuse *= attenuation;

            finalColor += diffuse;
        }
    }

    oColor = vec4(finalColor, 1.0);
} 

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef POINT_LIGHT_DEBUG

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;

uniform mat4 uWorldViewProjectionMatrix;

void main()
{
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec3 lightColor;

out vec4 oColor;

void main()
{             
    oColor = vec4(lightColor, 1.0);
} 

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.