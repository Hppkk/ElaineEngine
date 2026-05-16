// ============================================================================
// VTFeedback.vs - Virtual Texture Feedback Vertex Shader
// ============================================================================
// 
// This shader renders a fullscreen triangle for the feedback pass.
// It passes through the vertex positions and UV coordinates that the
// fragment shader uses to determine VT tile requests.
//
// In practice, the feedback pass re-renders the scene geometry at low
// resolution. This VS is used when scene objects have VT materials.
// ============================================================================

#include "Uniform_CommonVS.inl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragVirtualUV;
layout(location = 1) out flat uint fragSpaceID;

// Per-object uniform
layout(set = 2, binding = 0) uniform ObjectUniforms
{
    mat4 U_WorldMatrix;
    uint U_VTSpaceID;
    float U_Padding0;
    float U_Padding1;
    float U_Padding2;
};

void main()
{
    mat4 viewMat = mat4(U_ViewMatrix[0], U_ViewMatrix[1], U_ViewMatrix[2], U_ViewMatrix[3]);
    mat4 projMat = mat4(U_ProjectionMatrix[0], U_ProjectionMatrix[1], U_ProjectionMatrix[2], U_ProjectionMatrix[3]);

    vec4 worldPos = U_WorldMatrix * vec4(inPosition, 1.0);
    gl_Position = projMat * viewMat * worldPos;

    fragVirtualUV = inTexCoord;
    fragSpaceID = U_VTSpaceID;
}
