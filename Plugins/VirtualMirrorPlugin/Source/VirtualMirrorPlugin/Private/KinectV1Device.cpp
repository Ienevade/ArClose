//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#include "KinectV1Device.h"
#include "VirtualMirrorPluginPrivatePCH.h"



#include <string>
#include <iostream>
#include <sstream>
#include <direct.h>
#define GetCurrentDir _getcwd

//General Log
DEFINE_LOG_CATEGORY(KinectV1);

//Quickfix for MSVC 2015 linker errors (libjpeg.lib)
#if (_MSC_VER >= 1900)
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}
#endif

#include "Core.h"


FKinectV1Device::FKinectV1Device()
{
	initiated = false;
	QuadRGB = NULL;
	QuadDEPTH = NULL;
	QuadUSER = NULL;

	UserTracker = NULL;

	SensorOffset = FVector(0, 0, 0);
	
	UE_LOG(KinectV1, Log, TEXT("Device start"));
}

FKinectV1Device::~FKinectV1Device()
{
	UE_LOG(KinectV1, Log, TEXT("Device shutdown"));
    Cleanup();
}

bool FKinectV1Device::StartupDevice()
{
	return true;
}

UTexture2D* FKinectV1Device::GetTextureRGB(){
	UE_LOG(KinectV1, Log, TEXT("GetTextureRGB"));
	if (this->initiated) {
		
		return this->TextureRGB;
		
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("KinectV1 is not initialized properly. (RGB)")));
		}
		return this->DummyTexture;
	}
}



UTexture2D* FKinectV1Device::GetTextureDEPTH() {
	if (this->initiated) {
		return this->TextureDEPTH;
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("KinectV1 is not initialized properly. (DEPTH)")));
		}
		return this->DummyTexture;
	}
}

UTexture2D* FKinectV1Device::GetTextureUSER() {
	if (this->initiated) {
		return this->TextureUSER;
	}
	else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("KinectV1 is not initialized properly. (USER)")));
		}
		return this->DummyTexture;
	}
}


void FKinectV1Device::ShutdownDevice()
{
	Cleanup();
}

void FKinectV1Device::UpdateDevice(float DeltaTime){
	if (!this->initiated) return;
	
	//Check for camera refresh rate and update accordingly
	RefreshTimer += DeltaTime;
	if (RefreshTimer >= 1.0f / 30) //30FPS
	{
		RefreshTimer = 0; //Reset timer
		
		//Update texures
		UpdateTextureRGB();
		
		UpdateTextureDEPTH();
		
		//Update user tracker
		UpdateUserTracker();

		//User map
		UpdateTextureUSER();
	}
}



void FKinectV1Device::UpdateTextureRGB(){
	
	ColorStream.readFrame(&ColorFrame);
	
	if (ColorFrame.isValid()) {

		const openni::RGB888Pixel* TextureMap = (const openni::RGB888Pixel*)ColorFrame.getData();

		int u = 0;
		for (int i = 0; i < (640 * 480); i += 1) {

			QuadRGB[u].rgbRed = TextureMap[i].r;
			QuadRGB[u].rgbGreen = TextureMap[i].g;
			QuadRGB[u].rgbBlue = TextureMap[i].b;
			QuadRGB[u].rgbReserved = 255;
			u++;
		}
		
		const size_t SizeQuadRGB = 640 * 480* sizeof(RGBQUAD);
		uint8* Dest = (uint8*)TextureRGB->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, (uint8*)QuadRGB, SizeQuadRGB);

		TextureRGB->PlatformData->Mips[0].BulkData.Unlock();
		TextureRGB->UpdateResource();
		
	}

}

