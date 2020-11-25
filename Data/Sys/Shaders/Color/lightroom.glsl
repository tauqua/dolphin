// consider
// https://github.com/Asmodean-/dolphin/blob/master/Data/Sys/Shaders/DolphinFX.glsl
// http://www.workwithcolor.com/pink-red-color-hue-range-01.htm
// https://stackoverflow.com/questions/28283454/finding-dark-purple-pixels-in-an-image
// https://www.chilliant.com/rgb2hsv.html

// https://halisavakis.com/my-take-on-shaders-color-grading-with-look-up-textures-lut/
// https://abload.de/image.php?img=gta_sa2017-06-0120-14gaufi.png
// https://alaingalvan.tumblr.com/post/79864187609/glsl-color-correction-shaders
// https://defold.com/tutorials/grading/
// https://digital-photography-school.com/unlocking-power-basic-panel-lightroom/
// https://stackoverflow.com/questions/11577136/how-selective-color-and-color-balance-filter-of-photoshop-work
// https://gitlab.bestminr.com/bestminr/FrontShaders/-/blob/e6827d2de736c6509ac1e23fccb5e4f6385b72da/shaders/saturation_vibrance.glsl

[configuration]

[Meta]
Description = A shader that emulates lightroom like controls
Author = iwubcode

[OptionFloat]
GUIName = Exposure
OptionName = _GlobalExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = Global

[OptionFloat]
GUIName = Saturation
OptionName = _GlobalSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = Global

[OptionFloat]
GUIName = Highlights
OptionName = _GlobalHighlights
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = Global

[OptionFloat]
GUIName = Shadows
OptionName = _GlobalShadows
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = Global

[OptionFloat]
GUIName = Midtones
OptionName = _GlobalMidtones
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = Global

[OptionFloat]
GUIName = Gamma
OptionName = _GlobalGamma
MinValue = 0.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 1.0
Group = Global

[OptionFloat]
GUIName = Vibrance
OptionName = _GlobalVibrance
MinValue = 0.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = Global

[OptionFloat]
GUIName = RedHue
OptionName = _RedHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = RedSaturation
OptionName = _RedSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = RedExposure
OptionName = _RedExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = OrangeHue
OptionName = _OrangeHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = OrangeSaturation
OptionName = _OrangeSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = OrangeExposure
OptionName = _OrangeExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = YellowHue
OptionName = _YellowHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = YellowSaturation
OptionName = _YellowSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = YellowExposure
OptionName = _YellowExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = GreenHue
OptionName = _GreenHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = GreenSaturation
OptionName = _GreenSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = GreenExposure
OptionName = _GreenExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = CyanHue
OptionName = _CyanHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = CyanSaturation
OptionName = _CyanSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = CyanExposure
OptionName = _CyanExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = BlueHue
OptionName = _BlueHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = BlueSaturation
OptionName = _BlueSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = BlueExposure
OptionName = _BlueExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = PurpleHue
OptionName = _PurpleHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = PurpleSaturation
OptionName = _PurpleSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = PurpleExposure
OptionName = _PurpleExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = MagentaHue
OptionName = _MagentaHue
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = MagentaSaturation
OptionName = _MagentaSaturation
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[OptionFloat]
GUIName = MagentaExposure
OptionName = _MagentaExposure
MinValue = -1.0
MaxValue = 1.0
StepAmount = 0.1
DefaultValue = 0.0
Group = HSL

[/configuration]

float3 rgb_to_hsl(float3 color)
{
	float3 hsl;

	float fmin = min(min(color.r, color.g), color.b);	  //Min. value of RGB
	float fmax = max(max(color.r, color.g), color.b);	  //Max. value of RGB
	float delta = fmax - fmin; //Delta RGB value

	hsl.z = (fmax + fmin) / 2.0; // Luminance

	if (delta == 0.0) //This is a gray, no chroma...
	{
		hsl.x = 0.0;
		hsl.y = 0.0;
	}
	else //Chromatic data...
	{
		if (hsl.z < 0.5)
			hsl.y = delta / (fmax + fmin); // Saturation
		else
			hsl.y = delta / (2.0 - fmax - fmin); // Saturation
		
		float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;
		float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;
		float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;
		
		if (color.r == fmax )
			hsl.x = deltaB - deltaG; // Hue
		else if (color.g == fmax)
			hsl.x = (1.0 / 3.0) + deltaR - deltaB; // Hue
		else if (color.b == fmax)
			hsl.x = (2.0 / 3.0) + deltaG - deltaR; // Hue
		
		if (hsl.x < 0.0)
			hsl.x += 1.0; // Hue
		else if (hsl.x > 1.0)
			hsl.x -= 1.0; // Hue
	}

	return hsl;
}

