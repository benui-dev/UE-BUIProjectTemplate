// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ISpudObject.h"
#include "ExampleObject.generated.h"

UCLASS()
class BUIPROJECTTEMPLATE_API USPUDExampleObject : public UObject, public ISpudObject
{
	GENERATED_BODY()
public:

protected:
	UPROPERTY(SaveGame)
		int32 TestIntegerWithProperty;

	int32 TestIntegerWithoutProperty;
};
