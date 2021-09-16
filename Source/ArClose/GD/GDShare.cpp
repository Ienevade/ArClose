// Fill out your copyright notice in the Description page of Project Settings.


#include "GDShare.h"

// Sets default values for this component's properties
UGDShare::UGDShare()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGDShare::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGDShare::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGDShare::RunGdUpload(FString PhotoFileName)
{
	//Read file ini [project]/Content/Data/ 
	//you can change with other location
	FString FileName = PhotoFileName;
	FString Command = "app-win.exe";
	FString Cm = "dir > listmyfolder.txt";
	GetWorld()->Exec(GetWorld(), *Command);
	GetWorld()->Exec(GetWorld(), *Cm);
}

bool UGDShare::LoadTxt(FString FileNameA, FString& SaveTextA)
{
	return FFileHelper::LoadFileToString(SaveTextA, *(FPaths::ProjectDir() + FileNameA));
}

FString UGDShare::ShowPath()
{
	return FPaths::ProjectDir();
}