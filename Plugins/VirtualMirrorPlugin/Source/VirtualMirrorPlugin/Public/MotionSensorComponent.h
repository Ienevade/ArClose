//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once

#include "VirtualMirrorFunctionLibrary.h"
#include "MotionSensorDevice.h"
#include "MotionSensorComponent.generated.h"





UCLASS(ClassGroup = KinectV1, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UMotionSensorComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	bool Init(ESensorType SensorType, bool OverrideSensorType, int32& ResX, int32& ResY, float& fov, float& WidthMultiplier);

	

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	void Cleanup();

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	UMaterialInstanceDynamic* CreateDynamicMaterialInstance(UStaticMeshComponent* Mesh, UMaterial* SourceMaterial);

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	UMaterialInstanceDynamic* CreateDynamicMaterialInstanceSkeletal(USkeletalMeshComponent* SkeletalMesh, UMaterial* SourceMaterial);

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	void SetTextureParameterValue(UMaterialInstanceDynamic* SourceMaterial, UTexture2D* Texture, FName Param);

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	UTexture2D* GetCameraFrame(EFrameType Type);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FVector GetBonePosition(EJointType JointType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FRotator GetBoneRotation(EJointType JointType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	float GetSensorHeight();

	UFUNCTION(BlueprintCallable,  Category = VirtualMirror)
	FVector SetSensorOffset(FVector SensorOffset);

	UFUNCTION(BlueprintCallable, Category = VirtualMirror)
	float SetHandOcclusionMultiplier(float HandOcclusionMultiplier);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	float GetSensorAngle();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	bool IsTracking();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	ESensorType GetSensorType();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FString GetDeviceName();
	



	
	


	
	


	

	

	
	
	


	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);
	
protected:

	

	ESensorType SensorType;
	FMotionSensorDevice* Device;
};
