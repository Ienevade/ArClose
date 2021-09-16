//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once

#include "VirtualMirrorFunctionLibrary.h"
#include "KinectAnimInstance.h"
#include "KinectV2AnimInstance.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class UKinectV2AnimInstance : public  UKinectAnimInstance
{

	GENERATED_UCLASS_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FVector GetBonePosition(EJointType JointType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FRotator GetBoneRotation(EJointType JointType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	float GetSensorAngle();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	float GetSensorHeight();


	
								

protected:

	FKinectV2Device* KinectV2Device = nullptr;

private:

	



};


