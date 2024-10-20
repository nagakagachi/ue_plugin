
#include "Rendering/ViewExtensionSampleVe.h"

#include "RHIStaticStates.h"

#include "DynamicResolutionState.h"
#include "FXRenderingUtils.h"
#include "PostProcess/PostProcessInputs.h"
#include "Rendering/ViewExtensionSampleShader.h"
#include "Subsystem/ViewExtensionSampleSubsystem.h"

#include "Rendering/NagaVoronoiJfaCompute.h"

#include "PostProcess/PostProcessMaterialInputs.h"

FViewExtensionSampleVe::FViewExtensionSampleVe(const FAutoRegister& AutoRegister, UViewExtensionSampleSubsystem* InWorldSubsystem)
	: FSceneViewExtensionBase(AutoRegister), subsystem(InWorldSubsystem)
{
	
}

void FViewExtensionSampleVe::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	// 特になし.
}
void FViewExtensionSampleVe::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	// 特になし.
}
void FViewExtensionSampleVe::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	// 特になし.
}

template<typename CS_CLASS>
FIntVector CalcComputeDispatchGroupSizeHelper(const TShaderMapRef<CS_CLASS>& cs, const FUintVector3 target_size)
{
	const uint32_t gsx = FMath::Max(1u, FMath::DivideAndRoundUp(target_size.X, CS_CLASS::THREADGROUPSIZE_X));
	const uint32_t gsy = FMath::Max(1u, FMath::DivideAndRoundUp(target_size.Y, CS_CLASS::THREADGROUPSIZE_Y));
	const uint32_t gsz = FMath::Max(1u, FMath::DivideAndRoundUp(target_size.Z, CS_CLASS::THREADGROUPSIZE_Z));
	return FIntVector(gsx, gsy, gsz);
}
template<typename CS_CLASS>
constexpr FIntVector CalcComputeDispatchGroupSizeHelper(const TShaderMapRef<CS_CLASS>& cs, const FUintVector2 target_size)
{
	return CalcComputeDispatchGroupSizeHelper(cs, FUintVector3(target_size.X, target_size.Y, 1));
}


/**
 * Called right after Base Pass rendering finished when using the deferred renderer.
 */
/*
BEGIN_SHADER_PARAMETER_STRUCT(FViewExtensionSamplePreBasePassParameters, )
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()
*/

