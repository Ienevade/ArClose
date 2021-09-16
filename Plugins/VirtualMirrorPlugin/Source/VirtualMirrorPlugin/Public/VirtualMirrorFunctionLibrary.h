//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once
#include "VirtualMirrorPluginPrivatePCH.h"
#include "VirtualMirrorFunctionLibrary.generated.h"

#define MIRROR_PLUGIN_VERSION "1.1"

UENUM(BlueprintType)
enum class EFrameType : uint8
{
	RGB 	UMETA(DisplayName = "RGB"),
	DEPTH 	UMETA(DisplayName = "DEPTH"),
	USER 	UMETA(DisplayName = "USER"),
	
};

UENUM(BlueprintType)
enum class ESensorType : uint8
{
	KINECT_V1 	UMETA(DisplayName = "Kinect V1.0"),
	KINECT_V2 	UMETA(DisplayName = "Kinect V2.0")
};

UENUM(BlueprintType)
enum class EJointType : uint8
{
	//Basic joint (NITE skeleton)
	Head  UMETA(DisplayName = "Head"),
	Neck  UMETA(DisplayName = "Neck"),
	ShoulderLeft  UMETA(DisplayName = "Shoulder Left"),
	ShoulderRight  UMETA(DisplayName = "Shoulder Right"),
	ElbowLeft  UMETA(DisplayName = "Elbow Left"),
	ElbowRight  UMETA(DisplayName = "Elbow Right"),
	HandLeft  UMETA(DisplayName = "Hand Left"),
	HandRight  UMETA(DisplayName = "Hand Right"),
	SpineBase  UMETA(DisplayName = "Spine Base"),
	HipLeft  UMETA(DisplayName = "Hip Left"),
	HipRight  UMETA(DisplayName = "Hip Right"),
	KneeLeft  UMETA(DisplayName = "Knee Left"),
	KneeRight  UMETA(DisplayName = "Knee Right"),
	FootLeft  UMETA(DisplayName = "Foot Left"),
	FootRight  UMETA(DisplayName = "Foot Right"),
	
	// Additional joints (Kinect V2 skeleton)
	
	SpineMid  UMETA(DisplayName = "Spine Mid"),
	WristLeft UMETA(DisplayName = "Wrist Left"),
	WristRight UMETA(DisplayName = "Wrist Right"),
	AnkleLeft UMETA(DisplayName = "Ankle Left"),
	AnkleRight UMETA(DisplayName = "Ankle Right"),
	SpineShoulder UMETA(DisplayName = "Spine Shoulder"),
	HandTipLeft UMETA(DisplayName = "Hand Tip Left"),
	ThumbLeft UMETA(DisplayName = "Thumb Left"),
	HandTipRight UMETA(DisplayName = "Hand Tip Right"),
	ThumbRight UMETA(DisplayName = "Thumb Right")
};

UCLASS()
class UKinectV1FunctionLibrary : public UBlueprintFunctionLibrary
{

	GENERATED_BODY()

public:
	

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	static void  IntToRGB(int32 integer, int32 &r, int32 &g, int32 &b);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = VirtualMirror)
	static int32 RGBToInt(int32 r, int32 g, int32 b, int32 a);

	static JointType GetParentBone(JointType type);
	static bool JointPositionIsValid(FVector jointPosition);
	static bool BoneOrientationIsValid(FQuat boneOrientation);
	static bool IsTrackedOrInferred(IBody* body, JointType jt);
	static FQuat EnsureQuaternionNeighborhood(FQuat quaternionA, FQuat quaternionB);
	static FQuat EnhancedQuaternionSlerp(FQuat quaternionA, FQuat quaternionB, float amount);
	static FQuat RotationBetweenQuaternions(FQuat quaternionA, FQuat quaternionB);
	static float QuaternionAngle(FQuat rotation);
	static Vector4 UnrealQuaternionToVector4(FQuat quaternion);

};












