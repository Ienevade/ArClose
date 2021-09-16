//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

using UnrealBuildTool;
using System.IO;
using System;

public class OpenNI2 : ModuleRules
{
    public OpenNI2(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
    {
        Type = ModuleType.External;

        //string SDKDIR = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
		string SDKDIR = ModuleDirectory;
        
        SDKDIR = SDKDIR.Replace("\\", "/");
        
	if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{

            PublicIncludePaths.Add(SDKDIR+"/OpenNI2/Include/");

            string LibPath = SDKDIR + "/OpenNI2/Lib/";
            
            PublicAdditionalLibraries.AddRange(
                new string[] {
                    LibPath+"OpenNI2.lib",
                }
            );
            

            PublicDelayLoadDLLs.AddRange(new string[] { "OpenNI2.dll" });

            //
            RuntimeDependencies.Add(SDKDIR+ "/../../../../Binaries/Win64/OpenNI2.dll");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/OpenNI.ini");

            //OpenNI2/Drivers
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/OpenNI2/Drivers/Kinect.dll");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/OpenNI2/Drivers/OniFile.dll");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/OpenNI2/Drivers/PS1080.dll");

        } 
	}
}