void FKinectV1Device::UpdateTextureDEPTH() {
	
	DepthStream.readFrame(&DepthFrame);
	
	

	if (DepthFrame.isValid()) {
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
			int MinimumHandRadius = 25;


			//Hand left
			

				HandLeftDepth = FMath::Abs(GetBonePosition(EJointType::HandLeft).Z);
				HandLeftPosition2D = GetBonePosition2D(EJointType::HandLeft);
				HandLeftRadius = (GetBonePosition2D(EJointType::WristLeft) - GetBonePosition2D(EJointType::HandTipLeft)).Size()*HandOcclusionMultiplier;
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

		
		openni::RGB888Pixel TextureMap[640*480];
		
		const openni::DepthPixel* DepthRow = (const openni::DepthPixel*)DepthFrame.getData();
		openni::RGB888Pixel* TexRow = TextureMap +  640;
		int rowSize = DepthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);

		for (int y = 0; y < 480; ++y)
		{
			const openni::DepthPixel* Depth = DepthRow;
			
			openni::RGB888Pixel* Tex = TexRow;

			for (int x = 0; x < 640; ++x, ++Depth, ++Tex)
			{
				if (*Depth != 0)
				{
					if (
						((FVector2D(x, y) - HandLeftPosition2D).Size() < HandLeftRadius) && (*Depth < ClosestBodyJointLeftHandDepth - 150)
						|| ((FVector2D(x, y) - HandRightPosition2D).Size() < HandRightRadius) && (*Depth < ClosestBodyJointRightHandDepth - 150)
						) {

						int32 red, green, blue;

						UKinectV1FunctionLibrary::IntToRGB(*Depth, red, green, blue);
						Tex->r = (BYTE)red;
						Tex->g = (BYTE)green;
						Tex->b = (BYTE)blue;
					}
					else {
						Tex->r = 255;
						Tex->g = 255;
						Tex->b = 255;
					}
				}
				
			}

			DepthRow += rowSize;
			TexRow += DepthFrame.getWidth();
		}

		
		for (int i = 0; i < (640*480); i += 1) {
				QuadDEPTH[i].rgbRed = TextureMap[i].r;
				QuadDEPTH[i].rgbGreen = TextureMap[i].g;
				QuadDEPTH[i].rgbBlue = TextureMap[i].b;
				QuadDEPTH[i].rgbReserved = 255;
		}


		const size_t SizeQuadDEPTH = 640 * 480 * sizeof(RGBQUAD);
		uint8* Dest = (uint8*)TextureDEPTH->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, (uint8*)QuadDEPTH, SizeQuadDEPTH);

		TextureDEPTH->PlatformData->Mips[0].BulkData.Unlock();
		TextureDEPTH->UpdateResource();

	}

}

void FKinectV1Device::UpdateTextureUSER()
{
	if (!UserTrackerFrame.isValid()) return;
	
	if (UserTrackerFrame.getUsers().getSize() == 0) {
		//No users, make it full transparent
		uint8* pDest = (uint8*)TextureUSER->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		for (int y = 0; y < 480; ++y)
		{
			for (int x = 0; x < 640; ++x)
			{
				pDest[(y * 640 + x)] = 0;	pDest++;
				pDest[(y * 640 + x)] = 0;	pDest++;
				pDest[(y * 640 + x)] = 0;	pDest++;
				pDest[(y * 640 + x)] = 0;
			}
		}
		TextureUSER->PlatformData->Mips[0].BulkData.Unlock();
		TextureUSER->UpdateResource();

		return;

	}


	const nite::UserMap& userLabels = UserTrackerFrame.getUserMap();
	

	float factor[3] = { 1, 1, 1 };
	float Colors[][3] = { { 0, 0, 1 },{ 0, 0, 1 },{ 0, 0, 1 },{ 0, 0, 1 } };
	int colorCount = 3;

	
		// update dynamic texture
		
		const size_t Size = 640 * 480 * sizeof(RGBQUAD);
		
		uint8* pDest = (uint8*)TextureUSER->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		
		

	
	//Nite specific

	const nite::UserId* pLabels = userLabels.getPixels();

	

	
	for (int y = 0; y < 480; ++y)
	{
		for (int x = 0; x < 640; ++x, ++pLabels)
		{

			if (*pLabels == 0)
			{
				factor[0] = factor[1] = factor[2] = 0;
			}
			else
			{
				factor[0] = Colors[*pLabels % colorCount][0];
				factor[1] = Colors[*pLabels % colorCount][1];
				factor[2] = Colors[*pLabels % colorCount][2];
			}
			
			// write to output buffer
			
			pDest[(y * 640 + x)] = 255 * factor[0];	pDest++;
			pDest[(y * 640 + x)] = 255 * factor[1];	pDest++;
			pDest[(y * 640 + x)] = 255 * factor[2];	pDest++;
			
			if ((factor[0] == factor[1]) && (factor[1] == factor[2]) && (factor[2] == 0)) {
				pDest[(y*640 + x)] = 0;
			}
			else {
				pDest[(y*640 + x)] = 255;
			}


			factor[0] = factor[1] = factor[2] = 1;
		}
	}

	TextureUSER->PlatformData->Mips[0].BulkData.Unlock();
	TextureUSER->UpdateResource();

	
}



