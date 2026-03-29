#include "Uniform_CommonVS.inl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 vsColor;
layout(location = 1) out vec3 uv;

void main()
{
    vec4 worldPos = vec4(inPosition, 1.0);

    // 从 U_ViewMatrix 中去掉平移（行主序：将每行的 w 分量置 0）
    // 让天空盒始终以相机为中心，不受相机位移影响
    vec4 viewRow0 = vec4(U_ViewMatrix[0].xyz, 0.0);
    vec4 viewRow1 = vec4(U_ViewMatrix[1].xyz, 0.0);
    vec4 viewRow2 = vec4(U_ViewMatrix[2].xyz, 0.0);
    vec4 viewRow3 = vec4(0.0, 0.0, 0.0, 1.0);

    // 仅旋转的 View 变换
    vec4 viewPos;
    viewPos.x = dot(viewRow0, worldPos);
    viewPos.y = dot(viewRow1, worldPos);
    viewPos.z = dot(viewRow2, worldPos);
    viewPos.w = dot(viewRow3, worldPos);

    // 投影变换
    gl_Position.x = dot(U_ProjectionMatrix[0], viewPos);
    gl_Position.y = dot(U_ProjectionMatrix[1], viewPos);
    gl_Position.z = dot(U_ProjectionMatrix[2], viewPos);
    gl_Position.w = dot(U_ProjectionMatrix[3], viewPos);

    uv = inPosition;
    // 将 z 设为 w，确保天空盒始终在最远处（depth = 1.0）
    gl_Position.z = gl_Position.w;
}