//------------------------------------------------------------------------------------------------
// Blinn-Phong (lit) shader for Squirrel's C34 SD Engine (circa SD2 Spring 2025)
//	written by Squirrel Eiserloh, May 2025
//
// Requires Vertex_PCUTBN vertex data (includes tangent, bitangent, normal).
//------------------------------------------------------------------------------------------------
// D3D11 basic rendering pipeline stages:
//	IA = Input Assembler (grouping verts 3 at a time to form triangles, or N to form lines, fans, chains, etc.)
//	VS = Vertex Shader (transforming vertexes; moving them around, and computing them in different spaces)
//	RS = Rasterization Stage (converting math triangles into discrete pixels covered, interpolating values within)
//	PS = Pixel Shader (a.k.a. Fragment Shader, computing the actual output color(s) at each pixel being drawn)
//	OM = Output Merger (combining PS output with existing colors, using the current blend mode: additive, alpha, etc.)
//------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
// Input to the Vertex shader stage.
// Information contained per vertex, pulled from the VBO being drawn.
//------------------------------------------------------------------------------------------------
struct VertexInput
{
	// "a_" stands for for vertex "Attribute" which comes directly from VBO data (Squirrel's personal convention)
	// Type (after conversion)   name (as used in shader)  :  semanticName (arbitrary symbol to associate CPU-GPU and other linkages);
	float3	a_position		: POSITION;		
	float4	a_color			: TINT; // Expanded to float[0.f,1.f] from byte[0,255] because "UNORM" in DXGI_FORMAT_R8G8B8A8_UNORM
	float2	a_uvTexCoords	: TEXCOORDS; 
	float3	a_tangent		: TANGENT; 
	float3	a_bitangent		: BITANGENT; 
	float3	a_normal		: NORMAL; 
	
	// Built-in / automatic attributes (not part of incoming VBO data)
	// "SV_" means "System Variable" and is a built-in special reserved semantic
	uint	a_vertexID	: SV_VertexID; // Which vertex number in the VBO collection this is (automatic variable)
};


//------------------------------------------------------------------------------------------------
// Output passed from the Vertex shader into the Pixel/fragment shader.
// 
// Each of these values is automatically 3-way (barycentric) interpolated across the surface of
//	the triangle on a per-pixel basis during the Rasterization Stage (RS).
// "v_" stands for "Varying" meaning "barycentric-lepred" (Squirrel's personal convention)
//
// Note that the SV_Position variable is required, and expects the Vertex Shader (VS) to output
//	this variable in clip space; after the VS stage, before the Rasterization Stage (RS), this position
//	gets divided by its w value to convert from clip space to NDC (Normalized Device Coordinates).
//
// It is then 3-way (barycentric) interpolated across the surface of the triangle along with the
//	other variables here; the Pixel Shader (PS) stage then receives these interpolated values
//	which will be unique per pixel, and the SV_Position variable will be in NDC space.
//
// Semantic names other than "SV_" (System Variables) are arbitrary, and just need to match up
//	between the variable in the Vertex Shader output structure and the corresponding variable in the
//	Pixel Shader input structure.  Since we use the same structure for both, they all automatically
//	match up.
//------------------------------------------------------------------------------------------------
struct VertexOutPixelIn 
{
	float4 v_position		: SV_Position; // Required; VS output as clip-space vertex position; PS input as NDC pixel position.
	float4 v_color			: VERTEX_COLOR;
	float2 v_uvTexCoords	: TEXTURE_COORDS;
	float3 v_worldPos		: WORLD_POSITION;
	float3 v_worldTangent	: WORLD_TANGENT;
	float3 v_worldBitangent	: WORLD_BITANGENT;
	float3 v_worldNormal	: WORLD_NORMAL;
};


