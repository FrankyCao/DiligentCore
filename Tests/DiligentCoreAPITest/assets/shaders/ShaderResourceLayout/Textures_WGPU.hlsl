Texture2D g_Tex2D_Static;
Texture2D g_Tex2D_Mut;
Texture2D g_Tex2D_Dyn;

Texture2D g_Tex2DArr_Static_0;
Texture2D g_Tex2DArr_Static_1;

Texture2D g_Tex2DArr_Mut_0;
Texture2D g_Tex2DArr_Mut_1;
Texture2D g_Tex2DArr_Mut_2;
Texture2D g_Tex2DArr_Mut_3;

Texture2D g_Tex2DArr_Dyn_0;
Texture2D g_Tex2DArr_Dyn_1;
Texture2D g_Tex2DArr_Dyn_2;

SamplerState g_Sampler;

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}

float4 VerifyResources()
{
    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);

    float2 UV = float2(0.5, 0.5);
    AllCorrect *= CheckValue(g_Tex2D_Static.SampleLevel(g_Sampler, UV.xy, 0.0), Tex2D_Static_Ref);
    AllCorrect *= CheckValue(g_Tex2D_Mut.   SampleLevel(g_Sampler, UV.xy, 0.0), Tex2D_Mut_Ref);
    AllCorrect *= CheckValue(g_Tex2D_Dyn.   SampleLevel(g_Sampler, UV.xy, 0.0), Tex2D_Dyn_Ref);

    AllCorrect *= CheckValue(g_Tex2DArr_Static_0.SampleLevel(g_Sampler, UV.xy, 0.0), Tex2DArr_Static_Ref0);
    AllCorrect *= CheckValue(g_Tex2DArr_Static_1.SampleLevel(g_Sampler, UV.xy, 0.0), Tex2DArr_Static_Ref1);

    AllCorrect *= CheckValue(g_Tex2DArr_Mut_0.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Mut_Ref0);
    AllCorrect *= CheckValue(g_Tex2DArr_Mut_1.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Mut_Ref1);
    AllCorrect *= CheckValue(g_Tex2DArr_Mut_2.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Mut_Ref2);
    AllCorrect *= CheckValue(g_Tex2DArr_Mut_3.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Mut_Ref3);

    AllCorrect *= CheckValue(g_Tex2DArr_Dyn_0.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Dyn_Ref0);
    AllCorrect *= CheckValue(g_Tex2DArr_Dyn_1.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Dyn_Ref1);
    AllCorrect *= CheckValue(g_Tex2DArr_Dyn_2.SampleLevel(g_Sampler,  UV.xy, 0.0), Tex2DArr_Dyn_Ref2);

    return AllCorrect;
}

void VSMain(in  uint    VertId    : SV_VertexID,
            out float4 f4Color    : COLOR,
            out float4 f4Position : SV_Position)
{
    float4 Pos[6];
    Pos[0] = float4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4( 0.0, -0.5, 0.0, 1.0);

    Pos[3] = float4(+0.0, -0.5, 0.0, 1.0);
    Pos[4] = float4(+0.5, +0.5, 0.0, 1.0);
    Pos[5] = float4(+1.0, -0.5, 0.0, 1.0);

    f4Color = float4(VertId % 3 == 0 ? 1.0 : 0.0,
                     VertId % 3 == 1 ? 1.0 : 0.0,
                     VertId % 3 == 2 ? 1.0 : 0.0,
                     1.0) * VerifyResources();

    f4Position = Pos[VertId];
}

float4 PSMain(in float4 f4Color    : COLOR, // Name must match VS output
              in float4 f4Position : SV_Position) : SV_Target
{
    return f4Color * VerifyResources();
}