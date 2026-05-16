// ============================================================================
// VirtualTexture.inl - Virtual Texture Sampling Library for GLSL
// ============================================================================
// 
// Include this file in any shader that samples from a virtual texture.
// Provides VTSample() and VTSampleGrad() functions for virtual texture access.
//
// Required bindings (set per VT space):
//   - Indirection Texture: stores virtual page -> physical atlas mapping
//   - Physical Atlas Texture(s): the actual tile pixel data
//   - Sampler: for physical atlas sampling (linear/anisotropic)
//

// ============================================================================
// Constants (must match C++ VTConstants)
// ============================================================================

#define VT_TILE_SIZE           128.0
#define VT_TILE_BORDER_SIZE    4.0
#define VT_TILE_SIZE_WITH_BORDER (VT_TILE_SIZE + VT_TILE_BORDER_SIZE * 2.0)
#define VT_INVALID_TILE        0xFFFF
#define VT_MAX_MIP_LEVELS      12

// ============================================================================
// VT Uniforms
// ============================================================================

// Per virtual texture space parameters
struct VTSpaceParams
{
    vec2  VirtualTextureSize;      // Virtual texture size at mip 0
    vec2  InvVirtualTextureSize;   // 1.0 / VirtualTextureSize
    float TileSize;                // 128.0
    float InvTileSize;             // 1.0 / 128.0
    float MaxMipLevel;             // Max available mip level
    float PhysicalAtlasSize;       // Physical atlas texture size in pixels
};

// ============================================================================
// Helper Functions
// ============================================================================

// Compute the mip level for a virtual texture based on UV derivatives
float VTComputeMipLevel(vec2 virtualUV, vec2 virtualTextureSize)
{
    vec2 texCoord = virtualUV * virtualTextureSize;
    vec2 dx = dFdx(texCoord);
    vec2 dy = dFdy(texCoord);
    float maxDelta = max(dot(dx, dx), dot(dy, dy));
    float mipLevel = 0.5 * log2(maxDelta);
    
    // Bias: we want the mip level relative to tile size, not pixel size
    // Each tile covers TileSize pixels, so subtract log2(TileSize)
    // mipLevel = mipLevel - log2(VT_TILE_SIZE);
    
    return max(0.0, mipLevel);
}

// Compute mip level using explicit gradients
float VTComputeMipLevelGrad(vec2 ddx, vec2 ddy, vec2 virtualTextureSize)
{
    vec2 dx = ddx * virtualTextureSize;
    vec2 dy = ddy * virtualTextureSize;
    float maxDelta = max(dot(dx, dx), dot(dy, dy));
    return max(0.0, 0.5 * log2(maxDelta));
}

// ============================================================================
// Indirection Texture Lookup
// ============================================================================

// Read the indirection texture to find physical atlas location for a virtual page
// Indirection texture format: RGBA16_UINT
//   R = PhysicalTileX
//   G = PhysicalTileY
//   B = packed [PoolIndex:8 | MipBias:8]
//   A = flags (bit 0 = IsResident)
struct VTIndirectionResult
{
    vec2  physicalTileCoord;  // Physical tile position in atlas (in tiles)
    float mipBias;            // How many mip levels coarser than requested
    float poolIndex;          // Which physical pool
    bool  isResident;         // Whether the exact requested mip is resident
};

VTIndirectionResult VTReadIndirection(usampler2D indirectionTex, vec2 virtualUV, float mipLevel)
{
    VTIndirectionResult result;
    
    // Sample the indirection texture at the appropriate mip level
    // The indirection texture is dimensioned as (TilesX x TilesY) at each mip
    ivec2 indTexSize = textureSize(indirectionTex, int(mipLevel));
    ivec2 indCoord = ivec2(virtualUV * vec2(indTexSize));
    indCoord = clamp(indCoord, ivec2(0), indTexSize - ivec2(1));
    
    uvec4 indData = texelFetch(indirectionTex, indCoord, int(mipLevel));
    
    result.physicalTileCoord = vec2(float(indData.r), float(indData.g));
    result.mipBias = float(indData.b & 0xFFu);
    result.poolIndex = float((indData.b >> 8u) & 0xFFu);
    result.isResident = (indData.a & 1u) != 0u;
    
    return result;
}

