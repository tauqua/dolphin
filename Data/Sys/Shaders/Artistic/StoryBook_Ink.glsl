/*
Ported to Dolphin by One More Try
// ColoredStorybook shader
// by guest(r)
// License: GNU-GPL
*/

/*
[configuration]

[OptionInt]
GUIName = Color
OptionName = A_Color
MinValue = 0
MaxValue = 4
StepAmount = 1
DefaultValue = 3

[OptionFloat]
GUIName = Brightness (100)
OptionName = brightness
MinValue = 0.0
MaxValue = 150.0
StepAmount = 1.0
DefaultValue = 100.0

[OptionFloat]
GUIName = Storybook Intensity (0.80)
OptionName = pow1
MinValue = -0.10
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.07

[OptionFloat]
GUIName = Storybook Morph (0.30)
OptionName = pow2
MinValue = 0.25
MaxValue = 2.0
StepAmount = 0.01
DefaultValue = 1.80

[OptionFloat]
GUIName = Edges Darkness (0.7)
OptionName = bright2
MinValue = 0.25
MaxValue = 2.0
StepAmount = 0.01
DefaultValue = 1.25

[OptionBool]
GUIName = BGcolor
OptionName = coloring
DefaultValue = false

[OptionFloat]
GUIName = Red (0.97)
OptionName = A_red
MinValue = 0.0
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.97
DependentOption = coloring

[OptionFloat]
GUIName = Green (0.86)
OptionName = A_green
MinValue = 0.0
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.86
DependentOption = coloring

[OptionFloat]
GUIName = Blue (0.72)
OptionName = A_blue
MinValue = 0
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.72
DependentOption = coloring

[/configuration]
*/

// Can use GetInvResolution() for lower internal resolutions, but looks bad for x3 and up.
#define dx float(0.00095)
#define dy float(0.00078)
//float rand = fract(sin(dot(GetCoordinates(), float2(12.9898,78.233))) * 43758.5453); Could combine noise with paper, but this isn't lumpy enough
#define sp(a, b, c) float3 a = SampleLocation(GetCoordinates()+float2(dx*b,dy*c)).xyz;

void main()
{
	float3 paper = float3(0.0,0.0,0.0); //Load in the external storybook image.tga and replace this with a sampler of it.
	
	float4 color = Sample();
	if OptionEnabled(coloring)
	{
		paper = float3(GetOption(A_red),GetOption(A_green),GetOption(A_blue));
	}
	else
	{
		if (GetOption(A_Color) == 4) {paper = float3(0.85,0.85,0.85);}
		if (GetOption(A_Color) == 0) {paper = float3(0.65,0.60,0.45);}
		if (GetOption(A_Color) == 2) {paper = float3(.984,.835,.584);}
		if (GetOption(A_Color) == 3) {paper = float3(0.97,0.86,0.72);}
		if (GetOption(A_Color) == 1) {paper = float3(0.84,0.71,0.50);}
	}

	float3 ink  = float3(0.375,0.365,0.365);
	float brt = GetOption(brightness) / 100.0;
	paper = paper*brt;

	sp(up_y, 0, -1)
	sp(lx,0,1)
	sp(rx, -1, -1)
	sp(down_y,0,1)

	float3 dt = float3(1.0,1.0,1.0); 
	float d = dot(abs(lx-rx),dt)/(dot(lx+rx,dt)+0.25);
	d += dot(abs(up_y-down_y),dt)/(dot(up_y+down_y,dt)+0.25);
	d = GetOption(pow2)*pow(max(d-GetOption(pow1),0.0),GetOption(bright2));
	
	if (d == 0.0)
	{
		color.xyz = paper;
	}
	else
	{
		color.xyz = (1.0-d)*ink;
	}

	SetOutput(color);
}