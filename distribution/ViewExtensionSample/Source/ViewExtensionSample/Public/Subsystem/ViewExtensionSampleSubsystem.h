#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"
#include "Subsystems/EngineSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"



#include "ViewExtensionSampleSubsystem.generated.h"


class UViewExtensionSampleSubsystem;

// Subsystemへのパラメータ設定用Actor.
UCLASS()
class AViewExtensionSampleControlActor : public AActor
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	
public:
	AViewExtensionSampleControlActor();
	
	virtual bool ShouldTickIfViewportsOnly() const override { return true; };
	virtual void Tick(float DeltaSeconds) override;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="HistoryTest")
	bool enable_history_test = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="WorldNormalUnlitTest")
	bool enable_worldnormal_unlit = false;


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="VoronoiEffectTest")
	bool enable_voronoi_test = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="VoronoiEffectTest")
	float depth_edge_coef = 0.02f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="VoronoiEffectTest")
	bool enable_voronoi_tile_cell = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="VoronoiEffectTest", meta = (ClampMin = 0))
	int edge_debug_view = 0;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	bool enable_aniso_kuwahara = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	float aniso_kuwahara_aniso_control = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	float aniso_kuwahara_hardness = 8.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AnisoKuwahara")
	float aniso_kuwahara_sharpness = 8.0f;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LensGhost")
	bool enable_lensh_ghost = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LensGhost", meta = (ClampMin = 0.0))
	float lens_ghost_bright_threshold = 0.9f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LensGhost", meta = (ClampMin = 0.0))
	float lens_ghost_chroma_shift = 0.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LensGhost", meta = (ClampMin = 0, ClampMax = 16))
	int lensh_ghost_sample_count = 5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LensGhost")
	float lensh_ghost_step_scale = 0.1f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LensGhost")
	float halo_sample_length = 0.1f;
	
};

// ViewExtensionインスタンス管理用Subsystem.
UCLASS()
class UViewExtensionSampleSubsystem : public UEngineSubsystem /*UWorldSubsystem*/
{
	GENERATED_BODY()
public:

	// Subsystem Init/Deinit
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr< class FViewExtensionSampleVe, ESPMode::ThreadSafe > p_view_extension;

public:
	friend class FViewExtensionSampleVe;

	bool enable_voronoi_test = false;
	float depth_edge_coef = 0.02f;
	bool enable_voronoi_tile_cell = true;
	int edge_debug_view = 0;

	bool enable_history_test = false;
	
	bool enable_worldnormal_unlit = false;

	bool enable_aniso_kuwahara = false;
	float aniso_kuwahara_aniso_control = 1.0f;
	float aniso_kuwahara_hardness = 8.0f;
	float aniso_kuwahara_sharpness = 8.0f;

	bool enable_lens_ghost = true;
	float lens_ghost_bright_threshold = 0.9f;
	float lens_ghost_chroma_shift = 0.0f;
	int lens_ghost_sample_count = 5;
	float lens_ghost_step_scale = 0.1f;
	float halo_sample_length = 0.1f;
};