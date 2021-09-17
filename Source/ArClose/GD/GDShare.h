// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GDShare.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARCLOSE_API UGDShare : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGDShare();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UFUNCTION(BlueprintCallable)
	FString RunGdUpload(FString PhotoFileName);

	
	


	UFUNCTION(BlueprintCallable)
	static bool LoadTxt(FString FileNameA, FString& SaveTextA);

	
};


