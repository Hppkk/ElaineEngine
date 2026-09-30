#include "Uniform_CommonVS.inl"

// Fullscreen triangle. Uses gl_VertexIndex to generate 3 vertices covering the screen.
// Outputs near/far world-space positions for the fragment shader to
// intersect the Y=0 ground plane.

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

// Fullscreen triangle positions in NDC
vec2 gridPlane[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

mat4 GridMatrix(vec4 row0, vec4 row1, vec4 row2, vec4 row3) {
    // Common uniforms are stored as rows; GLSL mat4 constructors take columns.
    return transpose(mat4(row0, row1, row2, row3));
}

vec3 UnprojectPoint(float x, float y, float z) {
    mat4 viewMat = GridMatrix(U_ViewMatrix[0], U_ViewMatrix[1], U_ViewMatrix[2], U_ViewMatrix[3]);
    mat4 projMat = GridMatrix(U_ProjectionMatrix[0], U_ProjectionMatrix[1], U_ProjectionMatrix[2], U_ProjectionMatrix[3]);
    mat4 viewProj = projMat * viewMat;
    mat4 invViewProj = inverse(viewProj);
    vec4 unprojectedPoint = invViewProj * vec4(x, y, z, 1.0);
    if (abs(unprojectedPoint.w) < 0.000001)
        return vec3(0.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec2 p = gridPlane[gl_VertexIndex];
    nearPoint = UnprojectPoint(p.x, p.y, 0.0);
    farPoint  = UnprojectPoint(p.x, p.y, 1.0);
    gl_Position = vec4(p, 0.0, 1.0);
}
