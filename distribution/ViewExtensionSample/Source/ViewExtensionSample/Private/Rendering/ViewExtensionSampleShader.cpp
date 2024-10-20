

#include "../../Public/Rendering/ViewExtensionSampleShader.h"

IMPLEMENT_GLOBAL_SHADER(FPostBasePassModifyGBufferVs, "/ViewExtensionSampleShaders/Private/resolve_hack_shading_model.usf", "Pass0_VS", SF_Vertex );
IMPLEMENT_GLOBAL_SHADER(FPostBasePassModifyGBufferPs, "/ViewExtensionSampleShaders/Private/resolve_hack_shading_model.usf", "Pass0_PS", SF_Pixel );

IMPLEMENT_GLOBAL_SHADER(FImageProcessTestCS, "/ViewExtensionSampleShaders/Private/voronoi_postprocess_test.usf", "GenVoronoiEdgeCS", SF_Compute );
IMPLEMENT_GLOBAL_SHADER(FTestFinalCS, "/ViewExtensionSampleShaders/Private/voronoi_postprocess_test.usf", "VoronoiPostprocessCS", SF_Compute );


IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaEigenvectorCS, "/ViewExtensionSampleShaders/Private/aniso_kuwahara.usf", "EigenvectorPass_CS", SF_Compute );
IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaBlurCS, "/ViewExtensionSampleShaders/Private/aniso_kuwahara.usf", "BlurPass_CS", SF_Compute );
IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaCalcAnisoCS, "/ViewExtensionSampleShaders/Private/aniso_kuwahara.usf", "CalcAnisoPass_CS", SF_Compute );
IMPLEMENT_GLOBAL_SHADER(FAnisoKuwaharaFinalCS, "/ViewExtensionSampleShaders/Private/aniso_kuwahara.usf", "FinalPass_CS", SF_Compute );


IMPLEMENT_GLOBAL_SHADER(FLensGhostExtractBrightCS, "/ViewExtensionSampleShaders/Private/LensGhost/extract_bright.usf", "MainCS", SF_Compute );
IMPLEMENT_GLOBAL_SHADER(FLensGhostGenerateCS, "/ViewExtensionSampleShaders/Private/LensGhost/generate_ghost.usf", "MainCS", SF_Compute );
IMPLEMENT_GLOBAL_SHADER(FLensGhostCompositeCS, "/ViewExtensionSampleShaders/Private/LensGhost/composite.usf", "MainCS", SF_Compute );


IMPLEMENT_GLOBAL_SHADER(FHistoryTestCS, "/ViewExtensionSampleShaders/Private/history_test.usf", "MainCS", SF_Compute );









