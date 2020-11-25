/*
[configuration]
[Pass]
Input0 = ColorBuffer
Input0Mode = Clamp
Input0Filter = Nearest
EntryPoint = main
OutputScale=2.0
GUIName="Scaling Pass"
[/configuration]
*/

void main()
{
	SetOutput(Sample());
}