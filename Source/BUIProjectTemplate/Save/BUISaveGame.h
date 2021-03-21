#pragma once

#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "BUISaveGame.generated.h"

UCLASS()
class UBUISaveGameManager : public UObject
{
	GENERATED_BODY()
public:
	void TestSave();
	void TestLoad();

	void SaveGame( UBUISaveGame* SaveGameObject, const FString& SaveGameSlotName );
	UBUISaveGame* LoadGame( const FString& SaveGameSlotName );
};

UCLASS()
class UBUISaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	//friend FArchive& operator<<( FArchive& ar, UBUISaveGame& SaveGameRef );


	UPROPERTY()
		FString TestName;
	UPROPERTY()
		FDateTime SaveTime;

};


//FObjectAndNameAsStringProxyArchive
