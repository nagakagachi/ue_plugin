// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewExtensionSample.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FViewExtensionSampleModule"

void FViewExtensionSampleModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// PluguinのShadersディレクトリを ViewExtensionSampleShaders としてマッピング.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ViewExtensionSample"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/ViewExtensionSampleShaders"), PluginShaderDir);
	
}

void FViewExtensionSampleModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FViewExtensionSampleModule, ViewExtensionSample)