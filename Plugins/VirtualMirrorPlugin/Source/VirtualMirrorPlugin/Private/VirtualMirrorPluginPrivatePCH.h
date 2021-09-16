//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//
#include "IVirtualMirrorPlugin.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.

#include "Engine.h"
#include "Http.h"
//#include "Base64.h"

#include "NiTE.h"
 
#ifdef _WIN64
	#include "Windows/AllowWindowsPlatformTypes.h"

#endif

#include "kinect.h" 
#include "kinect.face.h"



#ifdef _WIN64
	#include "Windows/HideWindowsPlatformTypes.h"
#endif

