
Texture2D canvas : register(t0);
SamplerState mysampler : register(s0);

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float4 time; // use first element as timer
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

float4 main(PixelShaderInput input) : SV_TARGET
{
	Texture2D texture2;
	float t = time.r;
	float3 wipreColour = {1.0f, 0.0f, 0.5f };
	float3 cr = { 0.6, 0.4, 0.9 };
	float3 can2;
	float2 tv = input.tex;
	bool isWiper = false;

	//Part 2. TV static effect
	//tv.r = tv.r + 0.05*(tan(t / 10. + 8 * tv.g));
		
	float3 effect;

	//effect = canvas.Sample(mysampler, tv);

	//Multiply by two for Part 1B. Show a 2X2 tiling of the scene over the image plane.
	effect = canvas.Sample(mysampler, tv*2);

	//Part 1D. Make a strong blur effect. (Suggestion: average at least 25 texture reads. Use a for loop or nested for loops.)
	/*for (int blur = 0; blur < 25; blur++){
		effect += canvas.Sample(mysampler, (tv.xy - blur/100) );
	//	effect += canvas.Sample(mysampler, (tv.xy + blur / 100));
	}
	effect /= 25;*/

	// "transmission" horizontal and vertical lines:
	if (((int)(input.tex.r * 1920)) % 12 < 2)
		effect = (float3)0;

	if (((int)(input.tex.g * 1200)) % 12 < 2)
		effect = effect * 0;
	
	// threshold:
	if (effect.r + effect.g + effect.b > 0.3) effect = (float3)1.0; else effect = (float3)0;

	/*Part 1A. A horizontal wipe, where the screen turns 
	(say) blue beginning from the left and progressing 
	to the right. When the wipe is done the screen should 
	remain blue for about 1 second and then return to normal.
	*/
	if (((int)((0 - input.tex.r) + t / 15)) % 20 > 15 && (effect.r + effect.g + effect.b > 0.3)){
		isWiper = true;
	}
	else {
		isWiper = false;
	}
	if (isWiper && (t/15) % 20 >15){
		effect = wipreColour;
	}
	
	//Part 2. add magnet on screen effect to go with static
	float3 result = effect;
	for (int i = 1; i < 25; ++i) {
		// get offset in range [-0.5, 0.5]:
		float2 offset = (input.tex - 0.05) * (float(i) / float(25 - 1) - 0.5);
			// sample & add to result:
			float3 temp = { offset.r, offset.g, 0 };
			result = result + temp;
	}
	effect = effect/result;
	
	cr = effect;

	return float4(cr, 1.0f);
}