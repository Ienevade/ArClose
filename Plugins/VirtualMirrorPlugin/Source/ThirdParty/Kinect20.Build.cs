//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

using UnrealBuildTool;
using System.IO;
using System;

public class Kinect20 : ModuleRules
{
    public Kinect20(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
    { 

        Type = ModuleType.External;

        //string SDKDIR = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
		string SDKDIR = ModuleDirectory;

        SDKDIR = SDKDIR.Replace("\\", "/");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {

            PublicIncludePaths.Add(SDKDIR + "/Kinect20/Include/");
            
            string LibPath = SDKDIR + "/Kinect20/Lib/";
            
            PublicAdditionalLibraries.AddRange(
                new string[] {
                    LibPath+"Kinect20.lib",
					LibPath+"Kinect20.Face.lib",
                }
            );

            PublicDelayLoadDLLs.AddRange(new string[] { "Kinect20.dll","Kinect20.Face.dll" });

            //
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/Kinect20.Face.dll");
            RuntimeDependencies.Add(SDKDIR + "/../../../../Binaries/Win64/Kinect20.dll");

            //NuiDatabase
            //RuntimeDependencies.Add(new RuntimeDependency(SDKDIR + "/../../../../Binaries/Win64/NuiDatabase/"));

            DirSearch(SDKDIR + "/../../../../Binaries/Win64/NuiDatabase/");

        }
    }

    public void DirSearch(string sDir)
    {
        try
        {
            //Files
            foreach (string fs in Directory.GetFiles(sDir))
            {
                //Console.WriteLine(f);
                RuntimeDependencies.Add(fs);
            }

            //Directories
            foreach (string d in Directory.GetDirectories(sDir))
            {
                DirSearch(d);
            }
        }
        catch (System.Exception excpt)
        {
            Console.WriteLine(excpt.Message);
        }
    }
}
