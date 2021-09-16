//
// Copyright 2018 Adam Horvath - MIRROR.UPLUGINS.COM - info@uplugins.com - All Rights Reserved.
//
#include "PhotoComponent.h"
#include "VirtualMirrorPluginPrivatePCH.h"

#include "IVirtualMirrorPlugin.h"

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MINOR_VERSION == 10
	#include "Developer/ImageWrapper/Public/Interfaces/IImageWrapper.h"
	#include "Developer/ImageWrapper/Public/Interfaces/IImageWrapperModule.h"
#elif ENGINE_MINOR_VERSION >= 11 && ENGINE_MINOR_VERSION<18
	#include "Runtime/ImageWrapper/Public/Interfaces/IImageWrapper.h"
	#include "Runtime/ImageWrapper/Public/Interfaces/IImageWrapperModule.h"
#elif ENGINE_MINOR_VERSION >= 18
	#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
	#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#endif

//General Log
DEFINE_LOG_CATEGORY(VirtualMirror);


UPhotoComponent::UPhotoComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Make sure this component ticks
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bAutoActivate = true;

}



void UPhotoComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction){
	
}


//Move this to a screenshot component  //BY RAMA
UTexture2D* UPhotoComponent::LoadTexture2DFromFile(const FString& FullFilePath, bool& IsValid, int32& Width, int32& Height)
{
	IsValid = false;
	UTexture2D* LoadedT2D = NULL;



	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	//Load From File
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *FullFilePath))
	{
		return NULL;
	}


	//Create T2D!
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		TArray<uint8> UncompressedBGRA;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			LoadedT2D = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);

			//Valid?
			if (!LoadedT2D)
			{
				return NULL;
			}

			//Out!
			Width = ImageWrapper->GetWidth();
			Height = ImageWrapper->GetHeight();

			//Copy!
			void* TextureData = LoadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
			LoadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			//Update!
			LoadedT2D->UpdateResource();
		}
	}

	// Success!
	IsValid = true;
	return LoadedT2D;
}

FString UPhotoComponent::TakePhoto(){
	FString filename = "VirtualMirror.png";
	FScreenshotRequest::RequestScreenshot(filename, false /*no ui for now*/, true);
	FString fileNameNextScreenshot = FScreenshotRequest::GetFilename();
	FScreenshotRequest::CreateViewportScreenShotFilename(filename);
	
	
	return FPaths::ConvertRelativePathToFull(fileNameNextScreenshot);
}

void UPhotoComponent::PostPhotoToFB(FString PhotoFileName, FString Url, FString Token, FString Message){
	// the data
	TArray<uint8> UpFileRawData;
	FFileHelper::LoadFileToArray(UpFileRawData, *PhotoFileName);
	

	// the request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> FileUploadRequest = (&FHttpModule::Get())->CreateRequest();
	FileUploadRequest->OnProcessRequestComplete().BindUObject(this, &UPhotoComponent::PostPhotoToFBCompleted);
	FileUploadRequest->SetURL(Url);
	FileUploadRequest->SetVerb("POST");
	FileUploadRequest->SetHeader("Content-Type", "multipart/form-data; boundary=AaB03x");
	
	

	// header group for the file
	
	// content
	FString content;
	content = "\r\n";

	//Access token field
	content += "--AaB03x\r\n";
	content +="Content-Disposition:form-data; name=\"access_token\"\r\n";
	content +="Content-Type: text/plain\r\n";
	content += "Content-Transfer-Encoding: binary\r\n";
	
	content += "\r\n";
	content += Token;
	content += "\r\n";
	

	//Message field
	
	content += "--AaB03x\r\n";
	content += "Content-Disposition:form-data; name=\"message\"\r\n";
	content += "Content-Type: text/plain\r\n";
	content += "Content-Transfer-Encoding: binary\r\n";
	
	content += "\r\n";
	content += Message;
	content += "Plugin version:";
	content += MIRROR_PLUGIN_VERSION;
	content += "\r\n";

	
	

	//File field
	FString CleanFileName=FPaths::GetCleanFilename(PhotoFileName);


	//Before photo
	content += "--AaB03x\r\n";
	content += "Content-Disposition:file; filename=\"";
	content += CleanFileName;
	content += "\"\r\n";
	content += "Content-Type:image/png\r\n";
	content += "Content-Transfer-Encoding: binary\r\n";
	content += "\r\n";
	
	
	FBufferArchive BinaryBeforePhoto;
	BinaryBeforePhoto << content;
	
	//Remove ending NULL - TO DO: Find out why is it there ....
	BinaryBeforePhoto.RemoveAt(BinaryBeforePhoto.Num() - 1);

	uint8* ContentBeforePhoto = BinaryBeforePhoto.GetData();
	
	
	//After photo
	content = "\r\n";
	content += "--AaB03x\r\n";

	FBufferArchive BinaryAfterPhoto;
	BinaryAfterPhoto << content;

	//Remove ending NULL - TO DO: Find out why is it there ....
	BinaryAfterPhoto.RemoveAt(0);
	BinaryAfterPhoto.RemoveAt(1);
	BinaryAfterPhoto.RemoveAt(2);
	
	
	
	uint8* ContentAfterPhoto = BinaryAfterPhoto.GetData();
	
	TArray<uint8> ContentFinal;

	ContentFinal.Append(BinaryBeforePhoto);
	ContentFinal.Append(UpFileRawData);
	ContentFinal.Append(BinaryAfterPhoto);
	
	UE_LOG(VirtualMirror, Log, TEXT("%s"), *content);
	FileUploadRequest->SetContent(ContentFinal);
	

	// request processing
	FileUploadRequest->ProcessRequest();
}



void UPhotoComponent::PostPhotoToFBCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	
	UE_LOG(VirtualMirror, Log, TEXT("%s"), *Response->GetContentAsString());
	if (bWasSuccessful)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());

		FJsonSerializer::Deserialize(JsonReader, JsonObject);

		FString PostID = JsonObject->GetStringField("post_id");

		UE_LOG(VirtualMirror, Log, TEXT("Request completed. Post ID: %s "),*PostID);
		
		


	}
	else
	{
		// Handle error here
		UE_LOG(VirtualMirror, Log, TEXT("Request error...."));
		
	}
}