#define NAGA_SHADINGMODELID_MASK					0xF		// 4 bits reserved for ShadingModelID
// DeferredShadingCommon.ush
float Naga_EncodeShadingModelIdAndSelectiveOutputMask(uint32 ShadingModelId, uint32 SelectiveOutputMask)
{
	uint32 Value = (ShadingModelId & NAGA_SHADINGMODELID_MASK) | SelectiveOutputMask;
	return (float)Value / (float)0xFF;
}
void FViewExtensionSampleVe::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	if (!IsValid(subsystem))
	{
		return;
	}

	FScreenPassTexture ScreenPassTex(SceneTextures->GetParameters()->GBufferATexture);
	RDG_EVENT_SCOPE(GraphBuilder, "Naga_PostRenderBasePassDeferred %dx%d", ScreenPassTex.ViewRect.Width(), ScreenPassTex.ViewRect.Height());
	
	/*
	GBUfferレイアウト -> DeferredShadingCommon.ush EncodeGBuffer()
	ShadingModelID -> ShadingCommon.ush


	OutGBufferA.rgb = EncodeNormal( GBuffer.WorldNormal );
	OutGBufferA.a = GBuffer.PerObjectGBufferData;
	
	OutGBufferB.r = GBuffer.Metallic;
	OutGBufferB.g = GBuffer.Specular;
	OutGBufferB.b = GBuffer.Roughness;
	OutGBufferB.a = EncodeShadingModelIdAndSelectiveOutputMask(GBuffer.ShadingModelID, GBuffer.SelectiveOutputMask);

	OutGBufferC.rgb = EncodeBaseColor( GBuffer.BaseColor );
	OutGBufferC.a = GBuffer.GBufferAO;
	
	OutGBufferD = GBuffer.CustomData;
	OutGBufferE = GBuffer.PrecomputedShadowFactors;
	*/

	/*
		RenderTargets の内訳.
		RenderTargets[0] : SceneColor
		RenderTargets[1] : GBUfferA
		RenderTargets[2] : GBUfferB
		RenderTargets[3] : GBUfferC
		RenderTargets[4] : GBUfferD
		RenderTargets[5] : GBUfferE
		GBufferB.a のShadingModelIDは Naga_EncodeShadingModelIdAndSelectiveOutputMask(1, 0)) で生成する.	
	*/

	constexpr int RT_SceneColor = 0;
	constexpr int RT_GB_Normal = 1;
	constexpr int RT_GB_PbrShadingModel = 2;
	constexpr int RT_GB_BcAo = 3;
	constexpr int RT_GB_CustomData = 4;
	constexpr int RT_GB_PreShadow = 5;
	
	static float tmp_counter = 0.0f;
	tmp_counter += 1.0f / 60.0f;
	if(65535.0f < tmp_counter) tmp_counter = 0.0f;
	
	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	
	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(InView);
	
	// GBuffer操作.
	if(subsystem->enable_worldnormal_unlit)
	{
		// PostBasePassではSceneTexturesのGBuffer参照は空なのでRenderTargets側のGBufferを利用する.
		FRDGTextureRef gb_tex_pbr_shadingmodel = RenderTargets[RT_GB_PbrShadingModel].GetTexture();
		FRDGTextureRef gb_tex_bc_ao = RenderTargets[RT_GB_BcAo].GetTexture();

		// Create WorkBuffer.
		FRDGTextureRef new_gbuffer_tex_pbr_shadingmodel = GraphBuilder.CreateTexture(
			FRDGTextureDesc::Create2D(
				gb_tex_pbr_shadingmodel->Desc.Extent ,
				gb_tex_pbr_shadingmodel->Desc.Format,
				{},
				ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable
			), TEXT("Naga_GBuffer_C_Work"));
		
		FRDGTextureRef new_gbuffer_tex_bc_ao = GraphBuilder.CreateTexture(
			FRDGTextureDesc::Create2D(
				gb_tex_bc_ao->Desc.Extent ,
				gb_tex_bc_ao->Desc.Format,
				{},
				ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable
			), TEXT("Naga_GBuffer_C_Work"));
		
		// Copy GBuffer -> WorkBuffer.
		{
			FRHICopyTextureInfo copy_info{};
			AddCopyTexturePass(GraphBuilder, gb_tex_pbr_shadingmodel, new_gbuffer_tex_pbr_shadingmodel, copy_info);
			AddCopyTexturePass(GraphBuilder, gb_tex_bc_ao, new_gbuffer_tex_bc_ao, copy_info);
		}

		// もとのShadingModelを格納したコピーGBuffer参照を保存.
		FrameExtendGBuffer = new_gbuffer_tex_pbr_shadingmodel;
		
		{
			TShaderMapRef<FPostBasePassModifyGBufferVs> VertexShader(GlobalShaderMap);
			TShaderMapRef<FPostBasePassModifyGBufferPs> PixelShader(GlobalShaderMap);

			FPostBasePassModifyGBufferPs::FParameters* Parameters = GraphBuilder.AllocParameters<FPostBasePassModifyGBufferPs::FParameters>();
			{
				Parameters->pass0_sampler_screen = PointClampSampler;

				Parameters->pass0_tex_gbuffer_b_custom = new_gbuffer_tex_pbr_shadingmodel;
				Parameters->pass0_tex_gbuffer_c = new_gbuffer_tex_bc_ao;

				// オリジナルのGBufferへ書き戻して修正する.
				Parameters->RenderTargets[0] = RenderTargets[RT_GB_PbrShadingModel];
				Parameters->RenderTargets[1] = RenderTargets[RT_GB_BcAo];
			}
			
			FRHIBlendState* BlendState =
				TStaticBlendState<
					CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero,
					CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero
				>::GetRHI();
			
			const FScreenPassTextureViewport RegionViewport(GetAsTexture(RenderTargets[RT_GB_PbrShadingModel].GetTexture()), PrimaryViewRect);
			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("Naga_GBufferModify"),
				InView,
				RegionViewport,
				RegionViewport,
				VertexShader,
				PixelShader,
				BlendState,
				Parameters
			);
		}
	}
}
/**
 * Called right before Post Processing rendering begins
 */
