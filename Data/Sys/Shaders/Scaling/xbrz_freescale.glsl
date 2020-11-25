// Ported to Dolphin from Citra by iwubcode

//? #version 330

[configuration]

[Pass]
Input0=PreviousPass
Input0Filter=Linear
OutputScale=2
EntryPoint=main
GUIName="Scaling Pass"

[/configuration]

float ColorDist(float4 a, float4 b) {
	// https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.2020_conversion
	const float3 K = float3(0.2627, 0.6780, 0.0593);
	const mat3 MATRIX = mat3(K, -.5 * K.r / (1.0 - K.b), -.5 * K.g / (1.0 - K.b), .5, .5,
							 -.5 * K.g / (1.0 - K.r), -.5 * K.b / (1.0 - K.r));
	float4 diff = a - b;
	float3 YCbCr = mul(MATRIX, diff.rgb);
	
	const float LUMINANCE_WEIGHT = 1.0;
	// LUMINANCE_WEIGHT is currently 1, otherwise y would be multiplied by it
	float d = length(YCbCr);
	return sqrt(a.a * b.a * d * d + diff.a * diff.a);
}

bool IsPixEqual(const float4 pixA, const float4 pixB) {
	const float EQUAL_COLOR_TOLERANCE = 30.0 / 255.0;
	return ColorDist(pixA, pixB) < EQUAL_COLOR_TOLERANCE;
}

float GetLeftRatio(float2 center, float2 origin, float2 direction) {
	float2 P0 = center - origin;
	float2 proj = direction * (dot(P0, direction) / dot(direction, direction));
	float2 distv = P0 - proj;
	float2 orth = float2(-direction.y, direction.x);
	float side = sign(dot(P0, orth));
	float v = side * length(distv * output_scale);
	return smoothstep(-sqrt(2.0) / 2.0, sqrt(2.0) / 2.0, v);
}

