//----------------------------------------------------------------------------------------------------
struct vs_input_t
{
    float3 modelPosition : VERTEX_POSITION;
    float4 color : VERTEX_COLOR;
    float2 uv : VERTEX_UVTEXCOORDS;
    float3 modelTangent : VERTEX_TANGENT;
    float3 modelBitangent : VERTEX_BITANGENT;
    float3 modelNormal : VERTEX_NORMAL;
};

//----------------------------------------------------------------------------------------------------
struct v2p_t
{
    float4 clipPosition : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 worldPosition : WORLDPOS;
    float3 worldTangent : TANGENT;
    float3 worldBitangent : BITANGENT;
    float3 worldNormal : NORMAL;
};

//----------------------------------------------------------------------------------------------------
// NEW: PerFrameConstants buffer (b1)
cbuffer PerFrameConstants : register(b1)
{
    float c_time;
    int c_debugInt;
    float c_debugFloat;
};

//----------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b2)  // Moved to b2
{
    float3 SunDirection;
    float SunIntensity;
    float AmbientIntensity;
};

//----------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b3)  // Moved to b3
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
};

//----------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b4)  // Moved to b4
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

//----------------------------------------------------------------------------------------------------
// Multitexturing: diffuse and normal map
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);

SamplerState diffuseSampler : register(s0);
SamplerState normalSampler : register(s1);

//----------------------------------------------------------------------------------------------------
// Utility Functions

// Encode XYZ -> RGB: [-1,1] -> [0,1]
float3 EncodeXYZToRGB(float3 xyz)
{
    return xyz * 0.5f + 0.5f;
}

// Decode RGB -> XYZ: [0,1] -> [-1,1]
float3 DecodeRGBToXYZ(float3 rgb)
{
    return rgb * 2.0f - 1.0f;
}

//----------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
    float4 modelPosition = float4(input.modelPosition, 1);
    float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
    float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
    float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
    float4 clipPosition = mul(RenderToClipTransform, renderPosition);

    // Transform TBN vectors to world space (fixing the bug in original)
    float3 worldTangent = normalize(mul((float3x3)ModelToWorldTransform, input.modelTangent));
    float3 worldBitangent = normalize(mul((float3x3)ModelToWorldTransform, input.modelBitangent));
    float3 worldNormal = normalize(mul((float3x3)ModelToWorldTransform, input.modelNormal));

    v2p_t v2p;
    v2p.clipPosition = clipPosition;
    v2p.color = input.color;
    v2p.uv = input.uv;
    v2p.worldPosition = worldPosition.xyz;
    v2p.worldTangent = worldTangent;
    v2p.worldBitangent = worldBitangent;
    v2p.worldNormal = worldNormal;
    return v2p;
}

//----------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
    // Renormalize interpolated vectors
    float3 worldTangent = normalize(input.worldTangent);
    float3 worldBitangent = normalize(input.worldBitangent);
    float3 worldNormal = normalize(input.worldNormal);

    // Create TBN-to-World transformation matrix
    // Note: HLSL float3x3(T,B,N) creates columns, so we need to transpose
    float3x3 tbnToWorld = transpose(float3x3(worldTangent, worldBitangent, worldNormal));

    // Sample textures
    float4 diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    float3 normalMapRGB = normalTexture.Sample(normalSampler, input.uv).rgb;

    // Convert normal map from RGB to tangent-space normal
    float3 tangentSpaceNormal = normalize(DecodeRGBToXYZ(normalMapRGB));

    // Transform to world space
    float3 worldMicroNormal = normalize(mul(tangentSpaceNormal, tbnToWorld));

    // Debug visualization modes based on c_debugInt
    if (c_debugInt == 1) // Diffuse map texel
    {
        return float4(diffuseColor.rgb, 1.0f);
    }
    else if (c_debugInt == 2) // Normal map texel
    {
        return float4(normalMapRGB, 1.0f);
    }
    else if (c_debugInt == 3) // UV as Red, Green
    {
        return float4(input.uv.x, input.uv.y, 0.0f, 1.0f);
    }
    else if (c_debugInt == 4) // World-space tangent
    {
        return float4(EncodeXYZToRGB(worldTangent), 1.0f);
    }
    else if (c_debugInt == 5) // World-space bitangent
    {
        return float4(EncodeXYZToRGB(worldBitangent), 1.0f);
    }
    else if (c_debugInt == 6) // World-space normal (vertex)
    {
        return float4(EncodeXYZToRGB(worldNormal), 1.0f);
    }
    else if (c_debugInt == 7) // World-space normal (normal map)
    {
        return float4(EncodeXYZToRGB(worldMicroNormal), 1.0f);
    }
    else if (c_debugInt == 8) // Light strength vs vertex normal
    {
        float lightStrength = saturate(dot(worldNormal, -SunDirection));
        return float4(lightStrength.xxx, 1.0f);
    }
    else if (c_debugInt == 9) // Light strength vs normal map normal
    {
        float lightStrength = saturate(dot(worldMicroNormal, -SunDirection));
        return float4(lightStrength.xxx, 1.0f);
    }

    // Normal rendering with normal mapping
    float ambient = AmbientIntensity;
    float directional = SunIntensity * saturate(dot(worldMicroNormal, -SunDirection));
    float4 lightColor = float4((ambient + directional).xxx, 1);
    float4 vertexColor = input.color;
    float4 modelColor = ModelColor;
    float4 color = lightColor * diffuseColor * vertexColor * modelColor;

    clip(color.a - 0.01f);
    return color;
}