//------------------------------------------------------------------------------------------------
// CONSTANT BUFFERS (CBOs, a.k.a. UBOs or Uniform Buffers in OpenGL)
//	"u_" stands for "Uniform", Squirrel's traditional 
//
// There are 14 available CBO "slots" or "registers" (b0 through b13).
// In C++ code we bind structures into CBO slots when we call:
//	m_d3dContext->VSSetConstantBuffers( slot, 1, &cbo->m_gpuBuffer ); // if slot==5, we are binding to register(b5)
//	m_d3dContext->PSSetConstantBuffers( slot, 1, &cbo->m_gpuBuffer );
//
// We might update some CBOs once per frame; others perhaps between each draw call; others only occasionally.
// CBOs have very picky alignment rules, but can otherwise be anything we want (max of 64k == 65536 bytes each).
//
// Guildhall-specific conventions we use for different CBO register slot numbers:
//	b0 = Engine/System-Level constants (e.g. debug modes) -- updated rarely
//	b1 = Per-Frame constants (e.g. time) -- updated once per frame
//	b2 = Camera constants (e.g. view/proj matrices) -- updated once per CameraBegin
//	b3 = Model constants (e.g. model matrix & tint) -- updated once per draw call
//	b4-b7 = Other Engine-reserved slots
//	b8-b13 = Other Game-specific slots
//
// NOTE: Constant Buffers MUST be 16B-aligned (sizeof is a multiple of 16B), AND
//	also primitives may not cross 16B boundaries (unless they are 16B-aligned, like Mat44).
// So you must "pad out" any variables with dummy variables to make sure they adhere to these
//	rules, and make sure that your corresponding C++ struct has identical byte-layout to the shader struct.
// I find it easiest to think of this as the CBO having multiple rows, each row float4 (Vec4 == 16B) in size.
//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 u_renderToClip;	// a.k.a. "Projection" matrix (perpective or orthographic); render space to clip space
	float4x4 u_cameraToRender;	// a.k.a. "Game" matrix; axis-swaps from Game conventions (+X forward) to Render (+X right)
	float4x4 u_worldToCamera;	// a.k.a. "View" matrix; world space (+X east) to camera-relative space (+X camera-forward)
};


//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
    float4x4 u_modelToWorld;	// a.k.a. "Model" matrix; model local space (+X model forward) to world space (+X east)
    float4 u_modelTint;			// Uniform Vec4 model tint (including alpha) to multiply against diffuse texel & vertex color
};


//------------------------------------------------------------------------------------------------
// TEXTURE and SAMPLER constants
//
// There are 16 (on mobile) or 128 (on desktop) texture binding "slots" or "registers" (t0 through t15, or t127).
// There are 16 sampler slots (s0 through s15).
//
// In C++ code we bind textures into texture slots (t0 through t15 or t127) for use in the Pixel Shader when we call:
//	m_d3dContext->PSSetShaderResources( textureSlot, 1, &texture->m_shaderResourceView ); // e.g. (t3) if textureSlot==3
//
// In C++ code we bind texture samplers into sampler slots (s0 through s15) for use in the Pixel Shader when we call:
//	m_d3dContext->PSSetSamplers( samplerSlot, 1, &samplerState );  // e.g. (s3) if samplerSlot==3
//
// If we want to sample textures from within the Vertex Shader (VS), e.g. for displacement maps, we can also
//	use the VS versions of these C++ functions:
//	m_d3dContext->VSSetShaderResources( textureSlot, 1, &texture->m_shaderResourceView );
//	m_d3dContext->VSSetSamplers( samplerSlot, 1, &samplerState );
//------------------------------------------------------------------------------------------------
Texture2D<float4>	t_diffuseTexture : register(t0);	// Texture bound in texture constant slot #0 (t0)
SamplerState		s_diffuseSampler : register(s0);	// Sampler is bound in sampler constant slot #0 (s0)
// #ToDo: add additional textures/samples, for normal maps, specular/glossy/emissive maps, etc.