void FViewExtensionSampleVe::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	if (!IsValid(subsystem))
	{
		return;
	}
	// チェック.
	Inputs.Validate();

	const FSceneViewFamily& ViewFamily = *View.Family;

	// We need to make sure to take Windows and Scene scale into account.
	float ScreenPercentage = ViewFamily.SecondaryViewFraction;

	if (ViewFamily.GetScreenPercentageInterface())
	{
		DynamicRenderScaling::TMap<float> UpperBounds = ViewFamily.GetScreenPercentageInterface()->GetResolutionFractionsUpperBound();
		ScreenPercentage *= UpperBounds[GDynamicPrimaryResolutionFraction];
	}

	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
	FUintVector2 WorkRect(PrimaryViewRect.Width(), PrimaryViewRect.Height());

	FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
	const FScreenPassTextureViewport SceneColorTextureViewport(SceneColor);

	if (!SceneColor.IsValid())
	{
		return;
	}
	
	RDG_EVENT_SCOPE(GraphBuilder, "Naga_PrePostProcessPass %dx%d", SceneColorTextureViewport.Rect.Width(), SceneColorTextureViewport.Rect.Height());

	// Getting material data for the current view.
	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	
	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	const auto& input_scene_textures = Inputs.SceneTextures->GetParameters();


	// History Texture.
	FRDGTextureRef NewHistoryTexture{};
	FRDGTextureRef PrevHistoryTexture{};
	{
		FRDGTextureDesc WorkOutputDesc = {};;
		{
			const auto scene_color_desc = SceneColor.Texture->Desc;
			
			WorkOutputDesc = FRDGTextureDesc::Create2D(
				scene_color_desc.Extent, scene_color_desc.Format, scene_color_desc.ClearValue,
				ETextureCreateFlags::ShaderResource|ETextureCreateFlags::RenderTargetable|ETextureCreateFlags::UAV
				);
		}
		// 現フレームから次フレームへ送り出すHistoryTexture.
		NewHistoryTexture = GraphBuilder.CreateTexture(WorkOutputDesc, TEXT("NagaViewExtensionHistoryTexture"));

		// 前フレームのHistoryTexture.
		if(HistoryTexture.IsValid())
		{
			// 前フレームから有効なテクスチャが抽出されている場合はRDGに登録して利用準備.
			PrevHistoryTexture = GraphBuilder.RegisterExternalTexture(HistoryTexture);
		}
		else
		{
			// 初回フレームの場合は安全策.
			PrevHistoryTexture = GraphBuilder.CreateTexture(WorkOutputDesc, TEXT("NagaViewExtensionHistoryTexture"));
			AddCopyTexturePass(GraphBuilder, SceneColor.Texture, PrevHistoryTexture);
		}
	}
	

	// Aniso Kuwahara.
	if(subsystem->enable_aniso_kuwahara)
	{	
		AddAnisoKuwaharaPass(GraphBuilder, View, Inputs);
	}
	
	if(subsystem->enable_voronoi_test)
	{
		// Voronoi用Seedバッファ.
		FRDGTexture* VoronoiCellTexture{};
		FRDGTextureDesc VoronoiWorkUavTexDesc = {};;
		{
			{
				// Voronoi Diagram計算のピクセル座標格納のため, 1024以上の精度を要求. 16bit float推奨.
				const EPixelFormat work_format = PF_FloatRGBA;//16bit float.
				const auto scene_color_desc = SceneColor.Texture->Desc;
				// R16B16_Floatが望ましいがUEで選択できないため R32G32B32_Float 等としている. Shader側RWTextureの型と合わせることに注意.
				VoronoiWorkUavTexDesc = FRDGTextureDesc::Create2D(
					scene_color_desc.Extent, work_format, scene_color_desc.ClearValue,
					ETextureCreateFlags::ShaderResource|ETextureCreateFlags::UAV
					);
			}
			VoronoiCellTexture = GraphBuilder.CreateTexture(VoronoiWorkUavTexDesc, TEXT("NagaWorkTexture0"));
		}

		// Generate Voronoi Cell Seed (Edge detection and other).
		{
			FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(VoronoiCellTexture);
			FImageProcessTestCS::FParameters* Parameters = GraphBuilder.AllocParameters<FImageProcessTestCS::FParameters>();
			{
				Parameters->pass1_View = View.ViewUniformBuffer;// DeviceDepth to ViewDepth変換等のため.
				
				Parameters->pass1_SceneDepthTexture = input_scene_textures->SceneDepthTexture;
				Parameters->pass1_SourceDimensions = WorkRect;
				Parameters->pass1_SourceSampler = PointClampSampler;

				Parameters->pass1_OutputTexture = WorkUav;
				Parameters->pass1_OutputDimensions = WorkRect;

				Parameters->pass1_DepthEdgeCoef = subsystem->depth_edge_coef;
				Parameters->pass1_EnableTileCell = subsystem->enable_voronoi_tile_cell;
			}
		
			TShaderMapRef<FImageProcessTestCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
					FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
		
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("NagaGenerateEdge"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
		}

		// Generate Voronoi.
		FRDGTexture* voronoi_texture = FNagaVoronoiJfaCompute::Execute(GraphBuilder, VoronoiCellTexture, WorkRect);
		
		{
			// SceneColor をコピーするための Work Textureを作成.
			FRDGTextureDesc WorkSceneColorDesc = {};;
			{
				const auto scene_color_desc = SceneColor.Texture->Desc;
				WorkSceneColorDesc = FRDGTextureDesc::Create2D(
					scene_color_desc.Extent, scene_color_desc.Format, scene_color_desc.ClearValue,
					ETextureCreateFlags::ShaderResource|ETextureCreateFlags::RenderTargetable
					);
			}
			FRDGTexture* WorkSceneColorTexture = GraphBuilder.CreateTexture(WorkSceneColorDesc, TEXT("NagaWorkTexture"));
			// SceneColorを Workへコピー.
			{
				FRHICopyTextureInfo copy_info{};
				AddCopyTexturePass(GraphBuilder, SceneColor.Texture, WorkSceneColorTexture, copy_info);
			}

			FRDGTextureUAVRef SceneColorUav = GraphBuilder.CreateUAV(SceneColor.Texture);
			FTestFinalCS::FParameters* Parameters = GraphBuilder.AllocParameters<FTestFinalCS::FParameters>();
			{
				Parameters->pass0_SourceTexture = WorkSceneColorTexture;
				Parameters->pass0_VoronoiWorkTexture = voronoi_texture;
				Parameters->pass0_SourceSampler = PointClampSampler;
				Parameters->pass0_SourceDimensions = WorkRect;

				Parameters->pass0_OutputTexture = SceneColorUav;
				Parameters->pass0_SourceDimensions = WorkRect;
				
				Parameters->pass0_VisualizeMode = subsystem->edge_debug_view;
			}
		
			TShaderMapRef<FTestFinalCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
					FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
		
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("NagaVoronoiTest"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
		}
	}

	// History Test.
	if(subsystem->enable_history_test)
	{
		// SceneColor をコピーするための Work Textureを作成.
		FRDGTextureDesc WorkSceneColorDesc = {};;
		{
			const auto scene_color_desc = SceneColor.Texture->Desc;
			WorkSceneColorDesc = FRDGTextureDesc::Create2D(
				scene_color_desc.Extent, scene_color_desc.Format, scene_color_desc.ClearValue,
				ETextureCreateFlags::ShaderResource|ETextureCreateFlags::RenderTargetable
				);
		}
		FRDGTexture* WorkSceneColorTexture = GraphBuilder.CreateTexture(WorkSceneColorDesc, TEXT("NagaHistoryWorkTexture"));
		// SceneColorを Workへコピー.
		{
			FRHICopyTextureInfo copy_info{};
			AddCopyTexturePass(GraphBuilder, SceneColor.Texture, WorkSceneColorTexture, copy_info);
		}

		FRDGTextureUAVRef SceneColorUav = GraphBuilder.CreateUAV(SceneColor.Texture);
		FHistoryTestCS::FParameters* Parameters = GraphBuilder.AllocParameters<FHistoryTestCS::FParameters>();
		{
			Parameters->SourceTexture = WorkSceneColorTexture;
			Parameters->SourceSampler = PointClampSampler;
			Parameters->SourceDimensions = WorkRect;

			Parameters->HistoryTexture = PrevHistoryTexture;

			Parameters->OutputTexture = SceneColorUav;
			Parameters->SourceDimensions = WorkRect;
		}
		
		TShaderMapRef<FHistoryTestCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
		
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("NagaHistoryTest"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	
	// Historyテスト.
	if(subsystem->enable_history_test)
	{
		// SceneColorを次フレームへのHistoryTextureへコピー.
		FRHICopyTextureInfo copy_info{};
		AddCopyTexturePass(GraphBuilder, SceneColor.Texture, NewHistoryTexture, copy_info);

		// HistoryTextureをExtractionして次フレーム利用へ.
		GraphBuilder.QueueTextureExtraction(NewHistoryTexture, &HistoryTexture);
	}
}

void FViewExtensionSampleVe::SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	// MotionBlur後, Tonemapの前.
	if (PassId == EPostProcessingPass::MotionBlur)
	{
		if(subsystem->enable_lens_ghost)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateLambda([this, InPassId = PassId](FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InInputs)
			{
					const FScreenPassTexture& SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, InInputs.GetInput(EPostProcessMaterialInput::SceneColor));
					check(SceneColor.IsValid());

					// 内部でのテクスチャ全領域に対する描画領域の範囲を後段に伝えるためにFScreenPassTextureを返す.
					FScreenPassTexture PassOutputTexture = AddLensGhostPass(GraphBuilder, View, SceneColor);
					return PassOutputTexture;
				}));
		}
		
	}
}


