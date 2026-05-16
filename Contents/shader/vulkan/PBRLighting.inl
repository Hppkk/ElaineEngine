//=============================================================================
// PBRLighting.inl - Physically Based Rendering Lighting Library
//
// Shared GLSL include for both Forward and Deferred rendering paths.
// Implements Cook-Torrance microfacet BRDF with the following models:
//   - Normal Distribution Function (NDF): GGX / Trowbridge-Reitz
//   - Geometry Function: Smith's method with Schlick-GGX approximation
//   - Fresnel: Schlick approximation
//   - Image Based Lighting (IBL): Split-sum approximation
//
// Usage:
//   #include "PBRLighting.inl"
//   vec3 color = PBR_DirectionalLight(N, V, L, albedo, metallic, roughness, lightColor);
//=============================================================================

#ifndef PBR_LIGHTING_INL
#define PBR_LIGHTING_INL

//=============================================================================
// Constants
//=============================================================================
const float PI          = 3.14159265359;
const float INV_PI      = 0.31830988618;
const float EPSILON     = 1e-6;
const float MIN_ROUGHNESS = 0.04;

//=============================================================================
// Utility Functions
//=============================================================================

// Saturate / clamp to [0, 1]
float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3  saturateV(vec3 v)  { return clamp(v, vec3(0.0), vec3(1.0)); }

// Compute reflectance at normal incidence (F0) from metallic workflow
vec3 PBR_ComputeF0(vec3 albedo, float metallic)
{
    // Dielectric F0 is typically 0.04 (4% reflectance)
    // Metallic surfaces use the albedo color as F0
    return mix(vec3(0.04), albedo, metallic);
}

// Remap roughness to alpha (roughness^2) for NDF calculations
float PBR_RoughnessToAlpha(float roughness)
{
    float r = max(roughness, MIN_ROUGHNESS);
    return r * r;
}

//=============================================================================
// Normal Distribution Function (NDF) - GGX / Trowbridge-Reitz
//=============================================================================
// Models the distribution of microfacet normals.
// Higher roughness = wider distribution = more diffuse specular highlight.
float PBR_DistributionGGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + EPSILON);
}

//=============================================================================
// Geometry Function - Smith's method with Schlick-GGX
//=============================================================================
// Models self-shadowing of microfacets (geometry obstruction & shadowing).

// Single-direction Schlick-GGX geometry term
float PBR_GeometrySchlickGGX(float NdotV, float alpha)
{
    // Remapping for direct lighting (not IBL)
    float k = (alpha + 1.0);
    k = k * k / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + EPSILON);
}

// Smith's method: combine view and light direction geometry terms
float PBR_GeometrySmith(float NdotV, float NdotL, float alpha)
{
    float ggx1 = PBR_GeometrySchlickGGX(NdotV, alpha);
    float ggx2 = PBR_GeometrySchlickGGX(NdotL, alpha);
    return ggx1 * ggx2;
}

// IBL-specific geometry function (different k remapping)
float PBR_GeometrySchlickGGX_IBL(float NdotV, float alpha)
{
    float k = alpha / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k + EPSILON);
}

float PBR_GeometrySmith_IBL(float NdotV, float NdotL, float alpha)
{
    float ggx1 = PBR_GeometrySchlickGGX_IBL(NdotV, alpha);
    float ggx2 = PBR_GeometrySchlickGGX_IBL(NdotL, alpha);
    return ggx1 * ggx2;
}

//=============================================================================
// Fresnel - Schlick Approximation
//=============================================================================
// Models how reflectance changes based on viewing angle.
// At grazing angles, all surfaces become mirrors.

vec3 PBR_FresnelSchlick(float cosTheta, vec3 F0)
{
    float t = 1.0 - cosTheta;
    float t2 = t * t;
    float t5 = t2 * t2 * t;
    return F0 + (vec3(1.0) - F0) * t5;
}

// Fresnel with roughness (for IBL specular)
vec3 PBR_FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    float t = 1.0 - cosTheta;
    float t2 = t * t;
    float t5 = t2 * t2 * t;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * t5;
}

//=============================================================================
// Cook-Torrance Specular BRDF
//=============================================================================
// Returns the specular component of the BRDF (does NOT include kS)
vec3 PBR_CookTorranceSpecular(float NdotH, float NdotV, float NdotL, float alpha, vec3 F)
{
    float D = PBR_DistributionGGX(NdotH, alpha);
    float G = PBR_GeometrySmith(NdotV, NdotL, alpha);
    
    // Cook-Torrance: DFG / (4 * NdotV * NdotL)
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + EPSILON;
    return numerator / denominator;
}

