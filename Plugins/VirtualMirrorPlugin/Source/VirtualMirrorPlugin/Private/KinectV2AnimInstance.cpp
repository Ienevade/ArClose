#include "KinectV2AnimInstance.h"
#include "KinectV2Device.h"
#include "VirtualMirrorPluginPrivatePCH.h"
#include "IVirtualMirrorPlugin.h"



UKinectV2AnimInstance::UKinectV2AnimInstance(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
	KinectV2Device = IVirtualMirrorPlugin::GetKinectV2DeviceSafe();
}

FVector UKinectV2AnimInstance::GetBonePosition(EJointType JointType) {
	if (!KinectV2Device ||  !KinectV2Device->initiated) return FVector::ZeroVector;
	FVector PositionKinect = KinectV2Device->GetBonePosition(JointType);
	FVector PositionUE4 = FVector(PositionKinect.X, -PositionKinect.Z, PositionKinect.Y) / 10;
	if (!PositionUE4.ContainsNaN()) return PositionUE4;
	return FVector::ZeroVector;
}

FRotator UKinectV2AnimInstance::GetBoneRotation(EJointType JointType) {
	if (!KinectV2Device || !KinectV2Device->initiated) return FRotator::ZeroRotator;
	FRotator rot = KinectV2Device->GetBoneRotation(JointType);
	rot.Normalize();
	if (!rot.Quaternion().ContainsNaN()) return rot;
	return FRotator::ZeroRotator;
	
}

float UKinectV2AnimInstance::GetSensorAngle() {
	if (!KinectV2Device || !KinectV2Device->initiated) return 0;
	return KinectV2Device->SensorAngle;
}

float UKinectV2AnimInstance::GetSensorHeight() {
	if (!KinectV2Device || !KinectV2Device->initiated) return 0;
	return KinectV2Device->SensorHeight*100; //In cm
}