void FKinectV1Device::UpdateUserTracker() {
	//Check if tracker is valid
	if (!UserTracker->isValid()) return;

//Read the user frame
	nite::Status rc = UserTracker->readFrame(&UserTrackerFrame);

	if (rc != nite::STATUS_OK)
	{
		UE_LOG(KinectV1, Error, TEXT("Error reading user frame"));
		return;
	}

	//Floor
	if (this->UserTrackerFrame.getFloorConfidence()>0.5) {
		this->SceneFloor = this->UserTrackerFrame.getFloor();
		this->FloorFound = true;
	}
	else {
		this->FloorFound = false;
	}

	//Sensor Height  - Angle
	if (this->FloorFound) {
		FVector floor = FVector(SceneFloor.point._x, SceneFloor.point._y, SceneFloor.point._z);
		FVector floorNormal = FVector(SceneFloor.normal._x, SceneFloor.normal._y, SceneFloor.normal._z);

		float lenProduct = floorNormal.Size()* FVector::UpVector.Size();
		float f = FVector::DotProduct(floorNormal, FVector::UpVector) / lenProduct;
		f = FMath::Clamp(f, (float)-1.0, (float)1.0);

		float tilt = 90 - FMath::RadiansToDegrees(FMath::Acos(f));
		SensorAngle = tilt;

		float sensorHeightD = (floor.Z)*FMath::Sin(FMath::DegreesToRadians(tilt));
		float sensorHeight = FMath::Abs(floor.Y) - sensorHeightD;

		SensorHeight = sensorHeight;
	}

	const nite::Array<nite::UserData>& users = UserTrackerFrame.getUsers();


		

		this->NumberOfUsers = users.getSize();

		//If tracked user id 0 but no users 
		if (this->TrackedUserID != 0 && this->NumberOfUsers == 0) this->TrackedUserID = 0;

		//New user - Lost user 
		for (int i = 0; i < this->NumberOfUsers; ++i)
		{
			const nite::UserData& user = users[i];
			
			if (user.isNew())
			{
				UserTracker->startPoseDetection(user.getId(), nite::POSE_PSI);
				UE_LOG(KinectV1, Log, TEXT("Pose detection started on user ID: %d"), user.getId());
			}
			else if (user.getPose(nite::POSE_PSI).isEntered())
			{
				if (this->TrackedUserID == 0)
				{
					//if (this->FloorFound == true && this->SensorHeight>1300 && this->SensorHeight<2300) {
						UE_LOG(KinectV1,Log,TEXT("PSI pose detected!!!!"));
						this->Calibrating = true;
						UserTracker->stopPoseDetection(user.getId(), nite::POSE_PSI);
						UserTracker->startSkeletonTracking(user.getId());
						this->TrackedUserID = user.getId();
						UE_LOG(KinectV1,Log,TEXT("Tracking started on user ID : %d"), user.getId());
					//}
				}
			}
			else if (user.isLost() || !user.isVisible())
			{
				if (this->TrackedUserID != 0 && user.getId() == this->TrackedUserID)
				{
					UserTracker->stopSkeletonTracking((user.getId()));
					UE_LOG(KinectV1, Log, TEXT("Tracking stopped on user ID:%d"), user.getId());
					UserTracker->startPoseDetection(this->TrackedUserID, nite::POSE_PSI);
					UE_LOG(KinectV1, Log, TEXT("Pose detection started on user ID : %d"), user.getId());

					this->TrackedUserID = 0;
					this->Calibrating = false;
				}
			}
			else if (user.getId() == this->TrackedUserID && this->TrackedUserID != 0 && user.getSkeleton().getState() == nite::SkeletonState::SKELETON_TRACKED)
			{
				
				this->Calibrating = false;

				//Center of Mass for Height computation
				nite::Point3f com = user.getCenterOfMass();
				this->CoM = FVector(com._x, com._y, com._z);
				
			}
			else if (user.getId() == this->TrackedUserID && this->TrackedUserID != 0 && user.getSkeleton().getState() == nite::SkeletonState::SKELETON_CALIBRATING) {
				//UE_LOG(KinectV1, Log, TEXT("Calibration: %d"), user.getId());
				this->Calibrating = true;
			}
			else if (user.getId() == this->TrackedUserID && this->TrackedUserID != 0 && (
				user.getSkeleton().getState() == nite::SkeletonState::SKELETON_CALIBRATION_ERROR_HANDS ||
				user.getSkeleton().getState() == nite::SkeletonState::SKELETON_CALIBRATION_ERROR_HEAD ||
				user.getSkeleton().getState() == nite::SkeletonState::SKELETON_CALIBRATION_ERROR_LEGS ||
				user.getSkeleton().getState() == nite::SkeletonState::SKELETON_CALIBRATION_ERROR_TORSO ||
				user.getSkeleton().getState() == nite::SkeletonState::SKELETON_CALIBRATION_ERROR_NOT_IN_POSE ||
				user.getSkeleton().getState() == nite::SkeletonState::SKELETON_NONE
				)

				) {
				
				UE_LOG(KinectV1, Warning, TEXT("Calibration error : %d"), static_cast<int32>(user.getSkeleton().getState()));
				UserTracker->stopSkeletonTracking((user.getId()));
				this->Calibrating = false;
				
				UE_LOG(KinectV1, Log, TEXT("Tracking stopped on user ID:%d"), user.getId());
				UserTracker->startPoseDetection(user.getId(), nite::POSE_PSI);
				
				UE_LOG(KinectV1, Log, TEXT("Pose detection started on user ID:%d"), user.getId());
				this->TrackedUserID = 0;

			}


		}

}


