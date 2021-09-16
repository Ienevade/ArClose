//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//
#include "MotionSensorComponent.h"
#include "VirtualMirrorPluginPrivatePCH.h"


#include "IVirtualMirrorPlugin.h"

//Sensor devices
#include "KinectV1Device.h"
#include "KinectV2Device.h"






UMotionSensorComponent::UMotionSensorComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Make sure this component ticks
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bAutoActivate = true;



	

	

}



void UMotionSensorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction){
	
	
	if (Device) {
		Device->UpdateDevice(DeltaTime);
	}
	

}


UTexture2D* UMotionSensorComponent::GetCameraFrame(EFrameType Type){
	switch(Type){
		case EFrameType::RGB: return Device->GetTextureRGB(); break;
		case EFrameType::DEPTH: return Device->GetTextureDEPTH(); break;
		case EFrameType::USER: return Device->GetTextureUSER(); break;
		
		
		default: return Device->GetTextureRGB(); break;
	}
}




UMaterialInstanceDynamic* UMotionSensorComponent::CreateDynamicMaterialInstance(UStaticMeshComponent* Mesh, UMaterial* SourceMaterial){
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	Mesh->SetMaterial(0, DynamicMaterial);
	return DynamicMaterial;
}

UMaterialInstanceDynamic* UMotionSensorComponent::CreateDynamicMaterialInstanceSkeletal(USkeletalMeshComponent* SkeletalMesh, UMaterial* SourceMaterial) {
	if (!SkeletalMesh) return nullptr;

	UMaterialInstanceDynamic* DynamicMaterialSkeletal = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	if (!SourceMaterial) return nullptr;

	SkeletalMesh->SetMaterial(0, DynamicMaterialSkeletal);
	return DynamicMaterialSkeletal;
}

void UMotionSensorComponent::SetTextureParameterValue(UMaterialInstanceDynamic* SourceMaterial, UTexture2D* Texture, FName Param){
	if (!SourceMaterial) return;
	if (!Texture) return;

	SourceMaterial->SetTextureParameterValue(Param, Texture);

}

bool UMotionSensorComponent::Init(ESensorType _SensorType, bool OverrideSensorType, int32& ResX, int32& ResY, float& Fov, float& WidthMultiplier) {
	
	
	//Override sensor type from command line
	//Parse command line
	if (OverrideSensorType) {

		UE_LOG(KinectV1, Log, TEXT("Command line: %s"), FCommandLine::GetOriginal());

		TArray<FString> Tokens;
		TArray<FString> Switches;

		FCommandLine::Parse(FCommandLine::GetOriginal(), Tokens, Switches);

		UE_LOG(KinectV1, Log, TEXT("Switches:"));
		for (int i = 0; i < Switches.Num(); i++) {
			UE_LOG(KinectV1, Log, TEXT("%s"), *Switches[i]);
			if (Switches[i] == "KINECT_V1") {
				UE_LOG(KinectV1, Log, TEXT("Kinect V1.0 selected"));
				SensorType = ESensorType::KINECT_V1;
			}
			else if (Switches[i] == "KINECT_V2") {
				UE_LOG(KinectV1, Log, TEXT("Kinect V2.0 selected"));
				SensorType = ESensorType::KINECT_V2;
			}
		}
	}

	//Set sensor type 
	this->SensorType = _SensorType;


	//Set device to the chosen sensor type
	switch (SensorType) {
		case ESensorType::KINECT_V1: {
			Device = (FMotionSensorDevice*)IVirtualMirrorPlugin::GetKinectV1DeviceSafe();
			ResX = 640;
			ResY = 480;
			Fov = 48.6f; //Vertical FOV Kinect V1.0
			WidthMultiplier = 0.94;
			break; 
		}
		case ESensorType::KINECT_V2: {
			Device = (FMotionSensorDevice*)IVirtualMirrorPlugin::GetKinectV2DeviceSafe();
			ResX = 1920;
			ResY = 1080;
			Fov = 53.8f; //Vertical FOV
			WidthMultiplier = 1.05;
			break;
		}
	}

	bool res = Device->Init(false);

	//Check for openni sensor type
	if (Device->GetDeviceName() == "PS1080") {
		UE_LOG(KinectV1, Log, TEXT("PS1080 found, adjusting FOV ..."));
		Fov = 45.0f; //Vertical FOV PrimeSense Carmine 1.08
	}

	return  res;
	
}


void UMotionSensorComponent::Cleanup(){
	if (!Device) return;

	Device->Cleanup();
	Device=nullptr;
}

FVector UMotionSensorComponent::GetBonePosition(EJointType JointType) {
	if (!Device) return FVector::ZeroVector;

	FVector PositionKinect= Device->GetBonePosition(JointType);
	FVector PositionUE4 = FVector(PositionKinect.X, -PositionKinect.Z, PositionKinect.Y) / 10;

	

	return PositionUE4;
	
}
FVector UMotionSensorComponent::SetSensorOffset(FVector SensorOffset) {
	if (!Device) return FVector::ZeroVector;
	Device->SensorOffset = SensorOffset;
	return SensorOffset;
}

float UMotionSensorComponent::SetHandOcclusionMultiplier(float HandOcclusionMultiplier) {
	if (!Device) return 0;
	Device->HandOcclusionMultiplier = HandOcclusionMultiplier;
	return HandOcclusionMultiplier;
}

FRotator UMotionSensorComponent::GetBoneRotation(EJointType JointType) {
	if (!Device) return FRotator::ZeroRotator;
	return Device->GetBoneRotation(JointType);
}

float UMotionSensorComponent::GetSensorHeight() {
	if (!Device) return -1;
	return Device->SensorHeight*100;
}

float UMotionSensorComponent::GetSensorAngle() {
	if (!Device) return -1;
	return Device->SensorAngle;
}

bool UMotionSensorComponent::IsTracking() {
	if (!Device) return false;
	return Device->IsTracking();
}

ESensorType UMotionSensorComponent::GetSensorType() {
	return this->SensorType;
}

FString UMotionSensorComponent::GetDeviceName() {
	if (!Device) return "No sensor";
	return Device->GetDeviceName();
}



