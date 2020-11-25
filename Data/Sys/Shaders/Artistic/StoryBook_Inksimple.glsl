/*
Ported to Dolphin by One More Try
// ColoredStorybook shader
// by guest(r)
// License: GNU-GPL
*/

/*
[configuration]

[OptionRangeInteger]
GUIName = Color
OptionName = A_Color
MinValue = 0
MaxValue = 4
StepAmount = 1
DefaultValue = 3


[OptionRangeInteger]
GUIName = Brightness (100)
OptionName = brightness
MinValue = 0.0
MaxValue = 150.0
StepAmount = 1.0
DefaultValue = 100.0

[OptionBool]
GUIName = BGcolor
OptionName = coloring
DefaultValue = false

[OptionRangeFloat]
GUIName = Red (0.97)
OptionName = A_red
MinValue = 0.0
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.97
DependentOption = coloring

[OptionRangeFloat]
GUIName = Green (0.86)
OptionName = A_green
MinValue = 0.0
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.86
DependentOption = coloring

[OptionRangeFloat]
GUIName = Blue (0.72)
OptionName = A_blue
MinValue = 0
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.72
DependentOption = coloring

[/configuration]
*/
float dx = 0.00095;   // Can use GetInvResolution() for lower internal resolutions, but looks bad for x3 and up.
float dy = 0.00078;
float3 paper = float3(0.0,0.0,0.0); //Load in the external storybook image.tga and replace this with a sampler of it.
//float rand = fract(sin(dot(GetCoordinates(), float2(12.9898,78.233))) * 43758.5453); Could combine noise with paper, but this isn't lumpy enough
#define sp(a, b, c) float3 a = SampleLocation(GetCoordinates()+float2(dx*b,dy*c)).xyz;

void main()
{
float4 color = Sample();

if OptionEnabled(coloring){paper = float3(GetOption(A_red),GetOption(A_green),GetOption(A_blue));}
else
{
if (GetOption(A_Color) == 4) {paper = float3(0.85,0.85,0.85);}
if (GetOption(A_Color) == 0) {paper = float3(0.65,0.60,0.45);}
if (GetOption(A_Color) == 2) {paper = float3(.984,.835,.584);}
if (GetOption(A_Color) == 3) {paper = float3(0.97,0.86,0.72);}
if (GetOption(A_Color) == 1) {paper = float3(0.84,0.71,0.50);}
}

float3 ink  = float3(0.14,0.11,0.1);
float brt = GetOption(brightness) / 100.0;
paper = paper*brt;

float3 ct = color.xyz;
sp(uy, 0, -1) sp(lx,0,1) sp(rx, -1, -1) sp(dy,0,1) 

float3 dt = float3(1.0,1.0,1.0); 
float  d = dot(abs(lx-rx),dt)/(dot(lx+rx,dt)+0.15);
 d+= dot(abs(uy-dy),dt)/(dot(uy+dy,dt)+0.15);
	
color.xyz = mix(paper, ink, max(0.0, 4.0*d-0.15));

	SetOutput(color);
}