//=============================================================================
// Lambertian Diffuse BRDF
//=============================================================================
vec3 PBR_LambertDiffuse(vec3 albedo)
{
    return albedo * INV_PI;
}

//=============================================================================
// Complete Direct Lighting Calculation
//=============================================================================

/**
 * Evaluate PBR direct lighting for a single light source.
 * @param N         Surface normal (normalized)
 * @param V         View direction (normalized, pointing toward camera)
 * @param L         Light direction (normalized, pointing toward light)
 * @param albedo    Surface base color
 * @param metallic  Metallic factor [0, 1]
 * @param roughness Roughness factor [0, 1]
 * @param radiance  Light radiance (color * intensity)
 * @return          Final lit color contribution from this light
 */
vec3 PBR_DirectLighting(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 radiance)
{
    vec3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001); // Avoid division by zero
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    if (NdotL <= 0.0)
        return vec3(0.0);
    
    float alpha = PBR_RoughnessToAlpha(roughness);
    vec3 F0 = PBR_ComputeF0(albedo, metallic);
    
    // Fresnel term
    vec3 F = PBR_FresnelSchlick(VdotH, F0);
    
    // Specular BRDF
    vec3 specular = PBR_CookTorranceSpecular(NdotH, NdotV, NdotL, alpha, F);
    
    // Energy conservation: diffuse = (1 - kS) * (1 - metallic)
    // Metallic surfaces have no diffuse component
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    
    // Diffuse BRDF (Lambertian)
    vec3 diffuse = PBR_LambertDiffuse(albedo);
    
    // Combine: (kD * diffuse + specular) * radiance * NdotL
    return (kD * diffuse + specular) * radiance * NdotL;
}

//=============================================================================
// Convenience Functions for Common Light Types
//=============================================================================

/**
 * Evaluate directional light.
 * @param N             Surface normal
 * @param V             View direction
 * @param lightDir      Light direction (pointing FROM the light, will be negated)
 * @param albedo        Base color
 * @param metallic      Metallic factor
 * @param roughness     Roughness factor
 * @param lightColor    Light color (pre-multiplied by intensity)
 * @return              Lit color
 */
vec3 PBR_DirectionalLight(vec3 N, vec3 V, vec3 lightDir, vec3 albedo, float metallic, float roughness, vec3 lightColor)
{
    vec3 L = normalize(-lightDir);
    return PBR_DirectLighting(N, V, L, albedo, metallic, roughness, lightColor);
}

/**
 * Evaluate point light with attenuation.
 * @param N             Surface normal
 * @param V             View direction
 * @param fragPos       Fragment world position
 * @param lightPos      Point light world position
 * @param albedo        Base color
 * @param metallic      Metallic factor
 * @param roughness     Roughness factor
 * @param lightColor    Light color
 * @param lightRadius   Light influence radius (for attenuation falloff)
 * @return              Lit color
 */
vec3 PBR_PointLight(vec3 N, vec3 V, vec3 fragPos, vec3 lightPos, vec3 albedo, float metallic, float roughness, vec3 lightColor, float lightRadius)
{
    vec3 lightVec = lightPos - fragPos;
    float distance = length(lightVec);
    vec3 L = lightVec / (distance + EPSILON);
    
    // Inverse-square attenuation with radius falloff
    float attenuation = 1.0 / (distance * distance + EPSILON);
    
    // Smooth falloff at the edge of the light radius
    float falloff = saturate(1.0 - pow(distance / lightRadius, 4.0));
    falloff = falloff * falloff;
    
    vec3 radiance = lightColor * attenuation * falloff;
    return PBR_DirectLighting(N, V, L, albedo, metallic, roughness, radiance);
}

/**
 * Evaluate spot light.
 * @param N             Surface normal
 * @param V             View direction
 * @param fragPos       Fragment world position
 * @param lightPos      Spot light world position
 * @param lightDir      Spot light direction (normalized, pointing FROM the light)
 * @param albedo        Base color
 * @param metallic      Metallic factor
 * @param roughness     Roughness factor
 * @param lightColor    Light color
 * @param lightRadius   Light influence radius
 * @param innerConeAngle Inner cone angle cosine (full intensity)
 * @param outerConeAngle Outer cone angle cosine (zero intensity)
 * @return              Lit color
 */
