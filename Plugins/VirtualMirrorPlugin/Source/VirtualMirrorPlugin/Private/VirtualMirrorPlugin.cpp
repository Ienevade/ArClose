//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#include "VirtualMirrorPluginPrivatePCH.h"
#include "IVirtualMirrorPlugin.h"

#include "KinectV1Device.h"
#include "KinectV2Device.h"


class FVirtualMirrorPlugin : public IVirtualMirrorPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FVirtualMirrorPlugin, VirtualMirrorPlugin )

void FVirtualMirrorPlugin::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	// Attempt to create the device, and start it up.  Caches a pointer to the device if it successfully initializes
	
	//Kinect V1.0
	TSharedPtr<FKinectV1Device> KinectV1Startup(new FKinectV1Device);
	if (KinectV1Startup->StartupDevice())
	{
		KinectV1Device = KinectV1Startup;
	}
	//Kinect V2.0
	
	TSharedPtr<FKinectV2Device> KinectV2Startup(new FKinectV2Device);
	if (KinectV2Startup->StartupDevice())
	{
		KinectV2Device = KinectV2Startup;
	}
	

	//TODO: error handling	if dll cannot be loaded  

}

void FVirtualMirrorPlugin::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	if (KinectV1Device.IsValid())
	{
		KinectV1Device->ShutdownDevice();
		KinectV1Device = nullptr;

	}
	
	if (KinectV2Device.IsValid())
	{
		KinectV2Device->ShutdownDevice();
		KinectV2Device = nullptr;

	}
	
	
}