// ============================================================================
// Physical Atlas Sampling
// ============================================================================

// Convert virtual UV + indirection result to physical atlas UV
vec2 VTComputePhysicalUV(
    vec2 virtualUV,
    VTIndirectionResult indirection,
    float physicalAtlasSize,
    float mipLevel)
{
    // Which tile within the virtual texture at this mip level
    float effectiveMip = mipLevel + indirection.mipBias;
    float tileCountAtMip = max(1.0, floor(1.0 / (VT_TILE_SIZE * pow(2.0, effectiveMip))));
    
    // Fractional position within the tile [0, 1]
    vec2 tileCountVec = vec2(tileCountAtMip);
    vec2 withinTile = fract(virtualUV * tileCountVec);
    
    // Physical tile pixel position in the atlas
    vec2 physicalTileOrigin = indirection.physicalTileCoord * VT_TILE_SIZE_WITH_BORDER;
    
    // Offset within the tile: skip border, then scale to tile content area
    vec2 physicalPixelPos = physicalTileOrigin 
        + vec2(VT_TILE_BORDER_SIZE)   // Skip border
        + withinTile * vec2(VT_TILE_SIZE); // Position within tile content
    
    // Convert to [0, 1] UV in the physical atlas
    return physicalPixelPos / vec2(physicalAtlasSize);
}

// ============================================================================
// Main VT Sampling Functions
// ============================================================================

// Sample a virtual texture with automatic mip level selection
vec4 VTSample(
    usampler2D indirectionTex,
    sampler2D  physicalAtlas,
    vec2       virtualUV,
    VTSpaceParams params)
{
    // 1. Compute mip level from screen-space derivatives
    float mipLevel = VTComputeMipLevel(virtualUV, params.VirtualTextureSize);
    mipLevel = clamp(mipLevel, 0.0, params.MaxMipLevel);
    float mipFloor = floor(mipLevel);
    
    // 2. Look up indirection texture
    VTIndirectionResult indirection = VTReadIndirection(indirectionTex, virtualUV, mipFloor);
    
    // 3. Compute physical atlas UV
    vec2 physicalUV = VTComputePhysicalUV(
        virtualUV, indirection, params.PhysicalAtlasSize, mipFloor);
    
    // 4. Sample physical atlas (use hardware bilinear filtering within the tile)
    // Note: We use texture() with LOD 0 since we've already selected the correct
    // mip through the indirection texture
    return textureLod(physicalAtlas, physicalUV, 0.0);
}

// Sample a virtual texture with explicit gradients (for use in flow control)
vec4 VTSampleGrad(
    usampler2D indirectionTex,
    sampler2D  physicalAtlas,
    vec2       virtualUV,
    vec2       ddxUV,
    vec2       ddyUV,
    VTSpaceParams params)
{
    // 1. Compute mip level from explicit gradients
    float mipLevel = VTComputeMipLevelGrad(ddxUV, ddyUV, params.VirtualTextureSize);
    mipLevel = clamp(mipLevel, 0.0, params.MaxMipLevel);
    float mipFloor = floor(mipLevel);
    
    // 2. Look up indirection texture
    VTIndirectionResult indirection = VTReadIndirection(indirectionTex, virtualUV, mipFloor);
    
    // 3. Compute physical atlas UV
    vec2 physicalUV = VTComputePhysicalUV(
        virtualUV, indirection, params.PhysicalAtlasSize, mipFloor);
    
    // 4. Sample with explicit gradients scaled to physical atlas space
    // Scale gradients from virtual UV space to physical atlas pixel space
    float scaleFactor = VT_TILE_SIZE / (params.PhysicalAtlasSize * pow(2.0, indirection.mipBias));
    vec2 physDdx = ddxUV * scaleFactor;
    vec2 physDdy = ddyUV * scaleFactor;
    
    return textureGrad(physicalAtlas, physicalUV, physDdx, physDdy);
}

