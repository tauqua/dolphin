/*
Ported to Dolphin by One More Try
// ColoredStorybook shader
// by guest(r)
// License: GNU-GPL
*/

/*
[configuration]

[OptionInt]
GUIName = Color Samples
OptionName = A_ColorChoice
MinValue = 0
MaxValue = 4
StepAmount = 1
DefaultValue = 3

[OptionInt]
GUIName = Brightness (100)
OptionName = Bright
MinValue = 0
MaxValue = 150
StepAmount = 1
DefaultValue = 100


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
DefaultValue = 1.0


[OptionFloat]
GUIName = Edges Darkness (0.7)
OptionName = bright2
MinValue = 0.0
MaxValue = 2.0
StepAmount = 0.01
DefaultValue = 0.7


[OptionFloat]
GUIName = Colormap Mix Strength (.5 or 0)
OptionName = A_mix
MinValue = 0.0
MaxValue = 0.7
StepAmount = 0.01
DefaultValue = 0.5


[OptionFloat]
GUIName = Sample Distance (0.0095)
OptionName = sam_dist
MinValue = 0.00040
MaxValue = 0.00140
StepAmount = 0.00005
DefaultValue = 0.00095

[OptionBool]
GUIName = BGcolor
OptionName = coloring
DefaultValue = false

[OptionRGB]
GUIName = Color (0.97, 0.86, 0.72)
OptionName = A_Color
DefaultValue = 0.97, 0.86, 0.72
DependentOption = coloring
[/configuration]
*/

void main()
{
	float dx = GetOption(sam_dist);   // Can use GetInvResolution() for lower internal resolutions, but looks bad for x3 and up.
	float dy = GetResolution().y/GetResolution().x * dx;
	float3 paper = float3(0.0,0.0,0.0); //Load in the external storybook image.tga and replace this with a sampler of it.
	float4 color = Sample();

	if OptionEnabled(coloring){paper = GetOption(A_Color);}
	else
	{
		if (GetOption(A_ColorChoice) == 4) {paper = float3(0.85,0.85,0.85);}
		if (GetOption(A_ColorChoice) == 0) {paper = float3(0.65,0.60,0.45);}
		if (GetOption(A_ColorChoice) == 2) {paper = float3(.984,.835,.584);}
		if (GetOption(A_ColorChoice) == 3) {paper = float3(0.97,0.86,0.72);}
		if (GetOption(A_ColorChoice) == 1) {paper = float3(0.84,0.71,0.50);}
	}

	float brt = GetOption(Bright) / 100.0;
	paper = paper*brt;

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
	float d = (d1+d2+hl+vl)/(dot(c11+c10+c02+c22,dt)+0.2);
	d =  GetOption(pow1)*pow(d,GetOption(pow2)) + d; //Intensity and morph
	
	c11 = (GetOption(bright1)-(GetOption(bright2)*d))*c11; //bright1 = overall, bright2 = edge darkness/thickness

	color.xyz = mix(paper, c11, pow(max(min(d,1.1)-0.1,0.0),GetOption(A_mix))); //mix strength

	SetOutput(color);
}