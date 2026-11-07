#include "DeltaLens.h"
#include "ScreenRendering.h"
#include "PostProcess/SceneFilterRendering.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "RHIResourceUtils.h"

BEGIN_SHADER_PARAMETER_STRUCT(FDeltaLensPass,)
 RDG_TEXTURE_ACCESS(Source,ERHIAccess::SRVGraphics)
 RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

// Tessellated inverse-radial mesh uses Unreal's built-in cooked screen shaders.
// This keeps Android-only plugins independent of editor-loaded custom global shaders.
void DeltaDrawLens(FRDGBuilder& GraphBuilder,FRDGTextureRef Back,FRDGTextureRef Source,FDeltaLensSettings Settings)
{
 auto* P=GraphBuilder.AllocParameters<FDeltaLensPass>();
 P->Source=Source;P->RenderTargets[0]=FRenderTargetBinding(Back,ERenderTargetLoadAction::EClear);
 const FIntPoint Extent=Back->Desc.Extent;
 GraphBuilder.AddPass(RDG_EVENT_NAME("DeltaViewerLensCorrection"),P,ERDGPassFlags::Raster,[P,Extent,Settings](FRHICommandListImmediate& Cmd)
 {
  constexpr int32 Grid=48;
  static FDeltaLensSettings Cached;
  static bool Built=false;
  static FBufferRHIRef VertexBuffers[2],IndexBuffer;
  if(!Built||Cached.Size!=Settings.Size||Cached.Inset!=Settings.Inset||Cached.Curve!=Settings.Curve||Cached.EyeOffset[0]!=Settings.EyeOffset[0]||Cached.EyeOffset[1]!=Settings.EyeOffset[1])
  {
   TArray<uint16> Indices;
   for(int32 Y=0;Y<Grid;++Y)for(int32 X=0;X<Grid;++X)
   {uint16 A=Y*(Grid+1)+X,B=A+1,C=A+Grid+1,D=C+1;Indices.Append({A,C,B,B,C,D});}
   IndexBuffer=UE::RHIResourceUtils::CreateIndexBufferFromArray(Cmd,TEXT("DeltaLensIndices"),EBufferUsageFlags::Static,MakeConstArrayView(Indices));
   for(int32 Eye=0;Eye<2;++Eye)
   {
    TArray<FScreenVertex> Vertices;
    for(int32 Y=0;Y<=Grid;++Y)for(int32 X=0;X<=Grid;++X)
    {
     const FVector2f UV(float(X)/Grid,float(Y)/Grid),S=UV*2-FVector2f(1,1);
     const float Radius=S.Size();float R=Radius;
     for(int32 I=0;I<8;++I){float R2=R*R,R4=R2*R2;R-=(R*(1+Settings.Curve*R2+Settings.Curve*0.2f*R4)-Radius)/(1+3*Settings.Curve*R2+Settings.Curve*R4);}
     FVector2f Out=S*(Radius>0.0001f?R/Radius:1.f)*Settings.Size*0.5f;
     Out+=FVector2f(0.5f+(Eye==0?1:-1)*Settings.Inset,0.5f)+Settings.EyeOffset[Eye];
     Vertices.Add({FVector2f((Eye+Out.X)*0.5f,Out.Y),FVector2f((Eye+UV.X)*0.5f,UV.Y)});
    }
    VertexBuffers[Eye]=UE::RHIResourceUtils::CreateVertexBufferFromArray(Cmd,TEXT("DeltaLensVertices"),EBufferUsageFlags::Static,MakeConstArrayView(Vertices));
   }
   Cached=Settings;Built=true;
  }
  FGraphicsPipelineStateInitializer PSO;Cmd.ApplyCachedRenderTargets(PSO);
  PSO.BlendState=TStaticBlendState<>::GetRHI();PSO.RasterizerState=TStaticRasterizerState<FM_Solid,CM_None>::GetRHI();PSO.DepthStencilState=TStaticDepthStencilState<false,CF_Always>::GetRHI();PSO.PrimitiveType=PT_TriangleList;
  TShaderMapRef<FScreenVS> VS(GetGlobalShaderMap(GMaxRHIShaderPlatform));TShaderMapRef<FScreenPS> PS(GetGlobalShaderMap(GMaxRHIShaderPlatform));
  PSO.BoundShaderState.VertexDeclarationRHI=GScreenVertexDeclaration.VertexDeclarationRHI;PSO.BoundShaderState.VertexShaderRHI=VS.GetVertexShader();PSO.BoundShaderState.PixelShaderRHI=PS.GetPixelShader();SetGraphicsPipelineState(Cmd,PSO,0);
  FDrawRectangleParameters Draw;Draw.PosScaleBias=FVector4f(1,1,0,0);Draw.UVScaleBias=FVector4f(1,1,0,0);Draw.InvTargetSizeAndTextureSize=FVector4f(1,1,1,1);
  auto& VSParameters=Cmd.GetScratchShaderParameters();SetUniformBufferParameterImmediate(VSParameters,VS->GetUniformBufferParameter<FDrawRectangleParameters>(),Draw);Cmd.SetBatchedShaderParameters(VS.GetVertexShader(),VSParameters);
  SetShaderParametersLegacyPS(Cmd,PS,TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),P->Source.GetTexture()->GetRHI());
  Cmd.SetViewport(0,0,0,Extent.X,Extent.Y,1);
  for(int32 Eye=0;Eye<2;++Eye){Cmd.SetScissorRect(true,Eye*Extent.X/2,0,(Eye+1)*Extent.X/2,Extent.Y);Cmd.SetStreamSource(0,VertexBuffers[Eye],0);Cmd.DrawIndexedPrimitive(IndexBuffer,0,0,(Grid+1)*(Grid+1),0,Grid*Grid*2,1);}
  Cmd.SetScissorRect(false,0,0,0,0);
 });
}
