//=============================================================================
// RVTCapture.vs - Runtime Virtual Texture Capture Vertex Shader
//
// Renders scene geometry using an orthographic projection for RVT tile capture.
// The view/projection matrices come from the tile UBO (set=0, binding=0),
// which is overridden by RVTTileRenderer with the tile's ortho matrices.
//=============================================================================

// Tile UBO (overrides common uniform buffer during RVT capture)
layout(set = 0, binding = 0) uniform TileRenderUBO
{
    mat4  U_ViewMatrix;
    mat4  U_ProjectionMatrix;
    vec2  U_WorldBoundsMin;
    vec2  U_WorldBoundsMax;
};

// Per-object model matrix
layout(set = 1, binding = 0) uniform ObjectUBO
{
    mat4 U_ModelMatrix;
};

// Vertex inputs
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBitangent;
layout(location = 4) in vec2 aTexCoord;

// Outputs to fragment shader
layout(location = 0) out vec3 vWorldPosition;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vWorldNormal;
layout(location = 3) out vec3 vWorldTangent;
layout(location = 4) out vec3 vWorldBitangent;

void main()
{
    // Transform to world space
    vec4 worldPos = U_ModelMatrix * vec4(aPosition, 1.0);
    vWorldPosition = worldPos.xyz;

    // Transform normal/tangent/bitangent to world space
    mat3 normalMatrix = transpose(inverse(mat3(U_ModelMatrix)));
    vWorldNormal = normalize(normalMatrix * aNormal);
    vWorldTangent = normalize(normalMatrix * aTangent);
    vWorldBitangent = normalize(normalMatrix * aBitangent);

    // Pass through UVs
    vUV = aTexCoord;

    // Transform to clip space using tile's orthographic projection
    gl_Position = U_ProjectionMatrix * U_ViewMatrix * worldPos;
}