vec3 PBR_SpotLight(vec3 N, vec3 V, vec3 fragPos, vec3 lightPos, vec3 lightDir,
    vec3 albedo, float metallic, float roughness, vec3 lightColor,
    float lightRadius, float innerConeAngle, float outerConeAngle)
{
    vec3 lightVec = lightPos - fragPos;
    float distance = length(lightVec);
    vec3 L = lightVec / (distance + EPSILON);
    
    // Spot cone attenuation
    float theta = dot(L, normalize(-lightDir));
    float spotAttenuation = saturate((theta - outerConeAngle) / (innerConeAngle - outerConeAngle + EPSILON));
    spotAttenuation = spotAttenuation * spotAttenuation;  // Smooth falloff
    
    // Distance attenuation
    float distAttenuation = 1.0 / (distance * distance + EPSILON);
    float falloff = saturate(1.0 - pow(distance / lightRadius, 4.0));
    falloff = falloff * falloff;
    
    vec3 radiance = lightColor * distAttenuation * falloff * spotAttenuation;
    return PBR_DirectLighting(N, V, L, albedo, metallic, roughness, radiance);
}

//=============================================================================
// Shadow Mapping Utilities
//=============================================================================

/**
 * Sample shadow map with PCF (Percentage Closer Filtering).
 * @param shadowMap     Shadow depth texture (sampler2D)
 * @param shadowCoord   Fragment position in light clip space (after perspective divide)
 * @param bias          Depth bias to prevent shadow acne
 * @return              Shadow factor [0, 1] where 0 = fully shadowed, 1 = fully lit
 */
float PBR_ShadowPCF(sampler2D shadowMap, vec3 shadowCoord, float bias)
{
    // Check if fragment is outside shadow map bounds
    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 ||
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        shadowCoord.z > 1.0)
    {
        return 1.0; // Not in shadow
    }
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    
    // 3x3 PCF kernel
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, shadowCoord.xy + vec2(x, y) * texelSize).r;
            shadow += (shadowCoord.z - bias > pcfDepth) ? 0.0 : 1.0;
        }
    }
    
    return shadow / 9.0;
}

/**
 * Compute adaptive shadow bias based on surface normal and light direction.
 * Reduces shadow acne on surfaces at steep angles to the light.
 */
float PBR_ComputeShadowBias(vec3 N, vec3 L, float baseBias)
{
    float NdotL = max(dot(N, L), 0.0);
    return max(baseBias * (1.0 - NdotL), baseBias * 0.1);
}

//=============================================================================
// Normal Encoding/Decoding (for GBuffer)
//=============================================================================

/**
 * Encode world-space normal to [0, 1] range for storage in GBuffer.
 * Uses simple N * 0.5 + 0.5 encoding.
 */
vec3 PBR_EncodeNormal(vec3 N)
{
    return N * 0.5 + 0.5;
}

/**
 * Decode normal from [0, 1] range back to world-space [-1, 1].
 */
vec3 PBR_DecodeNormal(vec3 encoded)
{
    return normalize(encoded * 2.0 - 1.0);
}

/**
 * Octahedron normal encoding (more precision, 2 channels).
 * Encodes a unit normal into 2 floats in [0, 1].
 */
vec2 PBR_OctahedronEncode(vec3 N)
{
    vec3 n = N / (abs(N.x) + abs(N.y) + abs(N.z));
    if (n.z < 0.0)
    {
        n.xy = (1.0 - abs(n.yx)) * vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
    }
    return n.xy * 0.5 + 0.5;
}

/**
 * Octahedron normal decoding.
 */
vec3 PBR_OctahedronDecode(vec2 encoded)
{
    vec2 f = encoded * 2.0 - 1.0;
    vec3 N = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = saturate(-N.z);
    N.xy += vec2(N.x >= 0.0 ? -t : t, N.y >= 0.0 ? -t : t);
    return normalize(N);
}

//=============================================================================
// World Position Reconstruction from Depth
//=============================================================================

/**
 * Reconstruct world-space position from depth buffer.
 * @param uv              Screen UV [0, 1]
 * @param depth           Depth value from depth buffer
 * @param invViewProj     Inverse view-projection matrix (passed as 4 vec4 rows)
 * @return                World-space position
 */
vec3 PBR_ReconstructWorldPosition(vec2 uv, float depth, vec4 invViewProj[4])
{
    // Convert UV to NDC [-1, 1]
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    // Flip Y for Vulkan
    clipPos.y = -clipPos.y;
    
    // Transform to world space
    vec4 worldPos;
    worldPos.x = dot(invViewProj[0], clipPos);
    worldPos.y = dot(invViewProj[1], clipPos);
    worldPos.z = dot(invViewProj[2], clipPos);
    worldPos.w = dot(invViewProj[3], clipPos);
    
    return worldPos.xyz / worldPos.w;
}