void FViewExtensionSampleVe::AddAnisoKuwaharaPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
	FUintVector2 WorkRect(PrimaryViewRect.Width(), PrimaryViewRect.Height());

	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();


	RDG_EVENT_SCOPE(GraphBuilder, "Naga_AnisoKuwaharaPass %dx%d", PrimaryViewRect.Width(), PrimaryViewRect.Height());

	
	FRDGTextureRef tex_eigenvector = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			FIntPoint(PrimaryViewRect.Width(), PrimaryViewRect.Height()),
			EPixelFormat::PF_FloatRGBA,
			{},
			ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV
		), TEXT("AnisoKuwaharaEigen0"));
	
	FRDGTextureRef tex_eigenvector_blurwork = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			tex_eigenvector->Desc.Extent,
			tex_eigenvector->Desc.Format,
			{},
			tex_eigenvector->Desc.Flags
		), TEXT("AnisoKuwaharaEigen1"));

	FRDGTextureRef tex_aniso = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			tex_eigenvector->Desc.Extent,
			tex_eigenvector->Desc.Format,
			{},
			tex_eigenvector->Desc.Flags
		), TEXT("AnisoKuwaharaAniso"));
	
	// Eigenvector.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_eigenvector);
		FAnisoKuwaharaEigenvectorCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaEigenvectorCS::FParameters>();
		{
			Parameters->eigenvectorpass_SourceTexture = Inputs.SceneTextures->GetParameters()->SceneColorTexture;
			Parameters->eigenvectorpass_SourceDimensions = WorkRect;
			Parameters->eigenvectorpass_SourceSampler = PointClampSampler;

			Parameters->eigenvectorpass_OutputTexture = WorkUav;
			Parameters->eigenvectorpass_OutputDimensions = WorkRect;
		}
	
		TShaderMapRef<FAnisoKuwaharaEigenvectorCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaEigenvector"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	// Blur.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_eigenvector_blurwork);
		FAnisoKuwaharaBlurCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaBlurCS::FParameters>();
		{
			Parameters->blurpass_SourceTexture = tex_eigenvector;
			Parameters->blurpass_SourceDimensions = WorkRect;
			Parameters->blurpass_SourceSampler = PointClampSampler;

			Parameters->blurpass_OutputTexture = WorkUav;
			Parameters->blurpass_OutputDimensions = WorkRect;
		}
	
		TShaderMapRef<FAnisoKuwaharaBlurCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaBlur"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	
	// CalcAniso.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_aniso);
		FAnisoKuwaharaCalcAnisoCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaCalcAnisoCS::FParameters>();
		{
			Parameters->calcanisopass_SourceTexture = tex_eigenvector_blurwork;
			Parameters->calcanisopass_SourceDimensions = WorkRect;
			Parameters->calcanisopass_SourceSampler = PointClampSampler;

			Parameters->calcanisopass_OutputTexture = WorkUav;
			Parameters->calcanisopass_OutputDimensions = WorkRect;
		}
	
		TShaderMapRef<FAnisoKuwaharaCalcAnisoCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaBlur"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}

	
	FRDGTextureDesc TexSceneColorCopyDesc = {};;
	{
		const auto scene_color_desc = Inputs.SceneTextures->GetParameters()->SceneColorTexture->Desc;
		
		TexSceneColorCopyDesc = FRDGTextureDesc::Create2D(
			scene_color_desc.Extent,
			scene_color_desc.Format, scene_color_desc.ClearValue,
			ETextureCreateFlags::ShaderResource|ETextureCreateFlags::RenderTargetable|ETextureCreateFlags::UAV
			);
	}

	FRDGTextureRef tex_scene_color_copy = GraphBuilder.CreateTexture(TexSceneColorCopyDesc, TEXT("NagaViewExtensionSceneColorCopy"));

	// Copy.
	{
		FRHICopyTextureInfo copy_info{};
		AddCopyTexturePass(GraphBuilder, Inputs.SceneTextures->GetParameters()->SceneColorTexture, tex_scene_color_copy, copy_info);
	}
	
	// Final.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(Inputs.SceneTextures->GetParameters()->SceneColorTexture);
		FAnisoKuwaharaFinalCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAnisoKuwaharaFinalCS::FParameters>();
		{
			Parameters->finalpass_SourceTexture = tex_aniso;
			Parameters->finalpass_SourceDimensions = WorkRect;
			Parameters->finalpass_SourceSampler = PointClampSampler;

			Parameters->finalpass_SceneTexture = tex_scene_color_copy;

			Parameters->finalpass_OutputTexture = WorkUav;
			Parameters->finalpass_OutputDimensions = WorkRect;

			Parameters->finalpass_aniso_control = subsystem->aniso_kuwahara_aniso_control;// 1.0f;
			Parameters->finalpass_hardness = subsystem->aniso_kuwahara_hardness;//8.0f;
			Parameters->finalpass_sharpness = subsystem->aniso_kuwahara_sharpness;//8.0f;
		}
	
		TShaderMapRef<FAnisoKuwaharaFinalCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(WorkRect.X, cs->THREADGROUPSIZE_X),
				FMath::DivideAndRoundUp(WorkRect.Y, cs->THREADGROUPSIZE_Y), 1);
	
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("AnisoKuwaharaEigen"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
}


