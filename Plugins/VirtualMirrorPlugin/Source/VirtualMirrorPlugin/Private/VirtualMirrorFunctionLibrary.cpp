//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#include "VirtualMirrorFunctionLibrary.h"
#include "VirtualMirrorPluginPrivatePCH.h"






void  UKinectV1FunctionLibrary::IntToRGB(int32 integer, int32 &r, int32 &g, int32 &b) {

	b = integer & 255;
	g = (integer >> 8) & 255;
	r = (integer >> 16) & 255;

}

int32  UKinectV1FunctionLibrary::RGBToInt(int32 r, int32 g, int32 b, int32 a) {
	int integer = (256 * 256 * r) + (256 * g) + b;

	if (a == 0) return integer;
	if (a == 1) return -integer;

	return 0;
}

JointType UKinectV1FunctionLibrary::GetParentBone(JointType type)
{
	JointType parent = JointType_SpineBase;
	switch (type) {

		//
		// Spine from spine base up to head.
		// NOTE: SpineBase is the only joint without a parent.
		//
	case JointType_SpineBase: parent = JointType_SpineBase; break;
	case JointType_SpineMid: parent = JointType_SpineBase; break;
	case JointType_SpineShoulder: parent = JointType_SpineMid; break;
	case JointType_Neck: parent = JointType_SpineShoulder; break;
	case JointType_Head: parent = JointType_Neck; break;

		//
		// Left arm from shoulder-level spine.
		//
	case JointType_ShoulderLeft: parent = JointType_SpineShoulder; break;
	case JointType_ElbowLeft: parent = JointType_ShoulderLeft; break;
	case JointType_WristLeft: parent = JointType_ElbowLeft; break;
	case JointType_HandLeft: parent = JointType_WristLeft; break;
	case JointType_HandTipLeft: parent = JointType_HandLeft; break;
	case JointType_ThumbLeft: parent = JointType_HandLeft; break;

		//
		// Right arm from shoulder-level spine.
		//
	case JointType_ShoulderRight: parent = JointType_SpineShoulder; break;
	case JointType_ElbowRight: parent = JointType_ShoulderRight; break;
	case JointType_WristRight: parent = JointType_ElbowRight; break;
	case JointType_HandRight: parent = JointType_WristRight; break;
	case JointType_HandTipRight: parent = JointType_HandRight; break;
	case JointType_ThumbRight: parent = JointType_HandRight; break;

		//
		// Left leg from spine base down.
		//
	case JointType_HipLeft: parent = JointType_SpineBase; break;
	case JointType_KneeLeft: parent = JointType_HipLeft; break;
	case JointType_AnkleLeft: parent = JointType_KneeLeft; break;
	case JointType_FootLeft: parent = JointType_AnkleLeft; break;

		//
		// Right leg from spine base down.
		//
	case JointType_HipRight: parent = JointType_SpineBase; break;
	case JointType_KneeRight: parent = JointType_HipRight; break;
	case JointType_AnkleRight: parent = JointType_KneeRight; break;
	case JointType_FootRight: parent = JointType_AnkleRight; break;
	}
	return parent;
}

bool UKinectV1FunctionLibrary::JointPositionIsValid(FVector jointPosition)
{
	return !(isnan(jointPosition.X) || isnan(jointPosition.Y) || isnan(jointPosition.Z));
}

bool UKinectV1FunctionLibrary::BoneOrientationIsValid(FQuat boneOrientation)
{
	return !(isnan(boneOrientation.X) || isnan(boneOrientation.Y) || isnan(boneOrientation.Z) || isnan(boneOrientation.W));
}

bool UKinectV1FunctionLibrary::IsTrackedOrInferred(IBody* body, JointType jt)
{
	if (!body)
	{
		return false;
	}
	Joint joints[JointType_Count];
	HRESULT hr = body->GetJoints(JointType_Count, joints);
	if (SUCCEEDED(hr)) {
		return joints[jt].TrackingState != TrackingState_NotTracked;
	}
	else {
		return false;
	}
}

FQuat UKinectV1FunctionLibrary::EnsureQuaternionNeighborhood(FQuat quaternionA, FQuat quaternionB)
{


	float dot = quaternionA.X * quaternionB.X + quaternionA.Y * quaternionB.Y + quaternionA.Z * quaternionB.Z + quaternionA.W * quaternionB.W;


	if (dot < 0)
	{
		// Negate the second quaternion, to place it in the opposite 3D sphere.
		return FQuat(-quaternionB.X, -quaternionB.Y, -quaternionB.Z, -quaternionB.W);
	}


	return quaternionB;
}

FQuat UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(FQuat quaternionA, FQuat quaternionB, float amount)
{
	FQuat modifiedB = EnsureQuaternionNeighborhood(quaternionA, quaternionB);

	return FQuat::Slerp(quaternionA, modifiedB, amount);
}

FQuat UKinectV1FunctionLibrary::RotationBetweenQuaternions(FQuat quaternionA, FQuat quaternionB)
{
	FQuat modifiedB = EnsureQuaternionNeighborhood(quaternionA, quaternionB);
	return quaternionA.Inverse()*modifiedB;
}

float UKinectV1FunctionLibrary::QuaternionAngle(FQuat rotation)
{
	float angle = 2.0f * (float)FMath::Acos(rotation.W);
	return angle;
}

Vector4 UKinectV1FunctionLibrary::UnrealQuaternionToVector4(FQuat quaternion) {
	Vector4 vec;
	vec.x = quaternion.X;
	vec.y = quaternion.Y;
	vec.z = quaternion.Z;
	vec.w = quaternion.W;
	return vec;
}




