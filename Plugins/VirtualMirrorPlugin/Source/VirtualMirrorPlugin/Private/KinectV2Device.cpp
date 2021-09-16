//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//
#include "KinectV2Device.h"
#include "VirtualMirrorPluginPrivatePCH.h"



#include <string>
#include <iostream>
#include <sstream>

//General Log
DEFINE_LOG_CATEGORY(KinectV2);

#include "Core.h"

FKinectV2Device::FKinectV2Device()
{
	initiated = false;
	DepthCoordinates = NULL;
	QuadRGB = NULL;
	QuadBACKGROUND = NULL;
	QuadDEPTH = NULL;
	QuadUSER = NULL;
	KinectSensor = NULL;
	MultiSourceFrameReader = NULL;
	ColorFrameReader = NULL;
	CoordinateMapper = NULL;
	PlayerTrackingID = 0;
	SensorOffset = FVector(0, 0, 0);
	HandOcclusionMultiplier = 1.0;
	bIsTracking = false;
	KinectThread = NULL;
	MultiSourceEventHandle = NULL;
		
	for (int i = 0; i < BODY_COUNT; i++)
	{
		FaceFrameSources[i] = NULL;
		FaceFrameReaders[i] = NULL;
	}

	
	BoneOrientationFilter = 0;
	DeviceName = "Kinect v2.0";

	UE_LOG(KinectV2, Log, TEXT("Device start"));
}

FKinectV2Device::~FKinectV2Device()
{
	UE_LOG(KinectV2, Log, TEXT("Device shutdown"));
    Cleanup();
}

bool FKinectV2Device::StartupDevice()
{
	return true;
}

uint32 FKinectV2Device::Run() {

	while (!bStop) {


		if (!MultiSourceFrameReader)
		{
			return 0;
		}

		HANDLE handles[] = { reinterpret_cast<HANDLE>(MultiSourceEventHandle) };
		int idx;
		// Wait for any of the events to be signalled
		idx = WaitForMultipleObjects(1, handles, false, 0);
		switch (idx) {
		case WAIT_TIMEOUT:
			continue;
		case 0:
			IMultiSourceFrameArrivedEventArgs *pFrameArgs = nullptr;
			HRESULT hr = MultiSourceFrameReader->GetMultiSourceFrameArrivedEventData(MultiSourceEventHandle, &pFrameArgs);
			MultiSourceFrameArrived(pFrameArgs);

			break;
		}

	}

	return 0;
}

void FKinectV2Device::Stop() {

	bStop = true;


}

UTexture2D* FKinectV2Device::GetTextureRGB(){
	if (this->initiated) {
		return this->TextureRGB;
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("KinectV2 is not initialized properly. (RGB)")));
		}
		return this->DummyTexture;
	}
}



UTexture2D* FKinectV2Device::GetTextureDEPTH() {
	if (this->initiated) {
		return this->TextureDEPTH;
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("KinectV2 is not initialized properly. (DEPTH)")));
		}
		return this->DummyTexture;
	}
}

UTexture2D* FKinectV2Device::GetTextureUSER() {
	if (this->initiated) {
		return this->TextureUSER;
	}
	else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("KinectV2 is not initialized properly. (USER)")));
		}
		return this->DummyTexture;
	}
}


void FKinectV2Device::ShutdownDevice()
{
	Cleanup();
}

void FKinectV2Device::UpdateDevice(float DeltaTime) {
	

		//Read color frame only
		if (UpdateColorFrame()) {
			UpdateTextureRGB();
		}

		//If frames are ready update textures
		if (MultiFrameReady) {
			UpdateTextureDEPTH();
			UpdateTextureUSER();
			MultiFrameReady = false;
		}
	
}

void FKinectV2Device::StartThread() {
	if (KinectThread == nullptr) {
		KinectThread = FRunnableThread::Create(this, TEXT("FKinectThread"), 0, EThreadPriority::TPri_AboveNormal);
		UE_LOG(KinectV2, Log, TEXT("Kinect thread started"));
	}

}

void FKinectV2Device::StopThread() {
	Stop();
	if (KinectThread) {

		KinectThread->Kill(true);
		delete KinectThread;
	}
	KinectThread = nullptr;
	UE_LOG(KinectV2, Log, TEXT("Kinect thread stopped"));

}




void FKinectV2Device::UpdateTextureRGB(){
	
	if(QuadRGB){
		const size_t SizeQuadRGB = 1920 * 1080 * sizeof(RGBQUAD);
		uint8* Dest = (uint8*)TextureRGB->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, (uint8*)QuadRGB, SizeQuadRGB);

		TextureRGB->PlatformData->Mips[0].BulkData.Unlock();
		TextureRGB->UpdateResource();
		
	}

}

void FKinectV2Device::UpdateTextureDEPTH() {
	if (QuadDEPTH) {
		const size_t SizeQuadDEPTH = 1920 * 1080 * sizeof(RGBQUAD);
		uint8* Dest = (uint8*)TextureDEPTH->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, (uint8*)QuadDEPTH, SizeQuadDEPTH);

		TextureDEPTH->PlatformData->Mips[0].BulkData.Unlock();
		TextureDEPTH->UpdateResource();

	}

}

void FKinectV2Device::UpdateTextureUSER()
{
	if (QuadUSER) {
		const size_t SizeQuadUSER = 512 * 424 * sizeof(RGBQUAD);
		uint8* Dest = (uint8*)TextureUSER->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, (uint8*)QuadUSER, SizeQuadUSER);
		TextureUSER->PlatformData->Mips[0].BulkData.Unlock();
		TextureUSER->UpdateResource();

	}

	
}


