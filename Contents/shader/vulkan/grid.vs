#include "Uniform_CommonVS.inl"

// Fullscreen triangle — no vertex buffer needed.
// Uses gl_VertexIndex to generate 3 vertices covering the screen.
// Outputs near/far world-space positions for the fragment shader to
// intersect the Y=0 ground plane.

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

// Fullscreen triangle positions in NDC
vec3 gridPlane[6] = vec3[](
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 UnprojectPoint(float x, float y, float z) {
    // Reconstruct inverse VP per-row (vec4 rows stored in uniform)
    mat4 viewMat = mat4(U_ViewMatrix[0], U_ViewMatrix[1], U_ViewMatrix[2], U_ViewMatrix[3]);
    mat4 projMat = mat4(U_ProjectionMatrix[0], U_ProjectionMatrix[1], U_ProjectionMatrix[2], U_ProjectionMatrix[3]);
    mat4 viewProj = projMat * viewMat;
    mat4 invViewProj = inverse(viewProj);
    vec4 unprojectedPoint = invViewProj * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec3 p = gridPlane[gl_VertexIndex];
    nearPoint = UnprojectPoint(p.x, p.y, 0.0); // Near plane
    farPoint  = UnprojectPoint(p.x, p.y, 1.0); // Far plane
    gl_Position = vec4(p, 1.0);
}