// These two functions pulled from here https://github.com/Jam3/glsl-hsl2rgb
// The MIT License (MIT) Copyright (c) 2015 Jam3

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

float hue_to_rgb(float f1, float f2, float hue)
{
	if (hue < 0.0)
		hue += 1.0;
	else if (hue > 1.0)
		hue -= 1.0;
	float res;
	if ((6.0 * hue) < 1.0)
		res = f1 + (f2 - f1) * 6.0 * hue;
	else if ((2.0 * hue) < 1.0)
		res = f2;
	else if ((3.0 * hue) < 2.0)
		res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
	else
		res = f1;
	return res;
}

float3 hsl_to_rgb(float3 hsl)
{
	float3 rgb;

	if (hsl.y == 0.0)
		rgb = float3(hsl.z, hsl.z, hsl.z); // Luminance
	else
	{
		float f2;
		
		if (hsl.z < 0.5)
			f2 = hsl.z * (1.0 + hsl.y);
		else
			f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);
		
		float f1 = 2.0 * hsl.z - f2;
		
		rgb.r = hue_to_rgb(f1, f2, hsl.x + (1.0/3.0));
		rgb.g = hue_to_rgb(f1, f2, hsl.x);
		rgb.b = hue_to_rgb(f1, f2, hsl.x - (1.0/3.0));
	}

	return rgb;
}

// pulled from https://github.com/liovch/GPUImage/blob/master/framework/Source/GPUImageColorBalanceFilter.m
// Copyright (c) 2012, Brad Larson, Ben Cochran, Hugues Lismonde, Keitaroh Kobayashi, Alaric Cole, Matthew Clark, Jacob Gundersen, Chris Williams.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// Neither the name of the GPUImage framework nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
float3 rgb_apply_midtones_highlights_shadows(float3 rgb_color, float lightness, float midtones, float highlights, float shadows)
{
	float a = 0.25;
	float b = 0.333;
	float scale = 0.7;
	
	float3 midtones_shift = float3(midtones, midtones, midtones);
	float3 shadows_shift = float3(shadows, shadows, shadows);
	float3 highlights_shift = float3(highlights, highlights, highlights);
	
	float3 shadows_contribution = shadows_shift * (clamp((lightness - b) / -a + 0.5, 0.0, 1.0) * scale);
	float3 midtones_contribution = midtones_shift * (clamp((lightness - b) / a + 0.5, 0.0, 1.0) *  clamp((lightness + b - 1.0) / -a + 0.5, 0.0, 1.0) * scale);
	float3 highlights_contribution =  highlights_shift * (clamp((lightness + b - 1.0) / a + 0.5, 0.0, 1.0) * scale);

	float3 newColor = rgb_color + shadows_contribution + midtones_contribution + highlights_contribution;
	return clamp(newColor, 0.0, 1.0);
}

float rgb_to_luminance(float3 rgb_color)
{
	float min_val = min(min(rgb_color.r, rgb_color.g), rgb_color.b);
	float max_val = max(max(rgb_color.r, rgb_color.g), rgb_color.b);
	
	return (max_val + min_val) / 2.0;
}

float3 rgb_apply_gama(float3 rgb_color, float param)
{
	return float3(pow(abs(rgb_color.r), param), pow(abs(rgb_color.g), param), pow(abs(rgb_color.b), param));
}

// pulled from https://github.com/evanw/glfx.js/blob/master/src/filters/adjust/vibrance.js
// Copyright (C) 2011 by Evan Wallace

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
float3 rgb_apply_vibrance(float3 rgb_color, float param)
{
	float max_color_val = max(rgb_color.r, max(rgb_color.g, rgb_color.b));
	float vibrance_intensity  = (max_color_val - rgb_to_luminance(rgb_color)) * (param * -3.0);
	float3 newColor = mix(rgb_color, float3(max_color_val, max_color_val, max_color_val), vibrance_intensity);
	return clamp(newColor, 0.0, 1.0);
}

