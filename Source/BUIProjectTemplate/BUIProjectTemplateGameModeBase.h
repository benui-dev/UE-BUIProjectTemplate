// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BUIProjectTemplateGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class BUIPROJECTTEMPLATE_API ABUIProjectTemplateGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void StartPlay() override;

};
