//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once

#include "VirtualMirrorFunctionLibrary.h"
#include "PhotoComponent.generated.h"

//KinectV2 log
DECLARE_LOG_CATEGORY_EXTERN(VirtualMirror, Log, All);

UCLASS(ClassGroup = KinectV1, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UPhotoComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	UTexture2D* LoadTexture2DFromFile(const FString& FullFilePath, bool& IsValid, int32& Width, int32& Height);

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	FString TakePhoto();

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	void PostPhotoToFB(FString PhotoFileName, FString Url, FString Token, FString Message);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);
	
protected:

	void PostPhotoToFBCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

};
