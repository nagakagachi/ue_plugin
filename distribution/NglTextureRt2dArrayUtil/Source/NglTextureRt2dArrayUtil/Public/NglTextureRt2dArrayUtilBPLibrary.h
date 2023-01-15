// Copyright Taro Horikawa, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "NglTextureRt2dArrayUtilBPLibrary.generated.h"

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class UNglTextureRt2dArrayUtilBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()


	// Performs a DrawMaterial on the specified TextureArray Slice. This method creates a Temporal Texture and executes CopyTexture.
	// RenderTargetArrayの指定したSliceに対してDrawMaterialToRenderTargetを実行します. 内部処理に一時リソース生成やテクスチャコピーが含まれます.
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Ngl Draw Material To Texture RenderTarget Array Slice", Keywords = "NglTextureRt2dArrayUtil NglDrawMaterialToRenderTargetArraySlice", WorldContext = "WorldContextObject"), Category = "NglTextureRt2dArrayUtil")
	static void NglDrawMaterialToRenderTargetArraySlice(UObject* WorldContextObject, UMaterialInterface* Material, class UTextureRenderTarget2DArray* TextureRt2dArray, int SliceIndex = 0);

};