// Sample a virtual texture at a specific mip level (no derivatives needed)
vec4 VTSampleLevel(
    usampler2D indirectionTex,
    sampler2D  physicalAtlas,
    vec2       virtualUV,
    float      mipLevel,
    VTSpaceParams params)
{
    mipLevel = clamp(mipLevel, 0.0, params.MaxMipLevel);
    float mipFloor = floor(mipLevel);
    
    VTIndirectionResult indirection = VTReadIndirection(indirectionTex, virtualUV, mipFloor);
    
    vec2 physicalUV = VTComputePhysicalUV(
        virtualUV, indirection, params.PhysicalAtlasSize, mipFloor);
    
    return textureLod(physicalAtlas, physicalUV, 0.0);
}

// ============================================================================
// Feedback Pass Output
// ============================================================================

// For the feedback pass: compute and output the packed tile request
// Returns the packed VTTileCoord for the current pixel
uint VTComputeFeedback(vec2 virtualUV, uint spaceID, VTSpaceParams params)
{
    // Compute mip level
    float mipLevel = VTComputeMipLevel(virtualUV, params.VirtualTextureSize);
    mipLevel = clamp(mipLevel, 0.0, params.MaxMipLevel);
    uint mipFloor = uint(floor(mipLevel));
    
    // Compute tile coordinate at this mip level
    float mipScale = pow(2.0, float(mipFloor));
    vec2 virtualSizeAtMip = params.VirtualTextureSize / mipScale;
    vec2 tileCoordsFloat = virtualUV * virtualSizeAtMip * params.InvTileSize;
    
    uint tileX = uint(floor(tileCoordsFloat.x));
    uint tileY = uint(floor(tileCoordsFloat.y));
    
    // Clamp to valid range (12 bits each)
    tileX = min(tileX, 0xFFFu);
    tileY = min(tileY, 0xFFFu);
    mipFloor = min(mipFloor, 0xFu);
    spaceID = min(spaceID, 0xFu);
    
    // Pack: [SpaceID:4][MipLevel:4][Y:12][X:12]
    return (spaceID << 28u) | (mipFloor << 24u) | (tileY << 12u) | tileX;
}

// ============================================================================
// Debug Visualization
// ============================================================================

// Visualize mip levels with false color
vec3 VTDebugMipColor(float mipLevel)
{
    vec3 colors[8] = vec3[](
        vec3(1.0, 0.0, 0.0),  // Mip 0: Red (highest detail)
        vec3(1.0, 0.5, 0.0),  // Mip 1: Orange
        vec3(1.0, 1.0, 0.0),  // Mip 2: Yellow
        vec3(0.0, 1.0, 0.0),  // Mip 3: Green
        vec3(0.0, 1.0, 1.0),  // Mip 4: Cyan
        vec3(0.0, 0.0, 1.0),  // Mip 5: Blue
        vec3(0.5, 0.0, 1.0),  // Mip 6: Purple
        vec3(1.0, 0.0, 1.0)   // Mip 7+: Magenta
    );
    
    int idx = clamp(int(mipLevel), 0, 7);
    return colors[idx];
}

// Visualize tile boundaries as a grid overlay
float VTDebugTileGrid(vec2 virtualUV, float mipLevel, vec2 virtualTextureSize)
{
    float mipScale = pow(2.0, floor(mipLevel));
    vec2 tileUV = fract(virtualUV * virtualTextureSize / (VT_TILE_SIZE * mipScale));
    vec2 border = step(tileUV, vec2(0.02)) + step(vec2(0.98), tileUV);
    return max(border.x, border.y);
}
