//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

using UnrealBuildTool;
using System.IO;
using System;

public class NiTE2 : ModuleRules
{
    public NiTE2(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
    {
        Type = ModuleType.External;

        //string SDKDIR = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
		string SDKDIR = ModuleDirectory;

        SDKDIR = SDKDIR.Replace("\\", "/");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {

            PublicIncludePaths.Add(SDKDIR + "/NiTE2/Include/");
            
            string LibPath = SDKDIR + "/NiTE2/Lib/";
            
            PublicAdditionalLibraries.AddRange(
                new string[] {
                    LibPath+"NiTE2.lib",
                }
            );

            PublicDelayLoadDLLs.AddRange(new string[] {"NiTE2.dll" });

            //
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2.dll");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE.ini");
            
            //NiTE2
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/HandAlgorithms.ini");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/FeatureExtraction.ini");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/s.dat");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/h.dat");

            //NiTE2/data
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/Data/lbsdata.idx");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/Data/lbsdata.lbd");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/Data/lbsparam1.lbd");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/NiTE2/Data/lbsparam2.lbd");


        }
    }
}
