
[configuration]

[Pass]
Input0=PreviousPass
Input0Filter=Linear
OutputScale=0.5
EntryPoint=main
GUIName="Scaling Pass"

[/configuration]

float4 cubic(float v) {
	float3 n = float3(1.0, 2.0, 3.0) - v;
	float3 s = n * n * n;
	float x = s.x;
	float y = s.y - 4.0 * s.x;
	float z = s.z - 4.0 * s.y + 6.0 * s.x;
	float w = 6.0 - x - y - z;
	return float4(x, y, z, w) / 6.0;
}

// Bicubic interpolation
float4 textureBicubic(float2 tex_coords, float2 tex_size, float2 inv_tex_size) {
	tex_coords = tex_coords * tex_size - 0.5;

	float2 fxy = modf(tex_coords, tex_coords);

	float4 xcubic = cubic(fxy.x);
	float4 ycubic = cubic(fxy.y);

	float4 c = tex_coords.xxyy + float2(-0.5, +1.5).xyxy;

	float4 s = float4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
	float4 offset = c + float4(xcubic.yw, ycubic.yw) / s;

	offset *= inv_tex_size.xxyy;

	float4 sample0 = SampleInputLodLocation(PREV_OUTPUT_INPUT_INDEX, 0.0, offset.xz);
	float4 sample1 = SampleInputLodLocation(PREV_OUTPUT_INPUT_INDEX, 0.0, offset.yz);
	float4 sample2 = SampleInputLodLocation(PREV_OUTPUT_INPUT_INDEX, 0.0, offset.xw);
	float4 sample3 = SampleInputLodLocation(PREV_OUTPUT_INPUT_INDEX, 0.0, offset.yw);

	float sx = s.x / (s.x + s.y);
	float sy = s.z / (s.z + s.w);

	return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

void main() {
	float2 tex_size = float2(SampleInputSize(PREV_OUTPUT_INPUT_INDEX, 0));
	float2 inv_tex_size = 1.0 / tex_size;
	
	SetOutput(textureBicubic(GetCoordinates(), tex_size, inv_tex_size));
}
