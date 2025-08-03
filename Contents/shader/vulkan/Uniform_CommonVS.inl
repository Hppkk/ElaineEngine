layout(set = 0, binding = 0) uniform CommonUniformBuffer 
{
    vec4 U_ViewMatrix[4];
    vec4 U_ProjectionMatrix[4];
    vec4 U_ViewProjectionMatrix[4];
    vec4 U_CameraPosition;
    vec4 U_CameraDirection;
    vec4 U_LightDirection;
    vec4 U_LightColor;
};