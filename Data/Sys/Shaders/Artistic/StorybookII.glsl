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


[OptionInt]
GUIName = Background Brightness (100)
OptionName = Bright
MinValue = 0.0
MaxValue = 150.0
StepAmount = 1.0
DefaultValue = 100.0


[OptionFloat]
GUIName = Storybook Intensity (0.80)
OptionName = pow1
MinValue = 0.3
MaxValue = 2.0
StepAmount = 0.01
DefaultValue = 0.80


[OptionFloat]
GUIName = Storybook Morph (0.30)
OptionName = pow2
MinValue = 0.25
MaxValue = 1.0
StepAmount = 0.01
DefaultValue = 0.30


[OptionFloat]
GUIName = Storybook Brightness (1)
OptionName = bright1
MinValue = 0.0
MaxValue = 2.0
StepAmount = 0.01
DefaultValue = 1.1


[OptionFloat]
GUIName = Edges Darkness (0.7)
OptionName = bright2
MinValue = 0.0
MaxValue = 2.0
StepAmount = 0.01
DefaultValue = 0.7


[OptionFloat]
GUIName = Overall Brightness (.5 or 0)
OptionName = A_mix
MinValue = 0.6
MaxValue = 1.2
StepAmount = 0.01
DefaultValue = 1.0


[OptionFloat]
GUIName = Sample Distance (0.0095)
OptionName = sample
MinValue = 0.00040
MaxValue = 0.00140
StepAmount = 0.00005
DefaultValue = 0.00095


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
float dx = GetOption(sample);   // Can use GetInvResolution() for lower internal resolutions, but looks bad for x3 and up.
float dy = GetResolution().y/GetResolution().x * dx;
float3 paper = float3(0.0,0.0,0.0); //Load in the external storybook image.tga and replace this with a sampler of it.
//float rand = fract(sin(dot(GetCoordinates(), float2(12.9898,78.233))) * 43758.5453); Could combine noise with paper, but this isn't lumpy enough

void main()
{
float4 color = Sample();


float brt = GetOption(Bright) / 100.0;
paper = paper*brt;
if OptionEnabled(coloring){paper = float3(GetOption(A_red),GetOption(A_green),GetOption(A_blue));}
else
{
if (GetOption(A_Color) == 4) {paper = float3(0.85,0.85,0.85);}
if (GetOption(A_Color) == 0) {paper = float3(0.65,0.60,0.45);}
if (GetOption(A_Color) == 2) {paper = float3(.984,.835,.584);}
if (GetOption(A_Color) == 3) {paper = float3(0.97,0.86,0.72);}
if (GetOption(A_Color) == 1) {paper = float3(0.84,0.71,0.50);}
}

float3 c11 =  color.xyz;
float3 c00 = SampleLocation(GetCoordinates() + float2(-dx, -dy)).xyz;
float3 c10 = SampleLocation(GetCoordinates() + float2( 0, -dy)).xyz;
float3 c02 = SampleLocation(GetCoordinates() + float2( dx, -dy)).xyz;
float3 c01 = SampleLocation(GetCoordinates() + float2(-dx, 0)).xyz;
float3 c21 = SampleLocation(GetCoordinates() + float2( dx, 0)).xyz;
float3 c20 = SampleLocation(GetCoordinates() + float2(-dx, dy)).xyz;
float3 c12 = SampleLocation(GetCoordinates() + float2( 0, dy)).xyz;
float3 c22 = SampleLocation(GetCoordinates() + float2( dx, dy)).xyz;
float3 dt = float3(1.0,1.0,1.0); 

float d1=dot(abs(c00-c22),dt);
	float d2=dot(abs(c20-c02),dt);
	float hl=dot(abs(c01-c21),dt);
	float vl=dot(abs(c10-c12),dt);
	float d = 0.45*(d1+d2+hl+vl)/(dot(c11,dt)+0.5); 
	d =  GetOption(pow1)*pow(d,GetOption(pow2)) + d; //Intensity and morph
	
c11 = (GetOption(bright1)-(GetOption(bright2)*d))*c11; //bright1 = overall, bright2 = edge darkness/thickness
        d = pow(d,1.25-1.25*min(2.0*d,1.0));
	color.xyz = d*c11 + (GetOption(A_mix)-d)*paper; //mix strength

	SetOutput(color);
}