//=============================================================================
// Tone Mapping & Color Space
//=============================================================================

/**
 * ACES (Academy Color Encoding System) tone mapping.
 * Maps HDR values to LDR with a film-like response curve.
 */
vec3 PBR_ToneMapACES(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturateV((color * (a * color + b)) / (color * (c * color + d) + e));
}

/**
 * Reinhard tone mapping (simple).
 */
vec3 PBR_ToneMapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

/**
 * Uncharted 2 tone mapping (filmic).
 */
vec3 PBR_ToneMapUncharted2Partial(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 PBR_ToneMapUncharted2(vec3 color, float exposureBias)
{
    vec3 curr = PBR_ToneMapUncharted2Partial(color * exposureBias);
    vec3 whiteScale = vec3(1.0) / PBR_ToneMapUncharted2Partial(vec3(11.2));
    return curr * whiteScale;
}

/**
 * Linear to sRGB gamma correction.
 */
vec3 PBR_LinearToSRGB(vec3 color)
{
    vec3 lo = color * 12.92;
    vec3 hi = pow(color, vec3(1.0 / 2.4)) * 1.055 - 0.055;
    return mix(lo, hi, step(vec3(0.0031308), color));
}

/**
 * sRGB to linear.
 */
vec3 PBR_SRGBToLinear(vec3 color)
{
    vec3 lo = color / 12.92;
    vec3 hi = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), color));
}

//=============================================================================
// Ambient Occlusion
//=============================================================================

/**
 * Multi-bounce AO approximation (from Jimenez, SIGGRAPH 2016).
 * Accounts for inter-reflection of ambient light in occluded areas.
 */
vec3 PBR_MultiBounceAO(float ao, vec3 albedo)
{
    vec3 a = 2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c = 2.7552 * albedo + 0.6903;
    float x = ao;
    return max(vec3(x), ((x * a + b) * x + c) * x);
}

//=============================================================================
// IBL (Image Based Lighting) Helpers
//=============================================================================

/**
 * Compute the reflection vector for IBL specular.
 */
vec3 PBR_GetReflectionVector(vec3 N, vec3 V)
{
    return reflect(-V, N);
}

/**
 * Compute the mip level for IBL specular based on roughness.
 * @param roughness     Material roughness
 * @param maxMipLevel   Maximum mip level of the environment map
 */
float PBR_ComputeIBLMipLevel(float roughness, float maxMipLevel)
{
    return roughness * maxMipLevel;
}

/**
 * IBL Diffuse: Sample irradiance map.
 * In practice, this would sample from a precomputed irradiance cubemap.
 * Here we provide a simple hemisphere approximation as fallback.
 */
vec3 PBR_IBLDiffuseFallback(vec3 N, vec3 skyColor, vec3 groundColor)
{
    float NdotUp = N.y * 0.5 + 0.5;
    return mix(groundColor, skyColor, NdotUp);
}

/**
 * IBL Specular: Approximate the BRDF integration LUT.
 * This is the split-sum approximation's second part.
 * Uses the analytic approximation from Karis (UE4).
 */
vec2 PBR_IntegrateBRDF_Approx(float NdotV, float roughness)
{
    // Polynomial fit to the BRDF LUT
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

/**
 * Complete IBL contribution (ambient).
 * Combines diffuse irradiance and specular reflection.
 */
vec3 PBR_AmbientLighting(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float ao,
    vec3 irradiance, vec3 prefilteredSpecular, vec2 brdfLUT)
{
    vec3 F0 = PBR_ComputeF0(albedo, metallic);
    float NdotV = max(dot(N, V), 0.001);
    
    vec3 F = PBR_FresnelSchlickRoughness(NdotV, F0, roughness);
    
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    
    vec3 diffuse = kD * albedo * irradiance;
    vec3 specular = prefilteredSpecular * (F * brdfLUT.x + brdfLUT.y);
    
    return (diffuse + specular) * ao;
}

/**
 * Simplified ambient lighting when no IBL maps are available.
 * Uses a simple hemisphere sky + ground color.
 */
vec3 PBR_SimpleAmbient(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float ao,
    vec3 ambientColor)
{
    vec3 F0 = PBR_ComputeF0(albedo, metallic);
    float NdotV = max(dot(N, V), 0.001);
    
    vec3 F = PBR_FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    
    vec3 ambient = kD * albedo * ambientColor;
    
    // Simple specular ambient approximation
    ambient += F * ambientColor * (1.0 - roughness) * 0.3;
    
    return ambient * ao;
}

#endif // PBR_LIGHTING_INL
