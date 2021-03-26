// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ExampleObject.generated.h"

UCLASS()
class BUIPROJECTTEMPLATE_API UExampleObject : public UObject
{
	GENERATED_BODY()
public:

protected:
	UPROPERTY()
		int32 TestIntegerWithProperty;

	int32 TestIntegerWithoutProperty;
};
