

#include "Subsystem/ViewExtensionSampleSubsystem.h"

#include "Rendering/ViewExtensionSampleVe.h"

AViewExtensionSampleControlActor::AViewExtensionSampleControlActor()
{
	PrimaryActorTick.bCanEverTick = true;
}
void AViewExtensionSampleControlActor::BeginPlay()
{
	Super::BeginPlay();
}
void AViewExtensionSampleControlActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	{
		auto* subsystem = GEngine->GetEngineSubsystem<UViewExtensionSampleSubsystem>();
		if(subsystem)
		{
			subsystem->enable_voronoi_test = enable_voronoi_test;
			subsystem->depth_edge_coef = depth_edge_coef;
			subsystem->edge_debug_view = edge_debug_view;
			subsystem->enable_voronoi_tile_cell = enable_voronoi_tile_cell;

			subsystem->enable_history_test = enable_history_test;
			
			subsystem->enable_worldnormal_unlit = enable_worldnormal_unlit;

			subsystem->enable_aniso_kuwahara = enable_aniso_kuwahara;
			subsystem->aniso_kuwahara_aniso_control = aniso_kuwahara_aniso_control;
			subsystem->aniso_kuwahara_hardness = aniso_kuwahara_hardness;
			subsystem->aniso_kuwahara_sharpness = aniso_kuwahara_sharpness;

			subsystem->enable_lens_ghost = enable_lensh_ghost;
			subsystem->lens_ghost_bright_threshold = lens_ghost_bright_threshold;
			subsystem->lens_ghost_chroma_shift = lens_ghost_chroma_shift;
			subsystem->lens_ghost_sample_count = lensh_ghost_sample_count;
			subsystem->lens_ghost_step_scale = lensh_ghost_step_scale;
			subsystem->halo_sample_length = halo_sample_length;
		}
	}
}


void UViewExtensionSampleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	p_view_extension = FSceneViewExtensions::NewExtension<FViewExtensionSampleVe>(this);

	UE_LOG(LogTemp, Log, TEXT("[UViewExtensionSampleSubsystem] Initialize %p\n"), this);
}

void UViewExtensionSampleSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("[UViewExtensionSampleSubsystem] Deinitialize %p\n"), this);

	if(p_view_extension)
	{
		// Prevent this SVE from being gathered, in case it is kept alive by a strong reference somewhere else.
		{
			p_view_extension->IsActiveThisFrameFunctions.Empty();

			FSceneViewExtensionIsActiveFunctor IsActiveFunctor;
			IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
			{
				return TOptional<bool>(false);
			};

			p_view_extension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
		}

		ENQUEUE_RENDER_COMMAND(ReleaseSVE)([this](FRHICommandListImmediate& RHICmdList)
		{
			// Prevent this SVE from being gathered, in case it is kept alive by a strong reference somewhere else.
			{
				p_view_extension->IsActiveThisFrameFunctions.Empty();

				FSceneViewExtensionIsActiveFunctor IsActiveFunctor;
				IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
				{
					return TOptional<bool>(false);
				};

				p_view_extension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
			}

			p_view_extension.Reset();
			p_view_extension = nullptr;
		});
	}
	
	// Finish all rendering commands before cleaning up actors.
	FlushRenderingCommands();
	
	Super::Deinitialize();
}
