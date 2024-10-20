
#include "Rendering/NagaVoronoiJfaCompute.h"

#include "RHIStaticStates.h"

#include "GlobalShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialShader.h"
#include "ShaderParameterStruct.h"
#include "FXRenderingUtils.h"

// Voronoi Jump Flooding.
class FVoronoiJumpFloodingCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	
public:
	DECLARE_GLOBAL_SHADER(FVoronoiJumpFloodingCS);
	SHADER_USE_PARAMETER_STRUCT(FVoronoiJumpFloodingCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, VoronoiWorkTexture)
		SHADER_PARAMETER(FUintVector2, VoronoiWorkTextureDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, VoronoiWorkTextureSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float3>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, OutputDimensions)

		SHADER_PARAMETER(uint32, IsJumpFloodingFirstPass)
		SHADER_PARAMETER(uint32, JumpFloodingStepSize)
	
	END_SHADER_PARAMETER_STRUCT()

	//Called by the engine to determine which permutations to compile for this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		return !IsMobilePlatform(Parameters.Platform);
	}
	//Modifies the compilations environment of the shader
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		
		// Thread Group数マクロ指定.
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};
IMPLEMENT_GLOBAL_SHADER(FVoronoiJumpFloodingCS, "/ViewExtensionSampleShaders/Private/voronoi_jfa_compute.usf", "MainCS", SF_Compute );



// Input Voronoi CellMask Texture : 0->NonCell, 1->Cell
// Output Voronoi Diagram Texture : RGBA_float, RG->PixelPosition, B->CellMask
FRDGTexture* FNagaVoronoiJfaCompute::Execute(FRDGBuilder& GraphBuilder, FRDGTexture* seed_buffer, FUintVector2 work_size)
{
	FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	
	// Work作成.
	FRDGTexture* VoronoiWorkUavTexture0{};
	FRDGTexture* VoronoiWorkUavTexture1{};
	FRDGTextureDesc VoronoiWorkUavTexDesc = {};;
	{
		{
			// Voronoi Diagram計算のピクセル座標格納のため, 1024以上の精度を要求. 16bit float推奨.
			const EPixelFormat work_format = PF_FloatRGBA;//16bit float.
			
			// R16B16_Floatが望ましいがUEで選択できないためRGBA等を利用している.
			VoronoiWorkUavTexDesc = FRDGTextureDesc::Create2D(
				FIntPoint(work_size.X, work_size.Y), work_format, {},
				ETextureCreateFlags::ShaderResource|ETextureCreateFlags::UAV
				);
		}
		VoronoiWorkUavTexture0 = GraphBuilder.CreateTexture(VoronoiWorkUavTexDesc, TEXT("NagaVoronoiJfaWoek0"));
		VoronoiWorkUavTexture1 = GraphBuilder.CreateTexture(VoronoiWorkUavTexDesc, TEXT("NagaVoronoiJfaWoek1"));
	}
	// PingPong用.
	FRDGTexture* work_buffers[] =
	{
		VoronoiWorkUavTexture0,
		VoronoiWorkUavTexture1
	};

	
	int last_work_index = 0;
	{
		TShaderMapRef<FVoronoiJumpFloodingCS> cs(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		
		// 初回パス.
		{	
			FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(work_buffers[last_work_index]);
			FVoronoiJumpFloodingCS::FParameters* Parameters = GraphBuilder.AllocParameters<FVoronoiJumpFloodingCS::FParameters>();
			{
				Parameters->VoronoiWorkTexture = seed_buffer;
				Parameters->VoronoiWorkTextureDimensions = work_size;
				Parameters->VoronoiWorkTextureSampler = PointClampSampler;

				Parameters->OutputTexture = WorkUav;
				Parameters->OutputDimensions = work_size;

				Parameters->IsJumpFloodingFirstPass = 1;
				Parameters->JumpFloodingStepSize = 1;
			}
	

			FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(work_size.X, cs->THREADGROUPSIZE_X),
					FMath::DivideAndRoundUp(work_size.Y, cs->THREADGROUPSIZE_Y), 1);
	
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("NagaVoronoiJfa_FirstPass"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
		}
		
		const int JfaTargetSize = std::max(work_size.X, work_size.Y);
		const int JfaStepCount = log2(JfaTargetSize) - 1;
		// 反復.
		for(int jfa_step = 0; jfa_step < JfaStepCount; ++jfa_step)
		{
			const int jfa_step_size = pow(2, JfaStepCount - jfa_step);
			
			const int src_index = last_work_index;;
			last_work_index = 1 - last_work_index;
			
			FRDGTextureUAVRef WorkUav = GraphBuilder.CreateUAV(work_buffers[last_work_index]);
			FVoronoiJumpFloodingCS::FParameters* Parameters = GraphBuilder.AllocParameters<FVoronoiJumpFloodingCS::FParameters>();
			{
				Parameters->VoronoiWorkTexture = work_buffers[src_index];
				Parameters->VoronoiWorkTextureDimensions = work_size;
				Parameters->VoronoiWorkTextureSampler = PointClampSampler;

				Parameters->OutputTexture = WorkUav;
				Parameters->OutputDimensions = work_size;

				Parameters->IsJumpFloodingFirstPass = 0;
				Parameters->JumpFloodingStepSize = jfa_step_size;
			}

			FIntVector DispatchGroupSize = FIntVector(FMath::DivideAndRoundUp(work_size.X, cs->THREADGROUPSIZE_X),
					FMath::DivideAndRoundUp(work_size.Y, cs->THREADGROUPSIZE_Y), 1);
	
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("NagaVoronoiJfa_IteratePass"), ERDGPassFlags::Compute, cs, Parameters, DispatchGroupSize);
		}
	}

	// 最終結果のbufferポインタを返す.
	return work_buffers[last_work_index];
}
