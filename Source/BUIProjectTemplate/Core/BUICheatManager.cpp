#include "BUICheatManager.h"
#include "../Save/BUISaveGame.h"

void UBUICheatManager::TestSave( FString SaveGameSlotName )
{
	UBUISaveGame* SaveGame = NewObject<UBUISaveGame>();

	UBUISaveGameManager* SaveManager = NewObject<UBUISaveGameManager>();
	SaveManager->SaveGame( SaveGame, SaveGameSlotName );
}

void UBUICheatManager::TestLoad( const FString& SaveGameSlotName )
{
	UBUISaveGameManager* SaveManager = NewObject<UBUISaveGameManager>();
	UBUISaveGame* SaveGame = SaveManager->LoadGame( SaveGameSlotName );
}
