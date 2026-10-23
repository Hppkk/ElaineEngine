#include "SkyResources.inl"

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 sampleDirection;

void main()
{
    vec4 worldPosition = vec4(inPosition, 0.0);
    vec4 viewPosition = vec4(dot(U_ViewMatrix[0], worldPosition),
        dot(U_ViewMatrix[1], worldPosition), dot(U_ViewMatrix[2], worldPosition), 1.0);
    gl_Position = vec4(dot(U_ProjectionMatrix[0], viewPosition),
        dot(U_ProjectionMatrix[1], viewPosition), dot(U_ProjectionMatrix[2], viewPosition),
        dot(U_ProjectionMatrix[3], viewPosition));
    gl_Position.z = gl_Position.w;
    sampleDirection = inPosition;
}
