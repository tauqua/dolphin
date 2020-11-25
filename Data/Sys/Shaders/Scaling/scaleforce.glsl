// Ported to Dolphin by iwubcode

//? #version 320 es

// from https://github.com/BreadFish64/ScaleFish/tree/master/scale_force

// MIT License
//
// Copyright (c) 2020 BreadFish64
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

[configuration]

[Pass]
Input0=PreviousPass
Input0Filter=Linear
OutputScale=2.0
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

// Finds the distance between four colors and cc in YCbCr space
float4 ColorDist(float4 A, float4 B, float4 C, float4 D, mat4x3 center_matrix, float4 center_alpha) {
	// https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.2020_conversion
	const float3 K = float3(0.2627, 0.6780, 0.0593);
	const float LUMINANCE_WEIGHT = .6;
	const mat3 YCBCR_MATRIX =
		mat3(K * LUMINANCE_WEIGHT, -.5 * K.r / (1.0 - K.b), -.5 * K.g / (1.0 - K.b), .5, .5,
			 -.5 * K.g / (1.0 - K.r), -.5 * K.b / (1.0 - K.r));

	mat4x3 colors = mat4x3(A.rgb, B.rgb, C.rgb, D.rgb) - center_matrix;
	mat4x3 YCbCr = mul(colors, YCBCR_MATRIX);
	float4 color_dist = mul(YCbCr, float3(1.0, 1.0, 1.0));
	color_dist *= color_dist;
	float4 alpha = float4(A.a, B.a, C.a, D.a);

	return sqrt((color_dist + abs(center_alpha - alpha)) * alpha * center_alpha);
}

void main() {
	float4 bl = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(-1, -1));
	float4 bc = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(0, -1));
	float4 br = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(1, -1));
	float4 cl = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(-1, 0));
	float4 cc = SampleInputLod(PREV_OUTPUT_INPUT_INDEX, 0.0);
	float4 cr = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(1, 0));
	float4 tl = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(-1, 1));
	float4 tc = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(0, 1));
	float4 tr = SampleInputLodOffset(PREV_OUTPUT_INPUT_INDEX, 0.0, int2(1, 1));

	float2 tex_size = float2(SampleInputSize(PREV_OUTPUT_INPUT_INDEX, 0));
	float2 inv_tex_size = 1.0 / tex_size;
	mat4x3 center_matrix = mat4x3(cc.rgb, cc.rgb, cc.rgb, cc.rgb);
	float4 center_alpha = cc.aaaa;

	float4 offset_tl = ColorDist(tl, tc, tr, cr, center_matrix, center_alpha);
	float4 offset_br = ColorDist(br, bc, bl, cl, center_matrix, center_alpha);

	// Calculate how different cc is from the texels around it
	float total_dist = dot(offset_tl + offset_br, float4(1.0, 1.0, 1.0, 1.0));

	if (total_dist == 0.0) {
		// Doing bicubic filtering just past the edges where the offset is 0 causes black floaters
		// and it doesn't really matter which filter is used when the colors aren't changing.
		SetOutput(cc);
	} 
	else {
		// Add together all the distances with direction taken into account
		float4 tmp = offset_tl - offset_br;
		float2 total_offset = tmp.wy + tmp.zz + float2(-tmp.x, tmp.x);

		// When the image has thin points, they tend to split apart.
		// This is because the texels all around are different
		// and total_offset reaches into clear areas.
		// This works pretty well to keep the offset in bounds for these cases.
		float clamp_val = length(total_offset) / total_dist;
		float2 final_offset = clamp(total_offset, -clamp_val, clamp_val) * inv_tex_size;

		SetOutput(textureBicubic(GetCoordinates() - final_offset, tex_size, inv_tex_size));
	}
}