//------------------------------------------------------------------------------------------------
// VERTEX SHADER (VS)
//
// "Main" entry point for the Vertex Shader (VS) stage; this function (and functions it calls) are
//	the vertex shader program, called once per vertex.
//
// (The name of this entry function is chosen in C++ as a D3DCompile argument.)
//
// Inputs are typically vertex attributes (PCU, PCUTBN) coming from the VBO.
// Outputs include anything we want to pass through the Rasterization Stage (RS) to the Pixel Shader (PS).
//------------------------------------------------------------------------------------------------
VertexOutPixelIn VertexMain( VertexInput input )
{
    VertexOutPixelIn output;

	// Transform the position through the pipeline	
	float4 modelPos = float4( input.a_position, 1.0 );	// VBOs provide vertexes in model space
    float4 worldPos		= mul( u_modelToWorld, modelPos );		// Model space (+X local forward) to World space (+X east)
    float4 cameraPos	= mul( u_worldToCamera, worldPos );		// World space (+X east) to Camera space (+X camera-forward)
	float4 renderPos	= mul( u_cameraToRender, cameraPos );	// Camera space (+X cam-fwd) to Render space (+X right/+Z fwd)
    float4 clipPos		= mul( u_renderToClip, renderPos );		// Render space to Clip space (range-map/FOV/aspect, and put Z in W, preparing for W-divide)
	
	// Transform the tangents, normals, and bitangents (using W=0 for directions)
	float4 modelTangent		= float4( input.a_tangent, 0.0 );
	float4 modelBitangent	= float4( input.a_bitangent, 0.0 );
	float4 modelNormal		= float4( input.a_normal, 0.0 );
	float4 worldTangent		= mul( u_modelToWorld, modelTangent );
	float4 worldBitangent	= mul( u_modelToWorld, modelBitangent );
	float4 worldNormal		= mul( u_modelToWorld, modelNormal );

	// Set the outputs we want to pass through Rasterization Stage (RS) down to the Pixel Shader (PS)
    output.v_position		= clipPos;
    output.v_color			= input.a_color;
    output.v_uvTexCoords	= input.a_uvTexCoords;
	output.v_worldPos		= worldPos.xyz;
	output.v_worldTangent	= worldTangent.xyz;
	output.v_worldBitangent	= worldBitangent.xyz;
	output.v_worldNormal	= worldNormal.xyz;
	// #ToDo: we could also pass world position, or camera position, or tangent/bitangent/normals, etc.

    return output; // Pass to Rasterization Stage (RS) for barycentric interpolation, then into Pixel Shader (PS)
}


//------------------------------------------------------------------------------------------------
// Used standard normal color encoding, mapping xyz in [-1,1] to rgb in [0,1]
//------------------------------------------------------------------------------------------------
float3 EncodeXYZToRGB( float3 vec )
{
	return (vec + 1.0) * 0.5;
}


//------------------------------------------------------------------------------------------------
// Used standard normal color encoding, mapping xyz in [-1,1] to rgb in [0,1]
//------------------------------------------------------------------------------------------------
float3 DecodeRGBToXYZ( float3 color )
{
	return (color * 2.0) - 1.0;
}


//------------------------------------------------------------------------------------------------
// PIXEL SHADER (PS)
//
// "Main" entry point for the Pixel Shader (PS) stage; this function (and functions it calls) are
//	the pixel shader program.
//
// (The name of this entry function is chosen in C++ as a D3DCompile argument.)
//
// Inputs are typically the barycentric-interpolated outputs from the Vertex Shader (VS) via Rasterization.
// Output is the color sent to the render target, to be blended via the Output Merger (OM) blend mode settings.
// If we have multiple outputs (colors to write to each of several different Render Targets), we can change
//	this function to return a structure containing multiple float4 output colors, one per target.
//------------------------------------------------------------------------------------------------
float4 PixelMain( VertexOutPixelIn input ) : SV_Target0
{
	// Get the UV coordinates that were mapped onto this pixel
	float2 uvCoords = input.v_uvTexCoords;
	
	// Sample the diffuse map texture to see what this looks like at this pixel
	float4 diffuseTexel = t_diffuseTexture.Sample( s_diffuseSampler, uvCoords );
	float4 surfaceColor = input.v_color;
	float4 modelColor = u_modelTint;
	
	// Tint diffuse color based on overall model tinting (including alpha translucency)
	float4 diffuseColor = diffuseTexel * surfaceColor * modelColor;
	
	// Fake directional light for now; #ToDo: add a (b4) or (b8) Light CBO
	float3 lightDir = normalize( float3( 3.0, 1.0, -2.0 ) );

	// Get TBN basis vectors
	float3 worldTangent		= normalize( input.v_worldTangent );
	float3 worldBitangent	= normalize( input.v_worldBitangent );
	float3 worldNormal		= normalize( input.v_worldNormal );

	// #ToDo: add lighting and such later!
	float diffuseLightDot = dot( -lightDir, worldNormal );
	float lightStrength = clamp( diffuseLightDot, 0.1, 1.0 );
	float4 finalColor = float4( diffuseColor.rgb * lightStrength, diffuseColor.a ); 
	if( finalColor.a <= 0.001 ) // a.k.a. "clip" in HLSL
	{
		discard;
	}
	
//	finalColor.rgb = float3( uvCoords.xy, 0.f );
//	finalColor.rgb = EncodeXYZToRGB( worldTangent );
//	finalColor.rgb = EncodeXYZToRGB( worldBitangent );
//	finalColor.rgb = EncodeXYZToRGB( worldNormal );
	return finalColor;
}

