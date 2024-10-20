#pragma once

#include "RenderGraphUtils.h"

class FNagaVoronoiJfaCompute
{
public:
	// Input Voronoi CellMask Texture : 0->NonCell, 1->Cell
	// Output Voronoi Diagram Texture : RGBA_float, RG->PixelPosition, B->CellMask
	static FRDGTexture* Execute(FRDGBuilder& GraphBuilder, FRDGTexture* seed_buffer, FUintVector2 work_size);
public:
};