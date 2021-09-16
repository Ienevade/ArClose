//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once

#include "Animation/AnimInstance.h"
#include "KinectAnimInstance.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class UKinectAnimInstance : public UAnimInstance
{

	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimParams)
		float FootOffset;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimParams)
		float PelvisOffset;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimParams)
		float ScaleWidth;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimParams)
		FVector OffsetVectorNeck;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimParams)
		FVector OffsetVectorHip;






protected:

private:

	



};


