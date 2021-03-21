// Copyright 2017-2018 Brace Yourself Games. All Rights Reserved.

#include "BUISaveGame.h"
#include <Kismet/GameplayStatics.h>
#include <Engine/Engine.h>
#include <Serialization/ArchiveSaveCompressedProxy.h>
#include <Serialization/MemoryReader.h>

void UBUISaveGameManager::SaveGame( UBUISaveGame* SaveGameObject, const FString& SaveGameSlotName )
{
	GEngine->AddOnScreenDebugMessage( -1, 15.0f, FColor::Yellow, TEXT( "Saving game..." ) );

	// Call SaveGameToSlot to serialize and save our SaveGameObject with name: <SaveGameSlotName>.sav
	const bool bIsSaved = UGameplayStatics::SaveGameToSlot( SaveGameObject, SaveGameSlotName, 0 );

	UE_LOG( LogTemp, Warning, TEXT( "Game saved? %d" ), bIsSaved );
}


UBUISaveGame* UBUISaveGameManager::LoadGame( const FString& SaveGameSlotName )
{
	// Try to load a saved game file (with name: <SaveGameSlotName>.sav) if exists
	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot( SaveGameSlotName, 0 );
	UBUISaveGame* SaveGameObject = Cast<UBUISaveGame>( LoadedGame );

	GEngine->AddOnScreenDebugMessage( -1, 15.0f, FColor::Yellow, TEXT( "Trying to load a saved game." ) );

	if ( SaveGameObject )
	{
		GEngine->AddOnScreenDebugMessage( -1, 15.0f, FColor::Yellow, TEXT( "Saved game found. Loaded." ) );
	}

	return SaveGameObject;	
}


#if 0 
void UBUISaveGameManager::TestSave()
{
	FString SavePath;
	UObject* ObjectData;

	FMemoryWriter MemoryWriter( ObjectData, true );
	FObjectAndNameAsStringProxyArchive Ar( MemoryWriter, false );
	Ar.ArIsSaveGame = true; //Set achive is savegame
	Ar.ArNoDelta = true;
	Object->Serialize( Ar );


	TArray CompressedData;
	FArchiveSaveCompressedProxy Compressor( CompressedData, ECompressionFlags::COMPRESS_ZLIB );
	// Compresed
	Compressor << Data;
	//send archive serialized data to binary array
	Compressor.Flush();
	FFileHelper::SaveArrayToFile( CompressedData, *SavePath )
	}


void UBUISaveGameManager::TestLoad()
{
	FString SavePath;

	FMemoryReader MemoryReader( ObjectData, true );
	FObjectAndNameAsStringProxyArchive Ar( MemoryReader, false );
	Ar.ArIsSaveGame = true; //Set achive is savegame
	Ar.ArNoDelta = true;
	Object->Serialize( Ar );


	FFileHelper::LoadFileToArray( CompressedData, *SavePath )
		// Decompress File 
		FArchiveLoadCompressedProxy Decompressor( CompressedData, ECompressionFlags::COMPRESS_ZLIB );
	//Decompress
	Decompressor << Data;
}


#if 0
// SavedByteStream is a TArray<unit8>
if ( FFileHelper::SaveArrayToFile( SavedByteStream, *FilePath ) )
{
	ByteStreamWriter.FlushCache();
	SavedByteStream.Empty();
	GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Red, TEXT( "succcessful" ) );
	return;
}
#endif
#endif
