//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float4 lightpos;
	float4 eyepos;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
	float3 normal : NORMAL0;
	float4 surfpos : POSITION0;
	float2 tex : TEXCOORD0;
};

// 
float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 eyee;
	float3 lighte;

	eyee = float3(eyepos.x, eyepos.y, eyepos.z);
	lighte = float3(lightpos.x, lightpos.y, lightpos.z);

	float3 L, N, V, H;
	
	N = normalize(input.normal);
	V = (eyee - input.surfpos);
	V = normalize(V);
	L = (lighte - input.surfpos);
	L = normalize(L);
	float diffuse = dot(N,L);
	
	H = 0.5*(V + L);
	H = normalize(H);
	
	float spec = dot(N,H);
	spec = pow(spec, 275);
	
	float3 cr = float3(0.8, 0.5, 0.9);
		float amb = 0.1;
	
		float c = 0.5*diffuse + 1.3*spec + 0.1*amb;
		
		cr = float3(input.color.x, input.color.y, input.color.z);
		cr.b = eyepos.x / 10;
    return float4(cr*c, 1.0f);
}
