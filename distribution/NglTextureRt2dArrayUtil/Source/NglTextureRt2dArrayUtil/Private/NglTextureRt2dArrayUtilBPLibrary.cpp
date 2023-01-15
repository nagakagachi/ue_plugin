// Copyright Epic Games, Inc. All Rights Reserved.

#include "NglTextureRt2dArrayUtilBPLibrary.h"
#include "NglTextureRt2dArrayUtil.h"


#include "RenderingThread.h"
#include "Engine/Engine.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "TextureResource.h"
#include "SceneUtils.h"
#include "Logging/MessageLog.h"
#include "RenderGraphUtils.h"
#include "RenderUtils.h"

#include "TextureRenderTarget2DArrayResource.h"
#include "Engine/TextureRenderTarget2DArray.h"

#include "Kismet/KismetRenderingLibrary.h"



UNglTextureRt2dArrayUtilBPLibrary::UNglTextureRt2dArrayUtilBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}








// ---------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------
// フォーマット型の変換
// Engine\Source\Runtime\Engine\Classes\Engine\TextureRenderTarget2D.h
bool GetTargetFormatFromPixelFormat(const EPixelFormat PixelFormat, ETextureRenderTargetFormat& OutRTFormat)
{
	switch (PixelFormat)
	{
	case PF_G8: OutRTFormat = RTF_R8; return true;
	case PF_R8G8: OutRTFormat = RTF_RG8; return true;
	case PF_B8G8R8A8: OutRTFormat = RTF_RGBA8; return true;

	case PF_R16F: OutRTFormat = RTF_R16f; return true;
	case PF_G16R16F: OutRTFormat = RTF_RG16f; return true;
	case PF_FloatRGBA: OutRTFormat = RTF_RGBA16f; return true;

	case PF_R32_FLOAT: OutRTFormat = RTF_R32f; return true;
	case PF_G32R32F: OutRTFormat = RTF_RG32f; return true;
	case PF_A32B32G32R32F: OutRTFormat = RTF_RGBA32f; return true;
	case PF_A2B10G10R10: OutRTFormat = RTF_RGB10A2; return true;
	}
	return false;
}
// ---------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------
void UNglTextureRt2dArrayUtilBPLibrary::NglDrawMaterialToRenderTargetArraySlice(UObject* WorldContextObject, UMaterialInterface* Material, UTextureRenderTarget2DArray* TextureRt2dArray, int SliceIndex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(NglDrawMaterialToRenderTargetArraySlice);

	if (!FApp::CanEverRender())
	{
		// Returning early to avoid warnings about missing resources that are expected when CanEverRender is false.
		return;
	}

	// GraphBuilderによるマテリアル描画実行のテスト.
	if (Material && TextureRt2dArray)
	{
		if (TextureRt2dArray->Slices <= SliceIndex)
		{
			FMessageLog("Blueprint").Warning(FText::FromString("NglDrawMaterialToRenderTargetArraySlice Invalid Slice Index."));
			return;
		}


		SCOPED_DRAW_EVENTF_GAMETHREAD(NglCopyTextureToRenderTargetArray, *TextureRt2dArray->GetFName().ToString());

		auto* p_rt_array_res = TextureRt2dArray->GameThread_GetRenderTargetResource();

		// フォーマット違いやスケール違い,その他特殊な補正のためにDrawMaterialを使用.

		// ArrayのSliceに対するコピー時にフォーマットやサイズが一致するように一時テクスチャの生成.
		ETextureRenderTargetFormat temp_tex_format;
		GetTargetFormatFromPixelFormat(TextureRt2dArray->GetFormat(), temp_tex_format);
		auto temp_tex_Object = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
		check(temp_tex_Object);
		temp_tex_Object->RenderTargetFormat = temp_tex_format;;
		temp_tex_Object->ClearColor = FLinearColor::Blue;
		temp_tex_Object->InitAutoFormat(TextureRt2dArray->SizeX, TextureRt2dArray->SizeY);

		FTextureRenderTargetResource* RenderTargetResource = temp_tex_Object->GameThread_GetRenderTargetResource();

		// 一時テクスチャに対してDrawMaterial実行.
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, temp_tex_Object, Material);
		}

		// ArrayのSliceに対して一時テクスチャからコピー.
		ENQUEUE_RENDER_COMMAND(FlushDeferredResourceUpdateCommand)(
			[p_rt_array_res, RenderTargetResource, SliceIndex](FRHICommandListImmediate& RHICmdList)
			{
				FRDGBuilder rdgbuilder(RHICmdList);

				// TargetのSlice指定RDGTexture
				auto rdg_rt_array_ref = p_rt_array_res->GetRenderTargetTexture(rdgbuilder);

				// Input
				auto rdg_input_ref = RenderTargetResource->GetRenderTargetTexture(rdgbuilder);

				// 一時テクスチャから指定のSliceへコピー.
				FRHICopyTextureInfo CopyTextureInfo = {};
				CopyTextureInfo.NumMips = 1;
				CopyTextureInfo.Size = FIntVector(rdg_input_ref->Desc.GetSize().X, rdg_input_ref->Desc.GetSize().Y, 1);
				CopyTextureInfo.DestSliceIndex = SliceIndex;
				AddCopyTexturePass(rdgbuilder, rdg_input_ref, rdg_rt_array_ref, CopyTextureInfo);

				rdgbuilder.Execute();
			});

		// 破棄
		temp_tex_Object = nullptr;
	}
}


