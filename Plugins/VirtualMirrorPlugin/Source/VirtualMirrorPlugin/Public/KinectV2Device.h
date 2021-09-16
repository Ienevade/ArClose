//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once
#include "VirtualMirrorFunctionLibrary.h"
#include "MotionSensorDevice.h"
#include "BoneOrientationDoubleExponentialFilter.h"

#include <map>

//KinectV2 log
DECLARE_LOG_CATEGORY_EXTERN(KinectV2, Log, All);

#define MAX_DEPTH 10000

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

class FKinectV2Device: public FMotionSensorDevice, public FRunnable
{
	
public:
	FKinectV2Device();
	virtual ~FKinectV2Device();

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

	//Threading
	void StartThread();
	void StopThread();
	
	virtual uint32 Run() override;
	virtual void Stop() override;


	//Skeleton data
	FVector GetBonePosition(EJointType skelJoint, bool flip = false);
	FVector2D GetBonePosition2D(EJointType skelJoint);
	FRotator GetBoneRotation(EJointType skelJoint, bool flip = false);

	EJointType GetClosestBodyJoint(EJointType HandJoint);
	

	float						SensorHeight;
	float						SensorAngle;
	
	bool						initiated;

	
	

protected:
	void UpdateTextureRGB();
	void UpdateTextureDEPTH();
	void UpdateTextureUSER();
	
	

	bool UpdateMultiFrame();
	void MultiSourceFrameArrived(IMultiSourceFrameArrivedEventArgs* pArgs);
	bool UpdateColorFrame();

	virtual void ProcessFrames(
		const UINT16* pDepthBuffer, int nDepthHeight, int nDepthWidth,
		const BYTE* pBodyIndexBuffer, int nBodyIndexWidth, int nBodyIndexHeight
		);

	virtual void ProcessDepth(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);
	virtual void ProcessInfrared(INT64 nTime, const UINT16* pBuffer, int nHeight, int nWidth);
	virtual void ProcessBody(int nBodyCount, IBody** ppBodies);
	virtual void ProcessFaces(IBody** ppBodies);
	

	JointType ConvertJoint(EJointType Joint);

	// Current Kinect
	IKinectSensor*				KinectSensor;
	ICoordinateMapper*			CoordinateMapper;
	DepthSpacePoint*			DepthCoordinates;
	CameraSpacePoint*			CameraCoordinates;
	ColorSpacePoint*			ColorCoordinates;
	_Vector4					FloorClippingPlane;

	// Multiframe reader except color becasue of 15 FPS framedrop
	IMultiSourceFrameReader*	MultiSourceFrameReader;
	WAITABLE_HANDLE				MultiSourceEventHandle;

	
	//Color
	IColorFrameReader*			ColorFrameReader;

	bool						MultiFrameReady;
	bool						UpdateTexturesReady;
	bool						bIsTracking;
	
	//Joints
	Joint						Joints[JointType_Count];
	Joint						JointsFiltered[JointType_Count];
	JointOrientation			JointOrientationsFiltered[JointType_Count];
	JointOrientation			JointOrientations[JointType_Count];
	
	std::map<uint64, Vector4>	FaceOrientations;
	FString						FaceText;
	

	// Face readers
	IFaceFrameReader*			FaceFrameReaders[BODY_COUNT];

	// Face sources
	IFaceFrameSource*			FaceFrameSources[BODY_COUNT];

	//Kinect 
	int							NumberOfUsers;
	
	ColorSpacePoint				JointsColorSpace[JointType_Count];
	bool						IsOutOfFrame;


	
	FString						DeviceName;
	float						RefreshTimer;
	bool						FloorFound;
	UINT64						PlayerTrackingID;
	bool						Calibrating;

	
	//Textures
	UTexture2D*					TextureRGB;
	UTexture2D*					TextureDEPTH;
	UTexture2D*					TextureUSER;
	UTexture2D*					DummyTexture;
	
	RGBQUAD*					QuadRGB;
	RGBQUAD*					QuadBACKGROUND;
	RGBQUAD*					QuadDEPTH;
	RGBQUAD*					QuadUSER;

	

	//Filters
	BoneOrientationDoubleExponentialFilter* BoneOrientationFilter;

	//Unreal thread
	FRunnableThread*		KinectThread;
	FCriticalSection		ColorCriticalSection;
	bool					bStop;

	

	

};