void main()
{
	float3 color = Sample().rgb;
	float3 original_hsl = rgb_to_hsl(color);
	float lightness = original_hsl.z;
	
	// Shadows, Midtones, Highlights...
	float3 newColor = rgb_apply_midtones_highlights_shadows(color, lightness, GetOption(_GlobalMidtones), GetOption(_GlobalHighlights), GetOption(_GlobalShadows));
	
	// Gamma...
	newColor = rgb_apply_gama(newColor, GetOption(_GlobalGamma));
	
	// Vibrance...
	newColor = rgb_apply_vibrance(newColor, GetOption(_GlobalVibrance));
	
	// todo: clarity
	// todo: temperature
	// todo: tint
	// todo: white / black balance
	
	float3 hsl = rgb_to_hsl(newColor);
	
	// Saturation / Exposure
	hsl.y = clamp(hsl.y + GetOption(_GlobalSaturation), 0.0, 1.0);
	hsl.z = clamp(hsl.z + GetOption(_GlobalExposure), 0.0, 1.0);

	// Scale original hsl based on hue
	// The values here are modified from a 360 degree value to
	// a value that is in the range 0 to 1
	// Values come from the wheel at http://www.workwithcolor.com/yellow-green-color-hue-range-01.htm
	// and also tweaked with https://www.december.com/html/spec/colorhsltable10.html
	if (original_hsl.x >= 0.96 || original_hsl.x < 0.058)
	{
		hsl.x += original_hsl.x * GetOption(_RedHue);
		hsl.y += original_hsl.y * GetOption(_RedSaturation);
		hsl.z += original_hsl.z * GetOption(_RedExposure);
	}
	else if (original_hsl.x >= 0.058 && original_hsl.x < 0.138)
	{
		hsl.x += original_hsl.x * GetOption(_OrangeHue);
		hsl.y += original_hsl.y * GetOption(_OrangeSaturation);
		hsl.z += original_hsl.z * GetOption(_OrangeExposure);
	}
	else if (original_hsl.x >= 0.138 && original_hsl.x < 0.197)
	{
		hsl.x += original_hsl.x * GetOption(_YellowHue);
		hsl.y += original_hsl.y * GetOption(_YellowSaturation);
		hsl.z += original_hsl.z * GetOption(_YellowExposure);
	}
	else if(original_hsl.x >= 0.197 && original_hsl.x < 0.388)
	{
		hsl.x += original_hsl.x * GetOption(_GreenHue);
		hsl.y += original_hsl.y * GetOption(_GreenSaturation);
		hsl.z += original_hsl.z * GetOption(_GreenExposure);
	}
	else if (original_hsl.x >= 0.388 && original_hsl.x < 0.555)
	{
		hsl.x += hsl.x * GetOption(_CyanHue);
		hsl.y += hsl.y * GetOption(_CyanSaturation);
		hsl.z += hsl.z * GetOption(_CyanExposure);
	}
	else if(original_hsl.x >= 0.555 && original_hsl.x < 0.66)
	{
		hsl.x += original_hsl.x * GetOption(_BlueHue);
		hsl.y += original_hsl.y * GetOption(_BlueSaturation);
		hsl.z += original_hsl.z * GetOption(_BlueExposure);
	}
	else if(original_hsl.x >= 0.66 && original_hsl.x > 0.77)
	{
		hsl.x += original_hsl.x * GetOption(_PurpleHue);
		hsl.y += original_hsl.y * GetOption(_PurpleSaturation);
		hsl.z += original_hsl.z * GetOption(_PurpleExposure);
	}
	else
	{
		hsl.x += original_hsl.x * GetOption(_MagentaHue);
		hsl.y += original_hsl.y * GetOption(_MagentaSaturation);
		hsl.z += original_hsl.z * GetOption(_MagentaExposure);
	}
	
	SetOutput(float4(hsl_to_rgb(hsl), 1.0));
}