void main() {
	float2 source_size = float2(SampleInputSize(PREV_OUTPUT_INPUT_INDEX, 0));
	float2 pos = fract(GetCoordinates() * source_size) - float2(0.5, 0.5);
	#define P(x, y) SampleInputLocationOffset(PREV_OUTPUT_INPUT_INDEX, GetCoordinates() - pos / source_size, int2(x, y))
	#define Equals(v1, v2) (v1.r == v2.r && v1.g == v2.g && v1.b == v2.b && v1.a == v2.a)
	
	const int BLEND_NONE = 0;
	const int BLEND_NORMAL = 1;
	const int BLEND_DOMINANT = 2;
	
	const float STEEP_DIRECTION_THRESHOLD = 2.2;
	const float DOMINANT_DIRECTION_THRESHOLD = 3.6;

	//---------------------------------------
	// Input Pixel Mapping:  -|x|x|x|-
	//                       x|A|B|C|x
	//                       x|D|E|F|x
	//                       x|G|H|I|x
	//                       -|x|x|x|-
	float4 A = P(-1, -1);
	float4 B = P(0, -1);
	float4 C = P(1, -1);
	float4 D = P(-1, 0);
	float4 E = P(0, 0);
	float4 F = P(1, 0);
	float4 G = P(-1, 1);
	float4 H = P(0, 1);
	float4 I = P(1, 1);
	// blendResult Mapping: x|y|
	//                      w|z|
	int4 blendResult = int4(BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE);
	// Preprocess corners
	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|-|B|C|-
	//                    -|D|E|F|x
	//                    -|G|H|I|x
	//                    -|-|x|x|-
	if (!((Equals(E, F) && Equals(H,I)) || (Equals(E, H) && Equals(F, I)))) {
		float dist_H_F = ColorDist(G, E) + ColorDist(E, C) + ColorDist(P(0, 2), I) +
						 ColorDist(I, P(2, 0)) + (4.0 * ColorDist(H, F));
		float dist_E_I = ColorDist(D, H) + ColorDist(H, P(1, 2)) + ColorDist(B, F) +
						 ColorDist(F, P(2, 1)) + (4.0 * ColorDist(E, I));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_H_F) < dist_E_I;
		blendResult.z = ((dist_H_F < dist_E_I) && !Equals(E, F) && !Equals(E, H))
							? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL)
							: BLEND_NONE;
	}
	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|A|B|-|-
	//                    x|D|E|F|-
	//                    x|G|H|I|-
	//                    -|x|x|-|-
	if (!((Equals(D, E) && Equals(G, H)) || (Equals(D, G) && Equals(E, H)))) {
		float dist_G_E = ColorDist(P(-2, 1), D) + ColorDist(D, B) + ColorDist(P(-1, 2), H) +
						 ColorDist(H, F) + (4.0 * ColorDist(G, E));
		float dist_D_H = ColorDist(P(-2, 0), G) + ColorDist(G, P(0, 2)) + ColorDist(A, E) +
						 ColorDist(E, I) + (4.0 * ColorDist(D, H));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_D_H) < dist_G_E;
		blendResult.w = ((dist_G_E > dist_D_H) && !Equals(D, E) && !Equals(E, H))
							? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL)
							: BLEND_NONE;
	}
	// Pixel Tap Mapping: -|-|x|x|-
	//                    -|A|B|C|x
	//                    -|D|E|F|x
	//                    -|-|H|I|-
	//                    -|-|-|-|-
	if (!((Equals(B, C) && Equals(E, F)) || (Equals(B, E) && Equals(C, F)))) {
		float dist_E_C = ColorDist(D, B) + ColorDist(B, P(1, -2)) + ColorDist(H, F) +
						 ColorDist(F, P(2, -1)) + (4.0 * ColorDist(E, C));
		float dist_B_F = ColorDist(A, E) + ColorDist(E, I) + ColorDist(P(0, -2), C) +
						 ColorDist(C, P(2, 0)) + (4.0 * ColorDist(B, F));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_B_F) < dist_E_C;
		blendResult.y = ((dist_E_C > dist_B_F) && !Equals(B, E) && !Equals(E, F))
							? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL)
							: BLEND_NONE;
	}
	// Pixel Tap Mapping: -|x|x|-|-
	//                    x|A|B|C|-
	//                    x|D|E|F|-
	//                    -|G|H|-|-
	//                    -|-|-|-|-
	if (!((Equals(A, B) && Equals(D, E)) || (Equals(A, D) && Equals(B, E)))) {
		float dist_D_B = ColorDist(P(-2, 0), A) + ColorDist(A, P(0, -2)) + ColorDist(G, E) +
						 ColorDist(E, C) + (4.0 * ColorDist(D, B));
		float dist_A_E = ColorDist(P(-2, -1), D) + ColorDist(D, H) + ColorDist(P(-1, -2), B) +
						 ColorDist(B, F) + (4.0 * ColorDist(A, E));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_D_B) < dist_A_E;
		blendResult.x = ((dist_D_B < dist_A_E) && !Equals(D, E) && !Equals(B, E))
							? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL)
							: BLEND_NONE;
	}
	float4 res = E;
	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|-|B|C|-
	//                    -|D|E|F|x
	//                    -|G|H|I|x
	//                    -|-|x|x|-
	if (blendResult.z != BLEND_NONE) {
		float dist_F_G = ColorDist(F, G);
		float dist_H_C = ColorDist(H, C);
		bool doLineBlend = (blendResult.z == BLEND_DOMINANT ||
							!((blendResult.y != BLEND_NONE && !IsPixEqual(E, G)) ||
							  (blendResult.w != BLEND_NONE && !IsPixEqual(E, C)) ||
							  (IsPixEqual(G, H) && IsPixEqual(H, I) && IsPixEqual(I, F) &&
							   IsPixEqual(F, C) && !IsPixEqual(E, I))));
		float2 origin = float2(0.0, 1.0 / sqrt(2.0));
		int2 direction = int2(1, -1);
		if (doLineBlend) {
			bool haveShallowLine =
				(STEEP_DIRECTION_THRESHOLD * dist_F_G <= dist_H_C) && !Equals(E, G) && !Equals(D, G);
			bool haveSteepLine =
				(STEEP_DIRECTION_THRESHOLD * dist_H_C <= dist_F_G) && !Equals(E, C) && !Equals(B, C);
			origin = haveShallowLine ? float2(0.0, 0.25) : float2(0.0, 0.5);
			direction.x += haveShallowLine ? 1 : 0;
			direction.y -= haveSteepLine ? 1 : 0;
		}
		float4 blendPix = mix(H, F, step(ColorDist(E, F), ColorDist(E, H)));
		res = mix(res, blendPix, GetLeftRatio(pos, origin, direction));
	}
	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|A|B|-|-
	//                    x|D|E|F|-
	//                    x|G|H|I|-
	//                    -|x|x|-|-
	if (blendResult.w != BLEND_NONE) {
		float dist_H_A = ColorDist(H, A);
		float dist_D_I = ColorDist(D, I);
		bool doLineBlend = (blendResult.w == BLEND_DOMINANT ||
							!((blendResult.z != BLEND_NONE && !IsPixEqual(E, A)) ||
							  (blendResult.x != BLEND_NONE && !IsPixEqual(E, I)) ||
							  (IsPixEqual(A, D) && IsPixEqual(D, G) && IsPixEqual(G, H) &&
							   IsPixEqual(H, I) && !IsPixEqual(E, G))));
		float2 origin = float2(-1.0 / sqrt(2.0), 0.0);
		int2 direction = int2(1, 1);
		if (doLineBlend) {
			bool haveShallowLine =
				(STEEP_DIRECTION_THRESHOLD * dist_H_A <= dist_D_I) && !Equals(E, A) && !Equals(B, A);
			bool haveSteepLine =
				(STEEP_DIRECTION_THRESHOLD * dist_D_I <= dist_H_A) && !Equals(E, I) && !Equals(F, I);
			origin = haveShallowLine ? float2(-0.25, 0.0) : float2(-0.5, 0.0);
			direction.y += haveShallowLine ? 1 : 0;
			direction.x += haveSteepLine ? 1 : 0;
		}
		origin = origin;
		direction = direction;
		float4 blendPix = mix(H, D, step(ColorDist(E, D), ColorDist(E, H)));
		res = mix(res, blendPix, GetLeftRatio(pos, origin, direction));
	}
	// Pixel Tap Mapping: -|-|x|x|-
	//                    -|A|B|C|x
	//                    -|D|E|F|x
	//                    -|-|H|I|-
	//                    -|-|-|-|-
	if (blendResult.y != BLEND_NONE) {
		float dist_B_I = ColorDist(B, I);
		float dist_F_A = ColorDist(F, A);
		bool doLineBlend = (blendResult.y == BLEND_DOMINANT ||
							!((blendResult.x != BLEND_NONE && !IsPixEqual(E, I)) ||
							  (blendResult.z != BLEND_NONE && !IsPixEqual(E, A)) ||
							  (IsPixEqual(I, F) && IsPixEqual(F, C) && IsPixEqual(C, B) &&
							   IsPixEqual(B, A) && !IsPixEqual(E, C))));
		float2 origin = float2(1.0 / sqrt(2.0), 0.0);
		int2 direction = int2(-1, -1);
		if (doLineBlend) {
			bool haveShallowLine =
				(STEEP_DIRECTION_THRESHOLD * dist_B_I <= dist_F_A) && !Equals(E, I) && !Equals(H, I);
			bool haveSteepLine =
				(STEEP_DIRECTION_THRESHOLD * dist_F_A <= dist_B_I) && !Equals(E, A) && !Equals(D, A);
			origin = haveShallowLine ? float2(0.25, 0.0) : float2(0.5, 0.0);
			direction.y -= haveShallowLine ? 1 : 0;
			direction.x -= haveSteepLine ? 1 : 0;
		}
		float4 blendPix = mix(F, B, step(ColorDist(E, B), ColorDist(E, F)));
		res = mix(res, blendPix, GetLeftRatio(pos, origin, direction));
	}
	// Pixel Tap Mapping: -|x|x|-|-
	//                    x|A|B|C|-
	//                    x|D|E|F|-
	//                    -|G|H|-|-
	//                    -|-|-|-|-
	if (blendResult.x != BLEND_NONE) {
		float dist_D_C = ColorDist(D, C);
		float dist_B_G = ColorDist(B, G);
		bool doLineBlend = (blendResult.x == BLEND_DOMINANT ||
							!((blendResult.w != BLEND_NONE && !IsPixEqual(E, C)) ||
							  (blendResult.y != BLEND_NONE && !IsPixEqual(E, G)) ||
							  (IsPixEqual(C, B) && IsPixEqual(B, A) && IsPixEqual(A, D) &&
							   IsPixEqual(D, G) && !IsPixEqual(E, A))));
		float2 origin = float2(0.0, -1.0 / sqrt(2.0));
		int2 direction = int2(-1, 1);
		if (doLineBlend) {
			bool haveShallowLine =
				(STEEP_DIRECTION_THRESHOLD * dist_D_C <= dist_B_G) && !Equals(E, C) && !Equals(F, C);
			bool haveSteepLine =
				(STEEP_DIRECTION_THRESHOLD * dist_B_G <= dist_D_C) && !Equals(E, G) && !Equals(H, G);
			origin = haveShallowLine ? float2(0.0, -0.25) : float2(0.0, -0.5);
			direction.x -= haveShallowLine ? 1 : 0;
			direction.y += haveSteepLine ? 1 : 0;
		}
		float4 blendPix = mix(D, B, step(ColorDist(E, B), ColorDist(E, D)));
		res = mix(res, blendPix, GetLeftRatio(pos, origin, direction));
	}
	SetOutput(res);
}