bool FKinectV1Device::Init(bool playOni){
	RefreshTimer = 0;

	//Defaults	
	TrackedUserID = 0;
	FloorFound = false;
	Calibrating = false;
	SensorAngle = 0;
	SensorHeight = 0;
	
	//Color texture
	TextureRGB = UTexture2D::CreateTransient(640,480);
	TextureRGB->SRGB = 1;
	TextureRGB->UpdateResource();

	//Depth texture
	TextureDEPTH = UTexture2D::CreateTransient(640, 480);
	TextureDEPTH->SRGB = 0;
	TextureDEPTH->UpdateResource();

	//Texture user
	TextureUSER = UTexture2D::CreateTransient(640, 480);
	TextureUSER->SRGB = 1;
	TextureUSER->UpdateResource();

	
	//Return this if WebcamTexture cannot be created by any reason.
	DummyTexture = UTexture2D::CreateTransient(640,480);
	DummyTexture->SRGB = 1;
	DummyTexture->UpdateResource();

	//Create rgbquads for holding texture data
	QuadRGB = new RGBQUAD[640*480];
	QuadDEPTH = new RGBQUAD[640*480];
	QuadUSER = new RGBQUAD[640*480];
	
	//Create NiTE user tracker
	UserTracker = new nite::UserTracker;

	//Init sensor 
	if (!SensorInit(playOni)) {
		return false;
	}

	
	initiated = 1;
	UE_LOG(KinectV1, Log, TEXT("Init complete !!\n"));

	return 1; //Succesfull init
}

