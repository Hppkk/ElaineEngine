#include "Uniform_CommonVS.inl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 vsColor;
layout(location = 1) out vec3 uv;

// layout(set = 1, binding = 0) uniform PrivateUniformVS
// {
//     vec4 U_WorldMatrix[4];
// };

void main()
{
    vec2 uvs = inUv;
    vec3 normal = inNormal;
    vec4 wordPos = vec4(inPosition, 1.0);
    gl_Position.x = dot(U_ViewProjectionMatrix[0], wordPos);
    gl_Position.y = dot(U_ViewProjectionMatrix[1], wordPos);
    gl_Position.z = dot(U_ViewProjectionMatrix[2], wordPos);
    gl_Position.w = dot(U_ViewProjectionMatrix[3], wordPos);  
    uv = inPosition;
    gl_Position.z = gl_Position.w;

}