// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessUIBlur.cpp: Post processing UIBlur implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "PostProcessUIBlur.h"
#include "SceneUtils.h"


/** Encapsulates the post processing eye adaptation pixel shader. */
class FPostProcessUIBlurPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessUIBlurPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FPostProcessUIBlurPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;

	/** Initialization constructor. */
	FPostProcessUIBlurPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
	}

	void SetPS(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(Context.RHICmdList, ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessUIBlurPS,TEXT("PostProcessUIBlur"),TEXT("MainPS"),SF_Pixel);

void FRCPassPostProcessUIBlur::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(UIBlur, DEC_SCENE_ITEMS);
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	
	FIntPoint SrcSize = InputDesc->Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / SrcSize.X;
	
	TRefCountPtr<IPooledRenderTarget>& Dest = PassInputs[0].GetOutput()->PooledRenderTarget;
	const FSceneRenderTargetItem& DestRenderTarget = Dest->GetRenderTargetItem();


	// Set the view family's render target/viewport.
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());

	// set the state
	Context.RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	Context.RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	Context.RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessUIBlurPS> PixelShader(Context.GetShaderMap());

	static FGlobalBoundShaderState BoundShaderState;
	

	SetGlobalBoundShaderState(Context.RHICmdList, Context.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetPS(Context);

	// todo: we only expect few quads but ideally we have one one draw call for all quads
	int32 RectCount = Context.View.UIBlurOverrideRectangles.Num();
	for (int32 RectIndex = 0; RectIndex < RectCount; ++RectIndex)
	{
		const FIntRect& ThisUIBlurRect = Context.View.UIBlurOverrideRectangles[RectIndex];

		FIntRect SrcRect = ThisUIBlurRect / ScaleFactor;
		FIntRect DestRect = ThisUIBlurRect;

		float UpsampleScale = Context.View.ViewRect.Width() / (float)Context.View.UnscaledViewRect.Width();
		
		SrcRect = SrcRect.Scale(UpsampleScale);
		DestRect = DestRect.Scale(UpsampleScale);

		DrawRectangle(
			Context.RHICmdList,
			DestRect.Min.X, DestRect.Min.Y,
			DestRect.Width(), DestRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y,
			SrcRect.Width(), SrcRect.Height(),
			Dest->GetDesc().Extent,
			Dest->GetDesc().Extent,
			*VertexShader,
			EDRF_Default);
	}

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
	PassOutputs[0].PooledRenderTarget = BlurOutputRef.PooledRenderTarget;
	PassOutputs[0].RenderTargetDesc = BlurOutputRef.RenderTargetDesc;
}

FPooledRenderTargetDesc FRCPassPostProcessUIBlur::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	return FPooledRenderTargetDesc();
}