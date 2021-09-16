//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//
using UnrealBuildTool;
using System.IO;
using System;

namespace UnrealBuildTool.Rules
{
	public class VirtualMirrorPlugin : ModuleRules
	{
		public VirtualMirrorPlugin(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
        {

            PrivatePCHHeaderFile = "Private/VirtualMirrorPluginPrivatePCH.h";
            bEnableUndefinedIdentifierWarnings = false;


            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
            PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));


            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core","Engine","ImageWrapper","OpenNI2","NiTE2","Kinect20","libcurl","CoreUObject","HTTP","JSON"
				}
				);
		}
	}
}