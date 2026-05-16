//=============================================================================
// DeferredLighting.vs - Full-screen Quad Vertex Shader
//
// Generates a full-screen triangle (no vertex buffer needed).
// Uses VertexID to generate positions and UVs.
//=============================================================================

layout(location = 0) out vec2 vUV;

void main()
{
    // Full-screen triangle trick:
    // VertexID 0: (-1, -1) UV (0, 0)
    // VertexID 1: ( 3, -1) UV (2, 0)
    // VertexID 2: (-1,  3) UV (0, 2)
    vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);

    // Flip Y for Vulkan
    gl_Position.y = -gl_Position.y;
}
