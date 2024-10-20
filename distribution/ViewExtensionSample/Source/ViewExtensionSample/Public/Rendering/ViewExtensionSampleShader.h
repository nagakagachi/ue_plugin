#pragma once



#include "GlobalShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialShader.h"
#include "ShaderParameterStruct.h"

// 基底.
class FPostBasePassModifyGBuffer : public FGlobalShader
{
public:
	// ------------------------------------------------------------------------------------------
	// Declare Constructors for Shader. and bind local "FParameters" to ShaderParameter.
	BEGIN_SHADER_PARAMETER_STRUCT(FShaderInnerParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, pass0_View)

		SHADER_PARAMETER_SAMPLER(SamplerState,		pass0_sampler_screen)
	
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D,		pass0_tex_gbuffer_b_custom)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D,		pass0_tex_gbuffer_c)
	
		RENDER_TARGET_BINDING_SLOTS()	// RenderTargetバインド.
	END_SHADER_PARAMETER_STRUCT()
	using FParameters = FShaderInnerParameters;
	
	// LEGACYではない SHADER_USE_PARAMETER_STRUCT はこのシェーダ自体で完全なパラメータバインディングをしないとエラーチェックに引っかかるため, エンジンコードでもまだLEGACYが使われている.
	SHADER_USE_PARAMETER_STRUCT_WITH_LEGACY_BASE(FPostBasePassModifyGBuffer, FGlobalShader);
	// ------------------------------------------------------------------------------------------
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return !IsMobilePlatform(Parameters.Platform);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
// SampleShaderのVS実装.
class FPostBasePassModifyGBufferVs : public FPostBasePassModifyGBuffer
{
public:
	DECLARE_GLOBAL_SHADER(FPostBasePassModifyGBufferVs);
};

// SampleShaderのPS実装.
class FPostBasePassModifyGBufferPs : public FPostBasePassModifyGBuffer
{
public:
	DECLARE_GLOBAL_SHADER(FPostBasePassModifyGBufferPs);
};


class FTestFinalCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FTestFinalCS);
	SHADER_USE_PARAMETER_STRUCT(FTestFinalCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, pass0_SourceTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, pass0_VoronoiWorkTexture)
		SHADER_PARAMETER(FUintVector2, pass0_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, pass0_SourceSampler)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, pass0_OutputTexture)
		SHADER_PARAMETER(FUintVector2, pass0_OutputDimensions)

		SHADER_PARAMETER(uint32, pass0_VisualizeMode)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};

class FImageProcessTestCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FImageProcessTestCS);
	SHADER_USE_PARAMETER_STRUCT(FImageProcessTestCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, pass1_View)
	
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, pass1_SceneDepthTexture)
		SHADER_PARAMETER(FUintVector2, pass1_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, pass1_SourceSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float3>, pass1_OutputTexture)
		SHADER_PARAMETER(FUintVector2, pass1_OutputDimensions)

		SHADER_PARAMETER(float, pass1_DepthEdgeCoef)
		SHADER_PARAMETER(uint32, pass1_EnableTileCell)
	
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};



class FAnisoKuwaharaEigenvectorCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaEigenvectorCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaEigenvectorCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, eigenvectorpass_SourceTexture)
		SHADER_PARAMETER(FUintVector2, eigenvectorpass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, eigenvectorpass_SourceSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, eigenvectorpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, eigenvectorpass_OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};
class FAnisoKuwaharaCalcAnisoCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaCalcAnisoCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaCalcAnisoCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, calcanisopass_SourceTexture)
		SHADER_PARAMETER(FUintVector2, calcanisopass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, calcanisopass_SourceSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, calcanisopass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, calcanisopass_OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};
class FAnisoKuwaharaBlurCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaBlurCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaBlurCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, blurpass_SourceTexture)
		SHADER_PARAMETER(FUintVector2, blurpass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, blurpass_SourceSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, blurpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, blurpass_OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};
class FAnisoKuwaharaFinalCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FAnisoKuwaharaFinalCS);
	SHADER_USE_PARAMETER_STRUCT(FAnisoKuwaharaFinalCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, finalpass_SourceTexture)
		SHADER_PARAMETER(FUintVector2, finalpass_SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, finalpass_SourceSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, finalpass_SceneTexture)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, finalpass_OutputTexture)
		SHADER_PARAMETER(FUintVector2, finalpass_OutputDimensions)

		SHADER_PARAMETER(float, finalpass_aniso_control)	// 1
		SHADER_PARAMETER(float, finalpass_hardness)			// 8
		SHADER_PARAMETER(float, finalpass_sharpness)		// 8
	
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};




class FLensGhostExtractBrightCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FLensGhostExtractBrightCS);
	SHADER_USE_PARAMETER_STRUCT(FLensGhostExtractBrightCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
		SHADER_PARAMETER(FUintVector2, SourceDimensions)
		SHADER_PARAMETER(FVector2f, SourceValidRectScale)
		SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
	
		SHADER_PARAMETER(float, extract_threshold)
		SHADER_PARAMETER(float, chroma_shift)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};
class FLensGhostGenerateCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FLensGhostGenerateCS);
	SHADER_USE_PARAMETER_STRUCT(FLensGhostGenerateCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SeedTexture)
		SHADER_PARAMETER(FUintVector2, SeedDimensions)
		SHADER_PARAMETER(FVector2f, SeedValidRectScale)
		SHADER_PARAMETER_SAMPLER(SamplerState, SeedSampler)
	
		SHADER_PARAMETER(int, ghost_sample_count)
		SHADER_PARAMETER(float, ghost_sample_step_scale)
	
		SHADER_PARAMETER(float, halo_sample_length)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};
class FLensGhostCompositeCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FLensGhostCompositeCS);
	SHADER_USE_PARAMETER_STRUCT(FLensGhostCompositeCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, LensGhostTexture)
		SHADER_PARAMETER(FUintVector2, LensGhostDimensions)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, LensGhostSampler)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};




// History Test.
class FHistoryTestCS : public FGlobalShader
{
public:
	static constexpr uint32 THREADGROUPSIZE_X = 16;
	static constexpr uint32 THREADGROUPSIZE_Y = 16;
	static constexpr uint32 THREADGROUPSIZE_Z = 1;
	
public:
	DECLARE_GLOBAL_SHADER(FHistoryTestCS);
	SHADER_USE_PARAMETER_STRUCT(FHistoryTestCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
		SHADER_PARAMETER(FUintVector2, SourceDimensions)
		SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HistoryTexture)
	
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, OutputDimensions)
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
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), THREADGROUPSIZE_X);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), THREADGROUPSIZE_Y);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), THREADGROUPSIZE_Z);
	}
};