bool FKinectV2Device::Init(bool playOni){
	RefreshTimer = 0;

	// create heap storage for the coorinate mapping from color to depth
	DepthCoordinates = new DepthSpacePoint[1920 * 1080];

	//Defaults	
	
	FloorFound = false;
	Calibrating = false;
	SensorAngle = 0;
	SensorHeight = 0;
	
	//Color texture
	TextureRGB = UTexture2D::CreateTransient(1920,1080);
	TextureRGB->SRGB = 1;
	TextureRGB->UpdateResource();

	//Depth texture
	TextureDEPTH = UTexture2D::CreateTransient(1920, 1080);
	TextureDEPTH->SRGB = 0;
	TextureDEPTH->UpdateResource();

	//Texture user
	TextureUSER = UTexture2D::CreateTransient(512, 424);
	TextureUSER->SRGB = 1;
	TextureUSER->UpdateResource();

	
	//Return this if WebcamTexture cannot be created by any reason.
	DummyTexture = UTexture2D::CreateTransient(1920,1080);
	DummyTexture->SRGB = 1;
	DummyTexture->UpdateResource();

	//Create rgbquads for holding texture data
	QuadRGB = new RGBQUAD[1920*1080];
	QuadBACKGROUND = new RGBQUAD[1920 * 1080];
	QuadDEPTH = new RGBQUAD[1920*1080];
	QuadUSER = new RGBQUAD[512*424];

	//Filter
	BoneOrientationFilter = new BoneOrientationDoubleExponentialFilter();
	
	//Init sensor 
	HRESULT hr;

	hr = GetDefaultKinectSensor(&KinectSensor);
	if (FAILED(hr))
	{
		return false;
	}
	
	if(KinectSensor)
	{
		// Initialize the Kinect and get coordinate mapper and the frame reader

		if (SUCCEEDED(hr))
		{
			hr = KinectSensor->get_CoordinateMapper(&CoordinateMapper);
		}
		else {
			return false;
		}

		hr = KinectSensor->Open();

		//Multi Source Frame Reader
		if (SUCCEEDED(hr))
		{
			hr = KinectSensor->OpenMultiSourceFrameReader(
				FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_BodyIndex | FrameSourceTypes::FrameSourceTypes_Body | FrameSourceTypes::FrameSourceTypes_Infrared,
				&MultiSourceFrameReader);

			
		}
		else {
			return false;
		}

		if (SUCCEEDED(hr)) {
			MultiSourceFrameReader->SubscribeMultiSourceFrameArrived(&MultiSourceEventHandle);
		}

		//Color frame reader (cannot use multi frame becasue of the 15FPS color framedrop)
		IColorFrameSource* ColorFrameSource = NULL;
		if (SUCCEEDED(hr))
		{
			hr = KinectSensor->get_ColorFrameSource(&ColorFrameSource);
		}
		else {
			return false;
		}

		if (SUCCEEDED(hr))
		{
			hr = ColorFrameSource->OpenReader(&ColorFrameReader);
		} else {
			return false;
		}

		SafeRelease(ColorFrameSource);


		//Start face
		if (SUCCEEDED(hr))
		{
#include "Windows/AllowWindowsPlatformTypes.h" 
			static const DWORD c_FaceFrameFeatures =
				FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
				| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
				| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
				| FaceFrameFeatures::FaceFrameFeatures_Happy
				| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
				| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
				| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
				| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
				| FaceFrameFeatures::FaceFrameFeatures_LookingAway
				| FaceFrameFeatures::FaceFrameFeatures_Glasses
				| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;
#include "Windows/HideWindowsPlatformTypes.h"

			// create a face frame source + reader to track each body in the fov
			for (int i = 0; i < BODY_COUNT; i++)
			{
				if (SUCCEEDED(hr))
				{
					// create the face frame source by specifying the required face frame features
					// define the face frame features required to be computed by this application

					hr = CreateFaceFrameSource(KinectSensor, 0, c_FaceFrameFeatures, &FaceFrameSources[i]);
				}
				if (SUCCEEDED(hr))
				{
					// open the corresponding reader
					hr = FaceFrameSources[i]->OpenReader(&FaceFrameReaders[i]);
					if (SUCCEEDED(hr))
					{
						UE_LOG(KinectV2, Log, TEXT("Face frame reader opened for: %d"), i);
						//Subscribe for arrived frame
					}
					else {
						UE_LOG(KinectV2, Error, TEXT("Face frame reader NOT opened for: %d"), i);
					}
				}
				else {
					UE_LOG(KinectV2, Error, TEXT("Face source could not created for: %d"), i);
				}
			}
		}
		//End face reader
	}

	if (!KinectSensor || FAILED(hr))
	{
		return false;
	}
	/*
	BOOLEAN isAvailable = false;
	KinectSensor->get_IsAvailable(&isAvailable);

	if (!isAvailable) {
		UE_LOG(KinectV2, Error, TEXT("Sensor not connected."));
		return false;
	}
	*/
	initiated = 1;
	bStop = false; //Start thread
	
	StartThread();

	UE_LOG(KinectV2, Log, TEXT("Init complete !!\n"));

	return 1; //Succesfull init
}

bool FKinectV2Device::SensorInit(bool playOni) {
	return Init(playOni);
}

void FKinectV2Device::AbortTracking() {

}

bool  FKinectV2Device::IsTracking()
{
	
	if (PlayerTrackingID != 0) return true;
	return false;
}

