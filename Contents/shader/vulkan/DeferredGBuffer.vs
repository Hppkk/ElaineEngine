//=============================================================================
// DeferredGBuffer.vs - GBuffer Pass Vertex Shader
//
// Transforms vertices to clip space and passes world-space data to fragment shader.
// Outputs: world position, world normal, UV, tangent basis for normal mapping.
//=============================================================================

#include "Uniform_CommonVS.inl"

// Per-object uniform
layout(set = 1, binding = 0) uniform ObjectUniformBuffer
{
    vec4 U_ModelMatrix[4];
    vec4 U_NormalMatrix[4]; // inverse-transpose of model matrix (3x3 + padding)
};

// Vertex inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;  // xyz = tangent direction, w = handedness

// Outputs to fragment shader
layout(location = 0) out vec3 vWorldPosition;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vWorldNormal;
layout(location = 3) out vec3 vWorldTangent;
layout(location = 4) out vec3 vWorldBitangent;

void main()
{
    // Transform position to world space
    vec4 localPos = vec4(inPosition, 1.0);
    vec4 worldPos;
    worldPos.x = dot(U_ModelMatrix[0], localPos);
    worldPos.y = dot(U_ModelMatrix[1], localPos);
    worldPos.z = dot(U_ModelMatrix[2], localPos);
    worldPos.w = dot(U_ModelMatrix[3], localPos);
    vWorldPosition = worldPos.xyz;

    // Transform position to clip space
    vec4 viewPos;
    viewPos.x = dot(U_ViewMatrix[0], worldPos);
    viewPos.y = dot(U_ViewMatrix[1], worldPos);
    viewPos.z = dot(U_ViewMatrix[2], worldPos);
    viewPos.w = dot(U_ViewMatrix[3], worldPos);

    gl_Position.x = dot(U_ProjectionMatrix[0], viewPos);
    gl_Position.y = dot(U_ProjectionMatrix[1], viewPos);
    gl_Position.z = dot(U_ProjectionMatrix[2], viewPos);
    gl_Position.w = dot(U_ProjectionMatrix[3], viewPos);

    // Transform normal to world space (using inverse-transpose)
    vec3 N;
    N.x = dot(U_NormalMatrix[0].xyz, inNormal);
    N.y = dot(U_NormalMatrix[1].xyz, inNormal);
    N.z = dot(U_NormalMatrix[2].xyz, inNormal);
    vWorldNormal = normalize(N);

    // Transform tangent to world space
    vec3 T;
    T.x = dot(U_ModelMatrix[0].xyz, inTangent.xyz);
    T.y = dot(U_ModelMatrix[1].xyz, inTangent.xyz);
    T.z = dot(U_ModelMatrix[2].xyz, inTangent.xyz);
    vWorldTangent = normalize(T);

    // Compute bitangent with handedness
    vWorldBitangent = cross(vWorldNormal, vWorldTangent) * inTangent.w;

    // Pass UV
    vUV = inUV;
}