FScreenPassTexture FViewExtensionSampleVe::AddLensGhostPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SourceTexture)
{
	const FScreenPassTextureViewport InputViewport(SourceTexture);
	const FScreenPassTextureViewport OutputViewport(SourceTexture);

	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	RDG_EVENT_SCOPE(GraphBuilder, "Naga_LensGhostPass %dx%d", OutputViewport.Rect.Width(), OutputViewport.Rect.Height());
	
	FRDGTextureRef tex_bright_seed = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			OutputViewport.Extent,
			EPixelFormat::PF_FloatRGBA,
			{},
			ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV
		), TEXT("LensGhostBright"));
	
	FRDGTextureRef tex_ghost = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			OutputViewport.Extent,
			EPixelFormat::PF_FloatRGBA,
			{},
			ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV
		), TEXT("LensGhostBright"));

	FRDGTextureRef tex_composite = GraphBuilder.CreateTexture(
		FRDGTextureDesc::Create2D(
			OutputViewport.Extent,
			EPixelFormat::PF_FloatRGBA,
			{},
			ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV
		), TEXT("LensGhostBright"));

	FUintVector2 InputRect = FUintVector2(InputViewport.Rect.Width(), InputViewport.Rect.Height());
	// Extract Bright.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_bright_seed);
		auto* Parameters = GraphBuilder.AllocParameters<FLensGhostExtractBrightCS::FParameters>();
		{
			Parameters->SourceTexture = SourceTexture.Texture;
			Parameters->SourceDimensions = InputRect;
			Parameters->SourceValidRectScale = FVector2f(InputRect.X, InputRect.Y) / SourceTexture.Texture->Desc.Extent;
			Parameters->SourceSampler = PointClampSampler;

			Parameters->extract_threshold = subsystem->lens_ghost_bright_threshold;
			Parameters->chroma_shift = subsystem->lens_ghost_chroma_shift;
			
			Parameters->OutputTexture = WorkUav;
			Parameters->OutputDimensions = InputRect;
		}

		TShaderMapRef<FLensGhostExtractBrightCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		const auto DispatchGroupSize = CalcComputeDispatchGroupSizeHelper(cs, InputRect);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("LensGhostExtractBright"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	
	// Generate Ghost.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_ghost);
		auto* Parameters = GraphBuilder.AllocParameters<FLensGhostGenerateCS::FParameters>();
		{
			Parameters->SeedTexture = tex_bright_seed;
			Parameters->SeedDimensions = InputRect;
			Parameters->SeedValidRectScale = FVector2f(InputRect.X, InputRect.Y) / SourceTexture.Texture->Desc.Extent;
			Parameters->SeedSampler = PointClampSampler;

			Parameters->ghost_sample_count = subsystem->lens_ghost_sample_count;
			Parameters->ghost_sample_step_scale = subsystem->lens_ghost_step_scale;

			Parameters->halo_sample_length = subsystem->halo_sample_length;
			
			Parameters->OutputTexture = WorkUav;
			Parameters->OutputDimensions = InputRect;
		}

		TShaderMapRef<FLensGhostGenerateCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		const auto DispatchGroupSize = CalcComputeDispatchGroupSizeHelper(cs, InputRect);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("LensGhostGenerate"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	
	// Composite.
	{
		FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(tex_composite);
		auto* Parameters = GraphBuilder.AllocParameters<FLensGhostCompositeCS::FParameters>();
		{
			Parameters->LensGhostTexture = tex_ghost;
			Parameters->LensGhostDimensions = InputRect;
			
			Parameters->SceneTexture = SourceTexture.Texture;
			
			Parameters->LensGhostSampler = PointClampSampler;
			
			Parameters->OutputTexture = WorkUav;
			Parameters->OutputDimensions = InputRect;
		}
	
		TShaderMapRef<FLensGhostCompositeCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		const auto DispatchGroupSize = CalcComputeDispatchGroupSizeHelper(cs, InputRect);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("LensGhostComposite"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
	}
	
	return FScreenPassTexture(tex_composite, InputViewport.Rect);
}