bool FKinectV1Device::SensorInit(bool playOni) {
	
	//Init OpenNI
	const char* deviceURI = openni::ANY_DEVICE;
	openni::Status rc = openni::OpenNI::initialize();

	if (rc != openni::STATUS_OK) {
		UE_LOG(KinectV1, Error, TEXT("Failed to initialize OpenNI -  %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
		return 0;
	}
	if (!playOni) {
		//Open device
		rc = Device.open(deviceURI);
		

	}else{
		//Open ONI file
		//rc = Device.open("e:/UnrealProjects/KinectV1/Plugins/KinectV1Plugin/Binaries/Win64/adam.oni");
		rc = Device.open(deviceURI);
	}

	DeviceName = Device.getDeviceInfo().getName();
	UE_LOG(KinectV1, Log, TEXT("Device found: %s "), *FString(UTF8_TO_TCHAR(Device.getDeviceInfo().getName())));
	

	if (rc != openni::STATUS_OK)
	{
		UE_LOG(KinectV1, Error, TEXT("Failed to open device -   %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
		return 0;
	}

	//Create depth stream
	UE_LOG(KinectV1, Log, TEXT("Creating depth stream."));
	rc = DepthStream.create(Device, openni::SENSOR_DEPTH);
	if (rc == openni::STATUS_OK)
	{
		const openni::Array<openni::VideoMode>& vm = Device.getSensorInfo(openni::SensorType::SENSOR_DEPTH)->getSupportedVideoModes();

		for (int i = 0; i<vm.getSize(); i++) {
			if (vm[i].getResolutionX() == 640 && vm[i].getResolutionY() == 480 && vm[i].getFps() == 30) {
				this->DepthStream.setVideoMode(vm[i]);
				break;
			}
		}

		
		UE_LOG(KinectV1, Log, TEXT("Starting depth stream."));
		rc = DepthStream.start();
		if (rc != openni::STATUS_OK)
		{
			UE_LOG(KinectV1, Error, TEXT("Couldn't start depth stream - %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
			DepthStream.destroy();
			return 0;
		}
		


	}
	else
	{
		UE_LOG(KinectV1, Error, TEXT("Couldn't find depth stream: - %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
		return 0;
	}


	//Create color stream
	UE_LOG(KinectV1, Log, TEXT("Creating color stream."));
	rc = ColorStream.create(Device, openni::SENSOR_COLOR);

	if (rc == openni::STATUS_OK)
	{
		if (this->DeviceName != "Kinect") {
			const openni::Array<openni::VideoMode>& vm = Device.getSensorInfo(openni::SensorType::SENSOR_COLOR)->getSupportedVideoModes();
			for (int i = 0; i<vm.getSize(); i++) {
				if (vm[i].getResolutionX() == 640 && vm[i].getResolutionY() == 480 && vm[i].getFps() == 30) {
					ColorStream.setVideoMode(vm[i]);
					break;
				}
			}
		}
		UE_LOG(KinectV1, Log, TEXT("Starting color stream."));
		ColorStream.start();

		if (rc != openni::STATUS_OK)
		{
			UE_LOG(KinectV1, Error, TEXT("Couldn't start color stream - %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
			ColorStream.destroy();
			return 0;
		}
	}
	else
	{
		UE_LOG(KinectV1, Error, TEXT("Couldn't find color stream: - %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
		return nite::Status::STATUS_ERROR;
	}

	//Check streams
	if (!ColorStream.isValid())
	{
		UE_LOG(KinectV1, Error, TEXT("No valid streams"));
		openni::OpenNI::shutdown();
		return 0;
	}

	//Nite
	
	if (nite::NiTE::initialize() != nite::Status::STATUS_OK)
	{
		UE_LOG(KinectV1, Error, TEXT("NITE - Loading error"));
		return 0;
	}

	//Color to Depth registration
	UE_LOG(KinectV1, Log, TEXT("Registration: %d"), Device.isImageRegistrationModeSupported(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR));
	
	//Device.setDepthColorSyncEnabled(true);
	
	openni::Status res;
	res = Device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	if (res==openni::Status::STATUS_OK) {
		UE_LOG(KinectV1, Log, TEXT("Registration successfull!"));
	} else {
		UE_LOG(KinectV1, Log, TEXT("Registration failed!"));
	}

	FString NiteDataPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "/Binaries/Win64/");

	//Store current directory
	char buff[FILENAME_MAX];
	GetCurrentDir(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	
	
	
	//Change current directory to load NiTE data

	std::string s = std::string(TCHAR_TO_UTF8(*NiteDataPath));

	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;

	UE_LOG(KinectV1, Log, TEXT("Current directory set to: - %s "), *FString(UTF8_TO_TCHAR(s.c_str())));
	SetCurrentDirectory(r.c_str());

	UE_LOG(KinectV1, Log, TEXT("Creating user tracker"));
	
	
	if (UserTracker->create(&Device) != nite::STATUS_OK)
	{
		
		UE_LOG(KinectV1, Error, TEXT("User tracker error - %s "), *FString(UTF8_TO_TCHAR(openni::OpenNI::getExtendedError())));
		return 0;
	}
	
	//Set current directory back
	UE_LOG(KinectV1, Log, TEXT("Current directory set back to: - %s "), *FString(UTF8_TO_TCHAR(current_working_dir.c_str())));
	SetCurrentDirectoryA(current_working_dir.c_str());
	return 1;
}

void FKinectV1Device::AbortTracking() {
	if (!UserTrackerFrame.isValid()) return;
	if (UserTrackerFrame.getUsers().getSize()>0) {
		for (int i = 0; i<UserTrackerFrame.getUsers().getSize(); i++) {
			UserTracker->stopSkeletonTracking((UserTrackerFrame.getUsers()[i].getId()));
			UserTracker->startPoseDetection(UserTrackerFrame.getUsers()[i].getId(), nite::POSE_PSI);
			this->TrackedUserID = 0;
			this->Calibrating = false;
		}
	}
}

FString FKinectV1Device::GetDeviceName() {
	return this->DeviceName;
}

bool  FKinectV1Device::IsTracking()
{
	if (!UserTrackerFrame.isValid()) return false;
	if (UserTrackerFrame.getUsers().getSize()>0) {
		for (int i = 0; i<UserTrackerFrame.getUsers().getSize(); i++) {
			if (UserTrackerFrame.getUsers()[i].getSkeleton().getState() == nite::SkeletonState::SKELETON_TRACKED) {
				if (this->Calibrating == false) {
					if(GetBonePosition(EJointType::SpineBase)!=FVector::ZeroVector){
						return true;
					}
				}
			}
		}
	}
	return false;
}

FRotator FKinectV1Device::GetBoneRotation(EJointType skelJoint, bool flip)
{
	nite::JointType skelJointNite = ConvertJoint(skelJoint);

	if (!UserTrackerFrame.isValid() || this->TrackedUserID == 0) return FRotator::ZeroRotator;
	FQuat newQ = FRotator::ZeroRotator.Quaternion();
	nite::Quaternion jointOri;
	nite::SkeletonJoint joint;
	joint = this->UserTrackerFrame.getUserById(this->TrackedUserID)->getSkeleton().getJoint(skelJointNite);
	jointOri = joint.getOrientation();

	if (joint.getOrientationConfidence() > 0)
	{
		
		newQ.X = jointOri.x;
		newQ.Y = -jointOri.z;
		newQ.Z = jointOri.y;
		newQ.W = jointOri.w;
		

		newQ = newQ*FRotator(90, 0, 0).Quaternion();
		
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


	return FRotator(newQ);
}

FVector2D FKinectV1Device::GetBonePosition2D(EJointType skelJoint)
{
	FVector2D JointPosition2D(0,0);
	FVector JointPostion3D = GetBonePosition(skelJoint);
	UserTracker->convertJointCoordinatesToDepth(JointPostion3D.X, JointPostion3D.Y, JointPostion3D.Z, &JointPosition2D.X, &JointPosition2D.Y);
	return JointPosition2D;
}

FVector FKinectV1Device::GetBonePosition(EJointType skelJoint, bool flip)
{
	nite::JointType skelJointNite = ConvertJoint(skelJoint);
	if (!UserTrackerFrame.isValid() || this->TrackedUserID == 0) return  FVector::ZeroVector;
	FVector newPos = FVector::ZeroVector;
	nite::Point3f jointPos;
	nite::SkeletonJoint joint;
	joint = UserTrackerFrame.getUserById(TrackedUserID)->getSkeleton().getJoint(skelJointNite);

	if (joint.getPositionConfidence()>0) {
		jointPos = joint.getPosition();
		newPos = FVector(jointPos._x, jointPos._y, jointPos._z);
	}

	//Try to return valid rotation only if not found check previous data
	if (newPos != FVector::ZeroVector) {
		JointPositionsValid[skelJoint] = newPos;
	} else {
		if (JointPositionsValid.find(skelJoint) == JointPositionsValid.end()) {
			// not found
			return newPos; //No valid data
		}
		else {
			// found
			return JointPositionsValid.find(skelJoint)->second;
		}
	}

	return newPos; 
}


bool FKinectV1Device::SensorShutdown() {
	
	ColorFrame.release();
	ColorStream.stop();
	ColorStream.destroy();

	
	DepthStream.stop();
	
	if(UserTracker){
		if (UserTracker->isValid()) {
			UserTrackerFrame.release();
			UserTracker->destroy();
		}
	}

	DepthStream.destroy();
	Device.close();
	openni::OpenNI::shutdown();
	nite::NiTE::shutdown();
	
	return true;
}



void FKinectV1Device::Cleanup(void){
	//Delete RGB quad
	if (QuadRGB)
	{
		delete[] QuadRGB;
		QuadRGB = NULL;
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

	

	//Shutdown sensor
	SensorShutdown();

	initiated = false;

	UE_LOG(KinectV1, Log, TEXT("Cleanup ready !!\n"));
}

nite::JointType FKinectV1Device::ConvertJoint(EJointType Joint) {
	switch(Joint){
		
		//Center
		case EJointType::Head: return nite::JOINT_HEAD; break;
		case EJointType::Neck: return nite::JOINT_NECK; break;
		case EJointType::SpineBase: return nite::JOINT_TORSO; break;

		//Arms
		case EJointType::ShoulderLeft: return nite::JOINT_LEFT_SHOULDER; break;
		case EJointType::ShoulderRight: return nite::JOINT_RIGHT_SHOULDER; break;
		case EJointType::ElbowLeft: return nite::JOINT_LEFT_ELBOW; break;
		case EJointType::ElbowRight: return nite::JOINT_RIGHT_ELBOW; break;
		case EJointType::HandLeft: return nite::JOINT_LEFT_HAND; break;
		case EJointType::HandRight: return nite::JOINT_RIGHT_HAND; break;
		
		//Legs
		case EJointType::HipLeft: return nite::JOINT_LEFT_HIP; break;
		case EJointType::HipRight: return nite::JOINT_RIGHT_HIP; break;
		case EJointType::KneeLeft: return nite::JOINT_LEFT_KNEE; break;
		case EJointType::KneeRight: return nite::JOINT_RIGHT_KNEE; break;
		case EJointType::FootLeft: return nite::JOINT_LEFT_FOOT; break;
		case EJointType::FootRight: return nite::JOINT_RIGHT_FOOT; break;
	}
	return nite::JOINT_TORSO;
}

EJointType FKinectV1Device::GetClosestBodyJoint(EJointType HandJoint) {
	FVector HandPos = GetBonePosition(HandJoint);
	float distanceMinimum = 10000;
	float distance = 0;
	EJointType ResultJoint = EJointType::SpineBase;

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


