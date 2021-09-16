#include "KinectV1AnimInstance.h"
#include "KinectV1Device.h"
#include "VirtualMirrorPluginPrivatePCH.h"
#include "IVirtualMirrorPlugin.h"




UKinectV1AnimInstance::UKinectV1AnimInstance(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
	KinectV1Device = IVirtualMirrorPlugin::GetKinectV1DeviceSafe();
}

FVector UKinectV1AnimInstance::GetBonePosition(EJointType JointType) {
	if (!KinectV1Device || !KinectV1Device->initiated) return FVector::ZeroVector;

	FVector PositionKinect = KinectV1Device->GetBonePosition(JointType);
	FVector PositionUE4 = FVector(PositionKinect.X, -PositionKinect.Z, PositionKinect.Y) / 10;
	if (!PositionUE4.ContainsNaN()) return PositionUE4;
	return FVector::ZeroVector;
}

FRotator UKinectV1AnimInstance::GetBoneRotation(EJointType JointType) {
	if (!KinectV1Device || !KinectV1Device->initiated) return FRotator::ZeroRotator;

	FRotator Rot =  KinectV1Device->GetBoneRotation(JointType);
	if (!Rot.ContainsNaN()) return Rot;
	return FRotator::ZeroRotator;
}

float UKinectV1AnimInstance::GetSensorAngle() {
	if (!KinectV1Device || !KinectV1Device->initiated ) return 0;
	return KinectV1Device->SensorAngle;
}



