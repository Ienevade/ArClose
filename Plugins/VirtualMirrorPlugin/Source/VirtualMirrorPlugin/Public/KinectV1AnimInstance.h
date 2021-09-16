//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once

#include "VirtualMirrorFunctionLibrary.h"
#include "KinectAnimInstance.h"
#include "KinectV1AnimInstance.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class UKinectV1AnimInstance : public  UKinectAnimInstance
{

	GENERATED_UCLASS_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FVector GetBonePosition(EJointType JointType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	FRotator GetBoneRotation(EJointType JointType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	float GetSensorAngle();

	

	
								

protected:

	FKinectV1Device* KinectV1Device = nullptr;
	

private:

	



};


