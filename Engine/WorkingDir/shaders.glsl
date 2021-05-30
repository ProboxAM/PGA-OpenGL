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
    mat4 uWorldViewMatrix;
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
    mat4 uWorldViewMatrix;
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
            vec3 diffuse = 0.9 * uLight[i].color * cosAngle;

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

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldViewProjectionMatrix;
};

void main()
{
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform vec2 gScreenSize;
uniform uint gLightIndex;

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
uniform sampler2D gDepth;

void main()
{             
    vec2 vTexCoord = gl_FragCoord.xy / gScreenSize;

    // retrieve data from G-buffer
    vec3 Position = texture(gPosition, vTexCoord).rgb;
    vec3 Normal = normalize(texture(gNormal, vTexCoord).rgb);
    vec3 Diffuse = texture(gDiffuse, vTexCoord).rgb;
    
    // then calculate lighting as usual
    vec3 finalColor = vec3(0);
         
    //Point          
    // diffuse
    vec3 lightDir = normalize(uLight[gLightIndex].position - Position);
    vec3 diffuse = 0.9 * max(dot(Normal, lightDir), 0.0) * Diffuse * uLight[gLightIndex].color;
    vec3 ambient = 0.1 * Diffuse;
          
    // attenuation
    float distance = length(uLight[gLightIndex].position - Position);
    float attenuation = 1.0 / (1.0 + 0.14 * distance + 0.07 * (distance * distance));
    diffuse *= attenuation;

    finalColor += diffuse;

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

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldViewProjectionMatrix;
};

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

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef RELIEF_MAPPING

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewMatrix;
    mat4 uWorldViewProjectionMatrix;
};

uniform vec3 uCameraPos;

out vec3 vPosition;
out vec2 vTexCoord;
out mat3 TBN;
out vec3 vTangentFragPos;
out vec3 vTangentViewPos;

void main()
{
    vTexCoord = aTextCoord;
    vPosition = (uWorldMatrix * vec4(aPosition, 1.0)).xyz;

    // Normal matrix
    mat3 normalMatrix = transpose(inverse(mat3(uWorldMatrix)));

    // Tangent to world (TBN) matrix
    vec3 T = normalize(vec3(normalMatrix * aTangent));
    vec3 N = normalize(vec3(normalMatrix * aNormal));
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    mat3 TTBN = transpose(TBN);

    vTangentViewPos = TTBN * uCameraPos;
    vTangentFragPos = TTBN * vPosition;

    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 vPosition;
in vec2 vTexCoord;
in mat3 TBN;
in vec3 vTangentFragPos;
in vec3 vTangentViewPos;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir);

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;
uniform float uHeightScale;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec3 gNormal;

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewMatrix;
    mat4 uWorldViewProjectionMatrix;
};

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vPosition;

    vec3 viewDir = normalize(vTangentViewPos - vTangentFragPos);

    vec2 BumpedTexCoord = ParallaxMapping(vTexCoord,  viewDir);
    if(BumpedTexCoord.x > 1.0 || BumpedTexCoord.y > 1.0 || BumpedTexCoord.x < 0.0 || BumpedTexCoord.y < 0.0)
        discard;

    // and the diffuse per-fragment color
    gAlbedo = texture(uTexture, BumpedTexCoord);

    // Convert normal from tangent space to world space
    vec3 tangentSpaceNormal = normalize(texture(uNormalMap, BumpedTexCoord).xyz * 2.0 - 1.0);
    vec3 worldSpaceNormal = normalize(TBN * tangentSpaceNormal);

    // also store the per-fragment normals into the gbuffer
    gNormal = worldSpaceNormal;
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 32;
    const float maxLayers = 64;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * uHeightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(uHeightMap, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(uHeightMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(uHeightMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;  
} 

#endif
#endif

#ifdef G_BUFFER_NORMAL_MAPPING

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec3 vPosition;
out vec2 vTexCoord;
out mat3 TBN;

void main()
{
    vTexCoord = aTextCoord;
    vPosition = (uWorldMatrix * vec4(aPosition, 1.0)).xyz;

    // Normal matrix
    mat3 normalMatrix = transpose(inverse(mat3(uWorldMatrix)));

    // Tangent to world (TBN) matrix
    vec3 T = normalize(vec3(normalMatrix * aTangent));
    vec3 N = normalize(vec3(normalMatrix * aNormal));
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);


    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////


in vec3 vPosition;
in vec2 vTexCoord;
in mat3 TBN;


uniform sampler2D uTexture;
uniform sampler2D uNormalMap;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec3 gNormal;

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewMatrix;
    mat4 uWorldViewProjectionMatrix;
};

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vPosition;
    // and the diffuse per-fragment color
    gAlbedo = texture(uTexture, vTexCoord);

    // Convert normal from tangent space to world space
    vec3 tangentSpaceNormal = normalize(texture(uNormalMap, vTexCoord).xyz * 2.0 - 1.0);
    vec3 worldSpaceNormal = normalize(TBN * tangentSpaceNormal);

    // also store the per-fragment normals into the gbuffer
    gNormal = worldSpaceNormal;
} 

#endif
#endif

#ifdef NULL_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldViewProjectionMatrix;
};

void main()
{
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

void main()
{             

} 

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.