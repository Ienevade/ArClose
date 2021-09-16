//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//

#pragma once

#include "Runtime/Core/Public/Modules/ModuleManager.h" 


/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class IVirtualMirrorPlugin : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IVirtualMirrorPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked< IVirtualMirrorPlugin >( "VirtualMirrorPlugin" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "VirtualMirrorPlugin" );
	}

	FORCEINLINE TSharedPtr<class FKinectV1Device> GetKinectV1Device() const
	{
		return KinectV1Device;
	}

	FORCEINLINE TSharedPtr<class FKinectV2Device> GetKinectV2Device() const
	{
		return KinectV2Device;
	}

	/**
	* Simple helper function to get the device currently active.
	* @return	Pointer to the KinectV1Device, or nullptr if Device is not available.
	*/

	// KINECT V1.0
	static FKinectV1Device* GetKinectV1DeviceSafe()
	{
#if WITH_EDITOR
		FKinectV1Device* KinectV1Device = IVirtualMirrorPlugin::IsAvailable() ? IVirtualMirrorPlugin::Get().GetKinectV1Device().Get() : nullptr;
#else
		FKinectV1Device* KinectV1Device = IVirtualMirrorPlugin::Get().GetKinectV1Device().Get();
#endif
		return KinectV1Device;
	}
	
	// KINECT V2.0
	static FKinectV2Device* GetKinectV2DeviceSafe()
	{
#if WITH_EDITOR
		FKinectV2Device* KinectV2Device = IVirtualMirrorPlugin::IsAvailable() ? IVirtualMirrorPlugin::Get().GetKinectV2Device().Get() : nullptr;
#else
		FKinectV2Device* KinectV2Device = IVirtualMirrorPlugin::Get().GetKinectV2Device().Get();
#endif
		return KinectV2Device;
	}


protected:
	/**
	* Reference to the actual KinectV1Device, grabbed through the GetKinectV1Device() interface, and created and destroyed in Startup/ShutdownModule
	*/
	TSharedPtr<class FKinectV1Device> KinectV1Device;
	TSharedPtr<class FKinectV2Device> KinectV2Device;
};