FRotator FKinectV2Device::GetBoneRotation(EJointType skelJoint, bool flip)
{
	FQuat newQ = FRotator::ZeroRotator.Quaternion();
	if (Joints[ConvertJoint(skelJoint)].TrackingState == TrackingState_Tracked) {
		Vector4 jointOri = JointOrientations[ConvertJoint(skelJoint)].Orientation;
		

		//TO DO: Check confidence value!
		newQ.X = jointOri.x;
		newQ.Y = -jointOri.z;
		newQ.Z = jointOri.y;
		newQ.W = jointOri.w;


		newQ = newQ*FRotator(90, 180, 0).Quaternion();
	}
	//Try to return valid rotation only if not found check previous data
	if (newQ != FRotator::ZeroRotator.Quaternion()) {
		JointRotationsValid[skelJoint] = newQ.Rotator();
	}
	else {
		if (JointRotationsValid.find(skelJoint) == JointRotationsValid.end()) {
			// not found
			return FRotator(newQ); //No valid data
		}
		else {
			// found
			return JointRotationsValid.find(skelJoint)->second;
		}
	}

	return newQ.Rotator();;
}

FVector2D FKinectV2Device::GetBonePosition2D(EJointType skelJoint)
{
	ColorSpacePoint ColorPoint;
	CameraSpacePoint SkeletonPoint;

	Joint joint = Joints[ConvertJoint(skelJoint)];
	SkeletonPoint = joint.Position;

	// Skeleton-to-Color mapping
	CoordinateMapper->MapCameraPointToColorSpace(SkeletonPoint, &ColorPoint);

	return FVector2D(ColorPoint.X, ColorPoint.Y);
}

FVector FKinectV2Device::GetBonePosition(EJointType skelJoint, bool flip)
{
	FVector newPos = FVector::ZeroVector;

	CameraSpacePoint JointPosition = Joints[ConvertJoint(skelJoint)].Position;
	
	if(Joints[ConvertJoint(skelJoint)].TrackingState!=TrackingState_NotTracked){
		newPos = FVector(JointPosition.X, JointPosition.Y, JointPosition.Z)*1000;
		
		
		return newPos + (SensorOffset*10);

	}

	//Try to return valid rotation only if not found check previous data
	if (newPos != FVector::ZeroVector) {
		JointPositionsValid[skelJoint] = newPos;
	}
	else {
		if (JointPositionsValid.find(skelJoint) == JointPositionsValid.end()) {
			// not found
			return newPos; //No valid data
		}
		else {
			// found
			return JointPositionsValid.find(skelJoint)->second+(SensorOffset * 10);
		}
	}



	

	return newPos; 
}


bool FKinectV2Device::SensorShutdown() {
	StopThread();
	
	if (KinectThread) {

		KinectThread->Kill(true);
		delete KinectThread;
	}



	if (DepthCoordinates)
	{
		delete[] DepthCoordinates;
		DepthCoordinates = NULL;
	}

	
	if(KinectSensor) KinectSensor->Close();
	
	if(MultiSourceFrameReader){
		MultiSourceFrameReader->UnsubscribeMultiSourceFrameArrived(MultiSourceEventHandle);
		MultiSourceEventHandle = NULL;
	}

	SafeRelease(MultiSourceFrameReader);
	SafeRelease(ColorFrameReader);

	// done with face sources and readers
	for (int i = 0; i < BODY_COUNT; i++)
	{
		if(FaceFrameSources[i]) SafeRelease(FaceFrameSources[i]);
		if(FaceFrameReaders[i]) SafeRelease(FaceFrameReaders[i]);
	}



	SafeRelease(CoordinateMapper);
	SafeRelease(KinectSensor);
	
	KinectSensor = nullptr;
	MultiSourceFrameReader = nullptr;
	ColorFrameReader = nullptr;
	
	return true;
}



void FKinectV2Device::Cleanup(void){
	//Shutdown sensor
	bIsTracking = false;
	SensorShutdown();
	

	
	//Delete RGB quad
	if (QuadRGB)
	{
		delete[] QuadRGB;
		QuadRGB = NULL;
	}

	//Delete BACKGROUND quad
	if (QuadBACKGROUND)
	{
		delete[] QuadBACKGROUND;
		QuadBACKGROUND = NULL;
	}

	//Delete DEPTH quad
	if (QuadDEPTH)
	{
		delete[] QuadRGB;
		QuadRGB = NULL;
	}

	//Delete USER quad
	if (QuadUSER)
	{
		delete[] QuadUSER;
		QuadUSER = NULL;
	}

	//Filter
	if (BoneOrientationFilter) {
		delete BoneOrientationFilter;
		BoneOrientationFilter = 0;
	}
	initiated = false;
	

	UE_LOG(KinectV2, Log, TEXT("Cleanup ready !!\n"));
}

bool FKinectV2Device::UpdateColorFrame() {
	IColorFrame* ColorFrame = NULL;
	IFrameDescription* pColorFrameDescription = NULL;
	int nColorWidth = 0;
	int nColorHeight = 0;
	ColorImageFormat imageFormat = ColorImageFormat_None;
	unsigned int nColorBufferSize = 0;
	RGBQUAD *pColorBuffer = NULL;

	HRESULT hr = ColorFrameReader->AcquireLatestFrame(&ColorFrame);

	// get color frame data
	
	if (SUCCEEDED(hr))
	{
		hr = ColorFrame->get_FrameDescription(&pColorFrameDescription);
	}
	else {
		return false;
	}

	if (SUCCEEDED(hr))
	{
		hr = pColorFrameDescription->get_Width(&nColorWidth);
	}

	if (SUCCEEDED(hr))
	{
		hr = pColorFrameDescription->get_Height(&nColorHeight);
	}

	if (SUCCEEDED(hr))
	{
		hr = ColorFrame->get_RawColorImageFormat(&imageFormat);
	}

	if (SUCCEEDED(hr))
	{

		if (imageFormat == ColorImageFormat_Bgra){
			hr = ColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
		} else if (QuadRGB){
			pColorBuffer =QuadRGB;
			nColorBufferSize = 1920 * 1080 * sizeof(RGBQUAD);
			hr = ColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Bgra);
		} else {
			hr = E_FAIL;
		}
	}
	
	SafeRelease(pColorFrameDescription);
	SafeRelease(ColorFrame);

	return true;
}

