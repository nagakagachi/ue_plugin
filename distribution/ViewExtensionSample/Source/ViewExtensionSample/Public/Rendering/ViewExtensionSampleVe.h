#pragma once

#include "SceneViewExtension.h"
#include "RHI.h"
#include "RHIResources.h"

class UViewExtensionSampleSubsystem;

class FViewExtensionSampleVe : public FSceneViewExtensionBase
{
public:
	FViewExtensionSampleVe(const FAutoRegister& AutoRegister, UViewExtensionSampleSubsystem* InWorldSubsystem);
	
	//~ Begin FSceneViewExtensionBase Interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	
	/**
	 * Called right after Base Pass rendering finished when using the deferred renderer.
	 */
	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	
	/**
	 * Called right before Post Processing rendering begins
	 */
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

	/**
	* This will be called at the beginning of post processing to make sure that each view extension gets a chance to subscribe to an after pass event.
	*/
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
	
	//~ End FSceneViewExtensionBase Interface
public:

private:
	// SceneTextureをソースにSceneTextureへ書き込み.
	void AddAnisoKuwaharaPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs);
	
	// SceneTextureをソースにSceneTextureへ書き込み.
	FScreenPassTexture AddLensGhostPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SourceTexture);
	
private:
	UViewExtensionSampleSubsystem* subsystem{};

	FRDGTextureRef	FrameExtendGBuffer{};
	
	TRefCountPtr<IPooledRenderTarget> HistoryTexture{};
};