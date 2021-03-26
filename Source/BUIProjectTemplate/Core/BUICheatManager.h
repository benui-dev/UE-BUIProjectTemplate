#pragma once

#include "GameFramework/CheatManager.h"
#include "BUICheatManager.generated.h"

UCLASS()
class UBUICheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION( exec )
		void TestSave( FString SaveGameSlotName );
	UFUNCTION( exec )
		void TestLoad( const FString& SaveGameSlotName );
};

