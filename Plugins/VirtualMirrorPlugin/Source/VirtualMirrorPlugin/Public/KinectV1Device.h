//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once
#include "VirtualMirrorFunctionLibrary.h"
#include "MotionSensorDevice.h"


//KinectV1 log
DECLARE_LOG_CATEGORY_EXTERN(KinectV1, Log, All);

#define MAX_DEPTH 10000

class FKinectV1Device: public FMotionSensorDevice
{
	
public:
	FKinectV1Device();
	virtual ~FKinectV1Device();

	/** Startup the device, and do any initialization that may be needed */
	bool StartupDevice();

	/** Tear down the device */
	void ShutdownDevice();
	
	/** Update device on each Tick */
	void UpdateDevice(float DeltaTime);
	
	UTexture2D* GetTextureRGB();
	UTexture2D* GetTextureDEPTH();
	UTexture2D* GetTextureUSER();

	FString GetDeviceName();
	

	//Experimental RGB without the user
	void UpdateTextureCLEANRGB();

	bool Init(bool playOni=false);
	void Cleanup(void);

	//Sensor control
	bool SensorInit(bool playOni=false);
	bool SensorShutdown();
	

	//Tracking control
	void AbortTracking();
	bool IsTracking();

	//Skeleton data
	FVector GetBonePosition(EJointType skelJoint, bool flip = false);
	FVector2D GetBonePosition2D(EJointType skelJoint);
	FRotator GetBoneRotation(EJointType skelJoint, bool flip = false);
	EJointType GetClosestBodyJoint(EJointType HandJoint);

	

	
	float						SensorHeight;
	float						SensorAngle;
	FVector						SensorOffset;

	bool						initiated;
	

protected:
	void UpdateTextureRGB();
	void UpdateTextureDEPTH();
	void UpdateTextureUSER();
	
	void UpdateUserTracker();

	nite::JointType ConvertJoint(EJointType Joint);

	

	UTexture2D*					TextureRGB;
	UTexture2D*					TextureDEPTH;
	UTexture2D*					TextureUSER;
	UTexture2D*					DummyTexture;
	
	RGBQUAD*					QuadRGB;
	RGBQUAD*					QuadDEPTH;
	RGBQUAD*					QuadUSER;
	

	
	
	openni::Device				Device;
	FString						DeviceName;

	openni::VideoStream			DepthStream;
	openni::VideoFrameRef		DepthFrame;
	openni::VideoStream			ColorStream;
	openni::VideoFrameRef		ColorFrame;

	nite::UserTracker*			UserTracker;
	nite::UserTrackerFrameRef	UserTrackerFrame;

	float						RefreshTimer;

	bool						FloorFound;
	nite::Plane					SceneFloor;
	FVector						CoM;

	int32						NumberOfUsers;
	int32						TrackedUserID;
	
	bool						Calibrating;
	
	
	
	
	
    
};