void FKinectV2Device::MultiSourceFrameArrived(IMultiSourceFrameArrivedEventArgs* pArgs){

	IMultiSourceFrame* MultiSourceFrame = NULL;
	IDepthFrame* DepthFrame = NULL;

	IInfraredFrame* InfraredFrame = NULL;
	IBodyIndexFrame* BodyIndexFrame = NULL;
	IBodyFrame* BodyFrame = NULL;

	HRESULT hr = MultiSourceFrameReader->AcquireLatestFrame(&MultiSourceFrame);

	if (SUCCEEDED(hr))
	{
		IDepthFrameReference* DepthFrameReference = NULL;

		hr = MultiSourceFrame->get_DepthFrameReference(&DepthFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = DepthFrameReference->AcquireFrame(&DepthFrame);
		}

		SafeRelease(DepthFrameReference);
	}

	if (SUCCEEDED(hr))
	{
		IInfraredFrameReference* InfraredFrameReference = NULL;

		hr = MultiSourceFrame->get_InfraredFrameReference(&InfraredFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = InfraredFrameReference->AcquireFrame(&InfraredFrame);
		}

		SafeRelease(InfraredFrameReference);
	}
	

	if (SUCCEEDED(hr))
	{
		IBodyIndexFrameReference* BodyIndexFrameReference = NULL;

		hr = MultiSourceFrame->get_BodyIndexFrameReference(&BodyIndexFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = BodyIndexFrameReference->AcquireFrame(&BodyIndexFrame);
		}

		SafeRelease(BodyIndexFrameReference);
	}
	
		if (SUCCEEDED(hr))
		{
			IBodyFrameReference* BodyFrameReference = NULL;

			hr = MultiSourceFrame->get_BodyFrameReference(&BodyFrameReference);
			if (SUCCEEDED(hr))
			{
				hr = BodyFrameReference->AcquireFrame(&BodyFrame);
			}

			SafeRelease(BodyFrameReference);
		}


		if (SUCCEEDED(hr))
		{
			IBody* ppBodies[BODY_COUNT] = { 0 };

			if (SUCCEEDED(hr))
			{
				hr = BodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
			}

			if (SUCCEEDED(hr))
			{
				ProcessFaces(ppBodies);
				ProcessBody(BODY_COUNT, ppBodies); //only process body 1


				//Floor clipping plane
				hr = BodyFrame->get_FloorClipPlane(&FloorClippingPlane);

				//Sensor height / tilt
				FVector floorNormal = FVector(FloorClippingPlane.x, FloorClippingPlane.y, FloorClippingPlane.z);

				float lenProduct = floorNormal.Size()* FVector::UpVector.Size();

				float f = FVector::DotProduct(floorNormal, FVector::UpVector) / lenProduct;
				f = FMath::Clamp(f, (float)-1.0, (float)1.0);
				float tilt = 90 - FMath::RadiansToDegrees(FMath::Acos(f));
				this->SensorAngle = tilt;

				float sensorHeightD = (-floorNormal.Z)*FMath::Sin(tilt);
				float sensorHeight = abs(this->FloorClippingPlane.w) + sensorHeightD;
				this->SensorHeight = sensorHeight;

			}

			for (int i = 0; i < _countof(ppBodies); ++i)
			{
				SafeRelease(ppBodies[i]);
			}
		}
		SafeRelease(BodyFrame);
	





	if (SUCCEEDED(hr))
	{
		INT64 nTime = 0;
		IFrameDescription* InfraredFrameDescription = NULL;
		int nInfraredWidth = 0;
		int nInfraredHeight = 0;
		unsigned int nInfraredBufferSize = 0;
		UINT16 *InfraredBuffer = NULL;

		hr = InfraredFrame->get_RelativeTime(&nTime);

		if (SUCCEEDED(hr))
		{
			hr = InfraredFrame->get_FrameDescription(&InfraredFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = InfraredFrameDescription->get_Width(&nInfraredWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = InfraredFrameDescription->get_Height(&nInfraredHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = InfraredFrame->AccessUnderlyingBuffer(&nInfraredBufferSize, &InfraredBuffer);
		}

		if (SUCCEEDED(hr))
		{
			//ProcessInfrared(nTime, InfraredBuffer, nInfraredHeight, nInfraredWidth);
		}

		SafeRelease(InfraredFrameDescription);
	}


	if (SUCCEEDED(hr))
	{
		INT64 nDepthTime = 0;
		IFrameDescription* pDepthFrameDescription = NULL;
		int nDepthWidth = 0;
		int nDepthHeight = 0;
		USHORT nDepthMinReliableDistance = 0;
		USHORT nDepthMaxDistance = 0;
		unsigned int nDepthBufferSize = 0;
		UINT16 *pDepthBuffer = NULL;

		

		IFrameDescription* pBodyIndexFrameDescription = NULL;
		int nBodyIndexWidth = 0;
		int nBodyIndexHeight = 0;
		unsigned int nBodyIndexBufferSize = 0;
		BYTE *pBodyIndexBuffer = NULL;

		// get depth frame data

		hr = DepthFrame->get_RelativeTime(&nDepthTime);



		if (SUCCEEDED(hr))
		{
			hr = DepthFrame->get_FrameDescription(&pDepthFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameDescription->get_Width(&nDepthWidth);
			
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameDescription->get_Height(&nDepthHeight);
		}
		if (SUCCEEDED(hr))
		{
			hr = DepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);

		}

		if (SUCCEEDED(hr))
		{
			// In order to see the full range of depth (including the less reliable far field depth)
			// we are setting nDepthMaxDistance to the extreme potential depth threshold
			//nDepthMaxDistance = USHRT_MAX;

			// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
			hr = DepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);

		}

		if (SUCCEEDED(hr))
		{
			hr = DepthFrame->AccessUnderlyingBuffer(&nDepthBufferSize, &pDepthBuffer);
		}

		if (SUCCEEDED(hr))
		{

			//ProcessDepth(nDepthTime, pDepthBuffer, nDepthWidth, nDepthHeight, nDepthMinReliableDistance, nDepthMaxDistance);

		}




		//get color frame data

		//----

		// get body index frame data



		if (SUCCEEDED(hr))
		{
			hr = BodyIndexFrame->get_FrameDescription(&pBodyIndexFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameDescription->get_Width(&nBodyIndexWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameDescription->get_Height(&nBodyIndexHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = BodyIndexFrame->AccessUnderlyingBuffer(&nBodyIndexBufferSize, &pBodyIndexBuffer);
		}
		//Process body index buffer TODO:Write it's own function ProcessUser

		for (int i = 0; i < 512*424; i++) {
			USHORT player = pBodyIndexBuffer[i];

			//User texture
			if (player != 0xff) {
				QuadUSER[i].rgbRed = 255;
				QuadUSER[i].rgbGreen = 0;
				QuadUSER[i].rgbBlue = 0;
				QuadUSER[i].rgbReserved = 255;
			}
			else {
				QuadUSER[i].rgbRed = 0;
				QuadUSER[i].rgbGreen = 0;
				QuadUSER[i].rgbBlue = 0;
				QuadUSER[i].rgbReserved = 0;
			}
		}

		if (SUCCEEDED(hr))
		{
			
			ProcessFrames(pDepthBuffer, nDepthWidth, nDepthHeight, pBodyIndexBuffer, nBodyIndexWidth, nBodyIndexHeight);
			this->MultiFrameReady = true;
		}



		SafeRelease(pDepthFrameDescription);
		
		SafeRelease(pBodyIndexFrameDescription);


	}

	SafeRelease(DepthFrame);
	SafeRelease(InfraredFrame);
	SafeRelease(BodyIndexFrame);
	SafeRelease(BodyFrame);
	SafeRelease(MultiSourceFrame);

	



}

void FKinectV2Device::ProcessInfrared(INT64 nTime, const UINT16* pBuffer, int nHeight, int nWidth) {
	


}

void FKinectV2Device::ProcessDepth(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth)
{
	UE_LOG(KinectV2, Log, TEXT("%d %d"), nWidth, nHeight);
	// Make sure we've received valid data
	if (QuadDEPTH && pBuffer && (nWidth == 1920) && (nHeight == 1080))
	{
		

		// end pixel is start + width*height - 1
		const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

		while (pBuffer < pBufferEnd)
		{
			USHORT depth = *pBuffer;

			int32 r, g, b;

			UKinectV1FunctionLibrary::IntToRGB(depth, r, g, b);

			QuadDEPTH->rgbRed = 255;
			QuadDEPTH->rgbGreen = 0;
			QuadDEPTH->rgbBlue = 0;
			QuadDEPTH->rgbReserved = 255;

			++QuadDEPTH;
			++pBuffer;
		}
	}
}

void FKinectV2Device::ProcessFrames(
	const UINT16* pDepthBuffer, int nDepthWidth, int nDepthHeight,
	const BYTE* pBodyIndexBuffer, int nBodyIndexWidth, int nBodyIndexHeight

	) {

	
		// Make sure we've received valid data
		if (CoordinateMapper && DepthCoordinates && QuadRGB && QuadDEPTH &&
			pDepthBuffer && (nDepthWidth == 512) && (nDepthHeight == 424) &&
			pBodyIndexBuffer && (nBodyIndexWidth == 512) && (nBodyIndexHeight == 424)
			)
		{
			
			HRESULT hr = CoordinateMapper->MapColorFrameToDepthSpace(nDepthWidth * nDepthHeight, (UINT16*)pDepthBuffer, 1920 * 1080, DepthCoordinates);
			if (SUCCEEDED(hr))
			{
				//Hands on depth map
				FVector2D HandLeftPosition2D(0, 0);
				FVector2D HandRightPosition2D(0, 0);
				EJointType ClosestBodyJointLeftHand = EJointType::SpineBase;
				EJointType ClosestBodyJointRightHand = EJointType::SpineBase;
				
				float ClosestBodyJointLeftHandDepth = 450;
				float ClosestBodyJointRightHandDepth = 450;
				float HandLeftDepth = 450;
				float HandRightDepth = 450;
				float HandLeftRadius = 1024;
				float HandRightRadius = 1024;
				
				if (IsTracking()) {
					int MinimumHandRadius = 40;

					
					//Hand left
					HandLeftDepth = FMath::Abs(GetBonePosition(EJointType::HandLeft).Z);
					HandLeftPosition2D = GetBonePosition2D(EJointType::HandLeft);
					HandLeftRadius=(GetBonePosition2D(EJointType::WristLeft)- GetBonePosition2D(EJointType::HandTipLeft)).Size()*HandOcclusionMultiplier;
					if (HandLeftRadius < MinimumHandRadius) HandLeftRadius = MinimumHandRadius;
					
					ClosestBodyJointLeftHand = GetClosestBodyJoint(EJointType::HandLeft);
					ClosestBodyJointLeftHandDepth = FMath::Abs(GetBonePosition(ClosestBodyJointLeftHand).Z);

					
					//Hand right
					HandRightDepth = FMath::Abs(GetBonePosition(EJointType::HandRight).Z);
					HandRightPosition2D = GetBonePosition2D(EJointType::HandRight);
					HandRightRadius = (GetBonePosition2D(EJointType::WristRight) - GetBonePosition2D(EJointType::HandTipRight)).Size()*HandOcclusionMultiplier;
					if (HandRightRadius < MinimumHandRadius) HandRightRadius = MinimumHandRadius;

					ClosestBodyJointRightHand = GetClosestBodyJoint(EJointType::HandRight);
					ClosestBodyJointRightHandDepth = FMath::Abs(GetBonePosition(ClosestBodyJointRightHand).Z);
				}

				
				// loop over output pixels
				for (int colorIndex = 0; colorIndex < (1920*1080); ++colorIndex)
				{
					// default setting source to copy from the background pixel
					RGBQUAD* pSrc = QuadBACKGROUND + colorIndex;
					RGBQUAD* pSrcDepth = QuadBACKGROUND + colorIndex;
					

					pSrc->rgbBlue = 0;
					pSrc->rgbGreen = 0;
					pSrc->rgbRed = 0;
					pSrc->rgbReserved = 0;

					

					pSrcDepth->rgbBlue = 255;
					pSrcDepth->rgbGreen = 255;
					pSrcDepth->rgbRed = 255;
					pSrcDepth->rgbReserved = 0;


					int depthLast = 0;
					bool playerPixel = false;

					DepthSpacePoint p = DepthCoordinates[colorIndex];

					// Values that are negative infinity means it is an invalid color to depth mapping so we
					// skip processing for this pixel
					if (p.X != -std::numeric_limits<float>::infinity() && p.Y != -std::numeric_limits<float>::infinity())
					{
						int depthX = static_cast<int>(p.X + 0.5f);
						int depthY = static_cast<int>(p.Y + 0.5f);

						if ((depthX >= 0 && depthX < nDepthWidth) && (depthY >= 0 && depthY < nDepthHeight))
						{
							USHORT player = pBodyIndexBuffer[depthX + (depthY * 512)];
							USHORT depth = pDepthBuffer[depthX + (depthY * 512)];


							// if we're tracking a player for the current pixel, draw from the color camera
							int x = colorIndex % 1920;
							int y = colorIndex / 1920;
							
							
							
							if ((player != 0xff) &&
									(
										((FVector2D(x, y) - HandLeftPosition2D).Size() < HandLeftRadius) && (depth < ClosestBodyJointLeftHandDepth-175)
										|| ((FVector2D(x, y) - HandRightPosition2D).Size() < HandRightRadius) && (depth < ClosestBodyJointRightHandDepth - 175)

									)
								)
							{
								playerPixel = true;
								// set source for copy to the color pixel

								pSrc = QuadRGB + colorIndex;

								pSrcDepth->rgbRed = QuadRGB[depthX + (depthY * 512)].rgbRed;
								pSrcDepth->rgbGreen = QuadRGB[depthX + (depthY * 512)].rgbGreen;
								pSrcDepth->rgbBlue = QuadRGB[depthX + (depthY * 512)].rgbBlue;
								pSrcDepth->rgbReserved = QuadRGB[depthX + (depthY * 512)].rgbReserved;


								

								int32 r, g, b;
								UKinectV1FunctionLibrary::IntToRGB(depth, r, g, b);

								QuadDEPTH[colorIndex].rgbRed = r;
								QuadDEPTH[colorIndex].rgbGreen = g;
								QuadDEPTH[colorIndex].rgbBlue = b;
								QuadDEPTH[colorIndex].rgbReserved = 255;

								

							} else {
								//Make the background very far 
								QuadDEPTH[colorIndex].rgbRed = 255;
								QuadDEPTH[colorIndex].rgbGreen = 255;
								QuadDEPTH[colorIndex].rgbBlue = 255;
								QuadDEPTH[colorIndex].rgbReserved = 255;

							}
							
							

							//m_pOutputDepthRGBX[colorIndex] = *current; //Full depth to color space
							//End 
						}
					} else {
						QuadDEPTH[colorIndex].rgbRed = 255;
						QuadDEPTH[colorIndex].rgbGreen = 255;
						QuadDEPTH[colorIndex].rgbBlue = 255;
						QuadDEPTH[colorIndex].rgbReserved = 255;
					}

					// write output
					QuadBACKGROUND[colorIndex] = *pSrc;
					//m_pOutputDepthRGBX[colorIndex] = *pSrcDepth; //Player to color space
				}
			}
		}
}

void FKinectV2Device::ProcessBody(int nBodyCount, IBody** ppBodies)
{
	bIsTracking = false; //Ezt lehet a for -ba kellene beletenni

	//Check if lost
	if (PlayerTrackingID != 0) {
		bool bFoundTrackedPlayer = false;
		for (int i = 0; i < nBodyCount; ++i)
		{
			IBody* pBody = ppBodies[i];
			if (pBody)
			{
				BOOLEAN bTracked = false;
				HRESULT hr = pBody->get_IsTracked(&bTracked);
				if (SUCCEEDED(hr) && bTracked)
				{
					UINT64 trackingId;
					pBody->get_TrackingId(&trackingId);
					if (trackingId == PlayerTrackingID) {
						bFoundTrackedPlayer = true;
					}
				}
			}
		}

		if (bFoundTrackedPlayer == false) {
			PlayerTrackingID = 0;
			bIsTracking = false;
		}
	}

	//End check if lost
	for (int i = 0; i < nBodyCount; ++i)
	{
		IBody* pBody = ppBodies[i];
		if (pBody)
		{
			BOOLEAN bTracked = false;
			HRESULT hr = pBody->get_IsTracked(&bTracked);

			if (SUCCEEDED(hr) && bTracked)
			{
				UINT64 trackingId;
				pBody->get_TrackingId(&trackingId);

				hr = pBody->GetJoints(JointType_Count, Joints);
				

				

				if (SUCCEEDED(hr))
				{
					//Only track one user
					if (this->PlayerTrackingID == 0 && bIsTracking == false) {

						//Check for pose
						
						FVector leftHand = this->GetBonePosition(EJointType::HandLeft);
						FVector rightHand = this->GetBonePosition(EJointType::HandRight);
						FVector neck = this->GetBonePosition(EJointType::Neck);

						

						if (leftHand.Y > neck.Y && rightHand.Y > neck.Y) {
							PlayerTrackingID = trackingId;
							bIsTracking = true;

							this->BoneOrientationFilter->Reset();
							UE_LOG(KinectV2, Log, TEXT("Tracking started!"));
						}
						

					}
					else if (PlayerTrackingID == trackingId) {
						bIsTracking = true;

						pBody->GetJoints(JointType_Count, Joints);
						pBody->GetJointOrientations(JointType_Count, JointOrientations);

						////////////////////////
						/// Filters ////////////
						this->BoneOrientationFilter->UpdateFilter(pBody, this->JointOrientations, this->FaceOrientations);
						////////////////////////
						


						HandState leftHandState = HandState_Unknown;
						HandState rightHandState = HandState_Unknown;

						pBody->get_HandLeftState(&leftHandState);
						pBody->get_HandRightState(&rightHandState);

						/* Don't need that for now
						PlayerHandStates[trackingId].LeftHandState = leftHandState;
						PlayerHandStates[trackingId].RightHandState = rightHandState;
						*/


						for (int u = 0; u < _countof(Joints); u++) {
							this->CoordinateMapper->MapCameraPointToColorSpace(Joints[u].Position, &JointsColorSpace[u]);
						}
					}
				}

#include "Windows/AllowWindowsPlatformTypes.h"
				DWORD clippedEdges;
#include "Windows/HideWindowsPlatformTypes.h"
				IsOutOfFrame = true;
				hr = pBody->get_ClippedEdges(&clippedEdges);
				if (SUCCEEDED(hr))
				{
					if (clippedEdges & FrameEdge_Left) {
						//printf("Out of frame - LEFT\n");
						IsOutOfFrame = true;
					}
					else if (clippedEdges & FrameEdge_Right) {
						//printf("Out of frame - RIGHT\n");
						IsOutOfFrame = true;
					}
					else if (clippedEdges & FrameEdge_Top) {
						//printf("Out of frame - TOP\n");
						IsOutOfFrame = true;
					}
					else if (clippedEdges & FrameEdge_Bottom) {
						//printf("Out of frame - BOTTOM\n");
						IsOutOfFrame = true;
					}
					else {
						//printf("In the frame \n");
						IsOutOfFrame = false;
					}
				}
			}
		}
		if (bIsTracking == true && PlayerTrackingID != 0) break;
	}
}

JointType FKinectV2Device::ConvertJoint(EJointType Joint) {
		switch (Joint) {

			//case EJointType::SpineBase: return JointType_SpineBase; break;
			case EJointType::SpineBase: return JointType_SpineMid; break;
			
			case EJointType::SpineMid:return JointType_SpineMid; break;
			
				//case EJointType::Neck:return JointType_Neck; break;
			case EJointType::Neck:return JointType_SpineShoulder; break;

			case EJointType::Head:return JointType_Head; break;
			case EJointType::ShoulderLeft:return JointType_ShoulderLeft; break;
			case EJointType::ElbowLeft: return JointType_ElbowLeft; break;
			case EJointType::WristLeft: return JointType_WristLeft; break;
			case EJointType::HandLeft: return JointType_HandLeft; break;
			case EJointType::ShoulderRight:return JointType_ShoulderRight; break;
			case EJointType::ElbowRight: return JointType_ElbowRight; break;
			case EJointType::WristRight: return JointType_WristRight; break;
			case EJointType::HandRight: return JointType_HandRight; break;
			case EJointType::HipLeft: return JointType_HipLeft; break;
			case EJointType::KneeLeft:return JointType_KneeLeft; break;
			case EJointType::AnkleLeft: return JointType_AnkleLeft; break;
			case EJointType::FootLeft: return JointType_FootLeft; break;
			case EJointType::HipRight:return JointType_HipRight; break;
			case EJointType::KneeRight: return JointType_KneeRight; break;
			case EJointType::AnkleRight:return JointType_AnkleRight; break;
			case EJointType::FootRight:return JointType_FootRight; break;
			case EJointType::SpineShoulder:return JointType_SpineShoulder; break;
			case EJointType::HandTipLeft:return JointType_HandTipLeft; break;
			case EJointType::ThumbLeft:return JointType_ThumbLeft; break;
			case EJointType::HandTipRight:return JointType_HandTipRight; break;
			case EJointType::ThumbRight:return JointType_ThumbRight; break;
		};
		return JointType_SpineBase;
	
}

void FKinectV2Device::ProcessFaces(IBody** ppBodies)
{
	HRESULT hr;
	//IBody* ppBodies[BODY_COUNT] = { 0 };
	//bool bHaveBodyData = SUCCEEDED(UpdateBodyData(ppBodies));
	bool bHaveBodyData = true;

	// iterate through each face reader
	for (int iFace = 0; iFace < BODY_COUNT; ++iFace)
	{
		// retrieve the latest face frame from this reader
		IFaceFrame* pFaceFrame = nullptr;
		hr = FaceFrameReaders[iFace]->AcquireLatestFrame(&pFaceFrame);



		BOOLEAN bFaceTracked = false;
		if (SUCCEEDED(hr) && nullptr != pFaceFrame)
		{
			// check if a valid face is tracked in this face frame
			hr = pFaceFrame->get_IsTrackingIdValid(&bFaceTracked);
		}

		if (SUCCEEDED(hr))
		{
			if (bFaceTracked)
			{
				IFaceFrameResult* pFaceFrameResult = nullptr;
				RectI faceBox = { 0 };
				PointF facePoints[FacePointType::FacePointType_Count];
				Vector4 faceRotation=_Vector4();
				DetectionResult faceProperties[FaceProperty::FaceProperty_Count];



				hr = pFaceFrame->get_FaceFrameResult(&pFaceFrameResult);



				// need to verify if pFaceFrameResult contains data before trying to access it
				if (SUCCEEDED(hr) && pFaceFrameResult != nullptr)
				{
					uint64 faceTrackingId;
					pFaceFrameResult->get_TrackingId(&faceTrackingId);


					hr = pFaceFrameResult->get_FaceBoundingBoxInColorSpace(&faceBox);

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, facePoints);
					}

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->get_FaceRotationQuaternion(&faceRotation);
					}

					if (SUCCEEDED(hr))
					{
						FaceOrientations[faceTrackingId] = faceRotation;
						hr = pFaceFrameResult->GetFaceProperties(FaceProperty::FaceProperty_Count, faceProperties);
					}

					if (SUCCEEDED(hr))
					{
						FString faceText = "";

						// extract each face property information and store it is faceText
						for (int iProperty = 0; iProperty < FaceProperty::FaceProperty_Count; iProperty++)
						{
							switch (iProperty)
							{
							case FaceProperty::FaceProperty_Happy:
								faceText += "Happy :";


								break;
							case FaceProperty::FaceProperty_Engaged:
								faceText += "Engaged :";
								break;
							case FaceProperty::FaceProperty_LeftEyeClosed:
								faceText += "LeftEyeClosed :";
								break;
							case FaceProperty::FaceProperty_RightEyeClosed:
								faceText += "RightEyeClosed :";
								break;
							case FaceProperty::FaceProperty_LookingAway:
								faceText += "LookingAway :";
								break;
							case FaceProperty::FaceProperty_MouthMoved:
								faceText += "MouthMoved :";
								break;
							case FaceProperty::FaceProperty_MouthOpen:
								faceText += "MouthOpen :";
								break;
							case FaceProperty::FaceProperty_WearingGlasses:
								faceText += "WearingGlasses :";
								break;
							default:
								break;
							}


							switch (faceProperties[iProperty])
							{
							case DetectionResult::DetectionResult_Unknown:
								faceText += " UnKnown";
								break;
							case DetectionResult::DetectionResult_Yes:
								faceText += " Yes";
								break;

								// consider a "maybe" as a "no" to restrict 
								// the detection result refresh rate
							case DetectionResult::DetectionResult_No:
							case DetectionResult::DetectionResult_Maybe:
								faceText += " No";
								break;
							default:
								faceText += " Default";
								break;
							}

							faceText += "\n";

						}
						FaceText = faceText;

					}

					if (SUCCEEDED(hr))
					{
						// draw face frame results
						//m_pDrawDataStreams->DrawFaceFrameResults(iFace, &faceBox, facePoints, &faceRotation, faceProperties, &faceTextLayout);


					}




				}



				SafeRelease(pFaceFrameResult);
			}
			else
			{
				// face tracking is not valid - attempt to fix the issue
				// a valid body is required to perform this step
				if (bHaveBodyData)
				{
					// check if the corresponding body is tracked 
					// if this is true then update the face frame source to track this body
					IBody* pBody = ppBodies[iFace];
					if (pBody != nullptr)
					{
						BOOLEAN bTracked = false;
						hr = pBody->get_IsTracked(&bTracked);

						UINT64 bodyTId;
						if (SUCCEEDED(hr) && bTracked)
						{
							// get the tracking ID of this body
							hr = pBody->get_TrackingId(&bodyTId);
							if (SUCCEEDED(hr))
							{
								// update the face frame source with the tracking ID
								FaceFrameSources[iFace]->put_TrackingId(bodyTId);
							}
						}
					}
				}
			}
		}

		SafeRelease(pFaceFrame);
	}


}
FString FKinectV2Device::GetDeviceName() {
	return this->DeviceName;
}


EJointType FKinectV2Device::GetClosestBodyJoint(EJointType HandJoint) {
	FVector HandPos = GetBonePosition(HandJoint);
	float distanceMinimum = 10000;
	float distance = 0;
	EJointType ResultJoint=EJointType::SpineBase;

	//Joints to check
	FVector SpineBase = GetBonePosition(EJointType::SpineBase);
	FVector Neck = GetBonePosition(EJointType::Neck);
	FVector LeftThigh = GetBonePosition(EJointType::HipLeft);
	FVector RightThigh = GetBonePosition(EJointType::HipRight);

	//Check distance of each from the hand joint
	if ((HandPos - SpineBase).Size() < distanceMinimum) { distanceMinimum = (HandPos - SpineBase).Size();  ResultJoint = EJointType::SpineBase; }
	if ((HandPos - Neck).Size() < distanceMinimum) { distanceMinimum = (HandPos - Neck).Size();  ResultJoint = EJointType::Neck; }
	if ((HandPos - LeftThigh).Size() < distanceMinimum) { distanceMinimum = (HandPos - LeftThigh).Size();  ResultJoint = EJointType::HipLeft; }
	if ((HandPos - RightThigh).Size() < distanceMinimum) { distanceMinimum = (HandPos - RightThigh).Size();  ResultJoint = EJointType::HipRight; }

	return ResultJoint;
}
