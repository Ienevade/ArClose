//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once
#include <map>


class FMotionSensorDevice 
{
	
public:
	virtual ~FMotionSensorDevice() {};

	/** Startup the device, and do any initialization that may be needed */
	virtual bool StartupDevice()=0;

	/** Tear down the device */
	virtual void ShutdownDevice()=0;
	
	/** Update device on each Tick */
	virtual void UpdateDevice(float DeltaTime)=0;
	
	virtual UTexture2D* GetTextureRGB()=0;
	virtual UTexture2D* GetTextureDEPTH()=0;
	virtual UTexture2D* GetTextureUSER()=0;

	virtual FString GetDeviceName()=0;
	

	virtual bool Init(bool playOni=false) = 0;
	virtual void Cleanup(void)=0;

	//Sensor control
	virtual bool SensorInit(bool playOni=false)=0;
	virtual bool SensorShutdown()=0;
	

	//Tracking control
	virtual void AbortTracking()=0;
	virtual bool IsTracking()=0;

	//Skeleton data
	virtual FVector GetBonePosition(EJointType skelJoint, bool flip = false)=0;
	virtual FVector2D GetBonePosition2D(EJointType skelJoint)=0;
	virtual FRotator GetBoneRotation(EJointType skelJoint, bool flip = false)=0;
	

	

	
	float						SensorHeight;
	float						SensorAngle;
	FVector						SensorOffset;
	float						HandOcclusionMultiplier;
	

protected:
	virtual void UpdateTextureRGB() = 0;
	virtual void UpdateTextureDEPTH()=0;
	virtual void UpdateTextureUSER()=0;
	
	

	std::map<EJointType, FRotator>	JointRotationsValid;
	std::map<EJointType, FVector>	JointPositionsValid;

	FString DeviceName;
};