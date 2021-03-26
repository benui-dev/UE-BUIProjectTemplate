#include "SpudSubsystem.h"
#include "EngineUtils.h"
#include "SpudState.h"
#include "Engine/LevelStreaming.h"
#include "Kismet/GameplayStatics.h"
#include "ImageUtils.h"

PRAGMA_DISABLE_OPTIMIZATION

DEFINE_LOG_CATEGORY(LogSpudSubsystem)


#define SPUD_QUICKSAVE_SLOTNAME "__QuickSave__"
#define SPUD_AUTOSAVE_SLOTNAME "__AutoSave__"


void USpudSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Note: this will register for clients too, but callbacks will be ignored
	// We can't call ServerCheck() here because GameMode won't be valid (which is what we use to determine server mode)
	OnPostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USpudSubsystem::OnPostLoadMap);
	OnPreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USpudSubsystem::OnPreLoadMap);
	
#if ENGINE_MINOR_VERSION >= 26
	// Seamless travel is only supported on 4.26+ since event is only present there
	OnSeamlessTravelHandle = FWorldDelegates::OnSeamlessTravelTransition.AddUObject(this, &USpudSubsystem::OnSeamlessTravelTransition);
#endif
	
#if WITH_EDITORONLY_DATA
	// The one problem we have is that in PIE mode, PostLoadMap doesn't get fired for the current map you're on
	// So we'll need to trigger it manually
	auto World = GetWorld();
	if (World && World->WorldType == EWorldType::PIE)
	{
		// TODO: make this more configurable, use a known save etc
		NewGame();
	}
	
#endif
}

void USpudSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(OnPostLoadMapHandle);
	FCoreUObjectDelegates::PreLoadMap.Remove(OnPreLoadMapHandle);
#if ENGINE_MINOR_VERSION >= 26
	FWorldDelegates::OnSeamlessTravelTransition.Remove(OnSeamlessTravelHandle);
#endif
}


void USpudSubsystem::NewGame()
{
	if (!ServerCheck(true))
		return;
		
	EndGame();
	CurrentState = ESpudSystemState::RunningIdle;
	SubscribeAllLevelObjectEvents();
}

bool USpudSubsystem::ServerCheck(bool LogWarning) const
{
	// Note: must only call this when game mode is present! Don't call when unloading
	// On missing world etc we just assume true for safety
	auto GI = GetGameInstance();
	if (!GI)
		return true;

	auto World = GI->GetWorld();
	if (!World)
		return true;
	
	return World->GetAuthGameMode() != nullptr;
}

void USpudSubsystem::EndGame()
{
	if (ActiveState)
		ActiveState->ResetState();
	
	// Allow GC to collect
	ActiveState = nullptr;

	UnsubscribeAllLevelObjectEvents();
	CurrentState = ESpudSystemState::Disabled;
}

void USpudSubsystem::AutoSaveGame(FText Title, bool bTakeScreenshot, const USpudCustomSaveInfo* ExtraInfo)
{
	SaveGame(SPUD_AUTOSAVE_SLOTNAME,
		Title.IsEmpty() ? NSLOCTEXT("Spud", "AutoSaveTitle", "Autosave") : Title,
		bTakeScreenshot,
		ExtraInfo);
}

void USpudSubsystem::QuickSaveGame(FText Title, bool bTakeScreenshot, const USpudCustomSaveInfo* ExtraInfo)
{
	SaveGame(SPUD_QUICKSAVE_SLOTNAME,
		Title.IsEmpty() ? NSLOCTEXT("Spud", "QuickSaveTitle", "Quick Save") : Title,
		bTakeScreenshot,
		ExtraInfo);
}

void USpudSubsystem::QuickLoadGame()
{
	LoadGame(SPUD_QUICKSAVE_SLOTNAME);
}


bool USpudSubsystem::IsQuickSave(const FString& SlotName)
{
	return SlotName == SPUD_QUICKSAVE_SLOTNAME;
}

bool USpudSubsystem::IsAutoSave(const FString& SlotName)
{
	return SlotName == SPUD_AUTOSAVE_SLOTNAME;
}

void USpudSubsystem::LoadLatestSaveGame()
{
	auto Latest = GetLatestSaveGame();
	if (Latest)
		LoadGame(Latest->SlotName);
}

void USpudSubsystem::OnPreLoadMap(const FString& MapName)
{
	if (!ServerCheck(false))
		return;

	PreTravelToNewMap.Broadcast(MapName);
	// All streaming maps will be unloaded by travelling, so remove all
	LevelRequests.Empty();
	StopUnloadTimer();
	
	FirstStreamRequestSinceMapLoad = true;

	// When we transition out of a map while enabled, save contents
	if (CurrentState == ESpudSystemState::RunningIdle)
	{
		UnsubscribeAllLevelObjectEvents();

		const auto World = GetWorld();
		if (IsValid(World))
		{
			UE_LOG(LogSpudSubsystem, Verbose, TEXT("OnPreLoadMap saving: %s"), *UGameplayStatics::GetCurrentLevelName(World));
			// Map and all streaming level data will be released.
			// Block while doing it so they all get written predictably
			StoreWorld(World, true, true);
		}
	}
}

void USpudSubsystem::OnSeamlessTravelTransition(UWorld* World)
{
	// note: this only gets called on 4.26+
	if (IsValid(World))
	{
		FString MapName = UGameplayStatics::GetCurrentLevelName(World);
		UE_LOG(LogSpudSubsystem, Verbose, TEXT("OnSeamlessTravelTransition: %s"), *MapName);
		// Just before seamless travel, do the same thing as pre load map on OpenLevel
		OnPreLoadMap(MapName);
	}
}

void USpudSubsystem::OnPostLoadMap(UWorld* World)
{
	if (!ServerCheck(false))
		return;
	
	if (CurrentState == ESpudSystemState::RunningIdle ||
		CurrentState == ESpudSystemState::LoadingGame)
	{
		// This is called when a new map is loaded
		// In all cases, we try to load the state
		if (IsValid(World)) // nullptr seems possible if load is aborted or something?
		{
			FString LevelName = UGameplayStatics::GetCurrentLevelName(World); 
			UE_LOG(LogSpudSubsystem, Verbose, TEXT("OnPostLoadMap restore: %s"),
			       *LevelName);

			auto State = GetActiveState();
			PreLevelRestore.Broadcast(LevelName);
			State->RestoreLoadedWorld(World);
			PostLevelRestore.Broadcast(LevelName, true);

			SubscribeLevelObjectEvents(World->GetCurrentLevel());
		}

		// If we were loading, this is the completion
		if (CurrentState == ESpudSystemState::LoadingGame)
		{
			LoadComplete(SlotNameInProgress, true);
			UE_LOG(LogSpudSubsystem, Log, TEXT("Load: Success"));
		}
	}

	PostTravelToNewMap.Broadcast();
}

void USpudSubsystem::SaveGame(const FString& SlotName, const FText& Title, bool bTakeScreenshot, const USpudCustomSaveInfo* ExtraInfo)
{
	if (!ServerCheck(true))
	{
		SaveComplete(SlotName, false);
        return;
	}

	if (SlotName.IsEmpty())
	{
		UE_LOG(LogSpudSubsystem, Error, TEXT("Cannot save a game with a blank slot name"));		
		SaveComplete(SlotName, false);
		return;
	}

	if (CurrentState != ESpudSystemState::RunningIdle)
	{
		// TODO: ignore or queue?
		UE_LOG(LogSpudSubsystem, Error, TEXT("TODO: Overlapping calls to save/load, resolve this"));
		SaveComplete(SlotName, false);
		return;
	}

	CurrentState = ESpudSystemState::SavingGame;
	PreSaveGame.Broadcast(SlotName);

	if (bTakeScreenshot)
	{
		UE_LOG(LogSpudSubsystem, Verbose, TEXT("Queueing screenshot for save %s"), *SlotName);

		// Memory-based screenshot request
		SlotNameInProgress = SlotName;
		TitleInProgress = Title;
		ExtraInfoInProgress = ExtraInfo;
		UGameViewportClient* ViewportClient = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetLocalPlayer()->ViewportClient;
		OnScreenshotHandle = ViewportClient->OnScreenshotCaptured().AddUObject(this, &USpudSubsystem::OnScreenshotCaptured);
		FScreenshotRequest::RequestScreenshot(false);
		// OnScreenShotCaptured will finish
		// EXCEPT that if a Widget BP is open in the editor, this request will disappear into nowhere!! (4.26.1)
		// So we need a failsafe
		// Wait for 1 second. Can't use FTimerManager because there's no option for those to tick while game paused (which is common in saves!)
		ScreenshotTimeout = 1;
	}
	else
	{
		FinishSaveGame(SlotName, Title, ExtraInfo, nullptr);
	}
}


void USpudSubsystem::ScreenshotTimedOut()
{
	// We failed to get a screenshot back in time
	// This is mostly likely down to a weird fecking issue in PIE where if ANY Widget Blueprint is open while a screenshot
	// is requested, that request is never fulfilled

	UE_LOG(LogSpudSubsystem, Error, TEXT("Request for save screenshot timed out. This is most likely a UE4 bug: "
		"Widget Blueprints being open in the editor during PIE seems to break screenshots. Completing save game without a screenshot."))

	ScreenshotTimeout = 0;
	FinishSaveGame(SlotNameInProgress, TitleInProgress, ExtraInfoInProgress, nullptr);
	
}

void USpudSubsystem::OnScreenshotCaptured(int32 Width, int32 Height, const TArray<FColor>& Colours)
{
	ScreenshotTimeout = 0;

	UGameViewportClient* ViewportClient = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetLocalPlayer()->ViewportClient;
	ViewportClient->OnScreenshotCaptured().Remove(OnScreenshotHandle);
	OnScreenshotHandle.Reset();

	// Downscale the screenshot, pass to finish
	TArray<FColor> RawDataCroppedResized;
	FImageUtils::CropAndScaleImage(Width, Height, ScreenshotWidth, ScreenshotHeight, Colours, RawDataCroppedResized);

	// Convert down to PNG
	TArray<uint8> PngData;
	FImageUtils::CompressImageArray(ScreenshotWidth, ScreenshotHeight, RawDataCroppedResized, PngData);
	
	FinishSaveGame(SlotNameInProgress, TitleInProgress, ExtraInfoInProgress, &PngData);
	
}
void USpudSubsystem::FinishSaveGame(const FString& SlotName, const FText& Title, const USpudCustomSaveInfo* ExtraInfo, TArray<uint8>* ScreenshotData)
{
	auto State = GetActiveState();
	auto World = GetWorld();

	// We do NOT reset
	// a) deleted objects must remain, they're built up over time
	// b) we may not be updating all levels and must retain for the others

	State->StoreWorldGlobals(World);
	
	for (auto Ptr : GlobalObjects)
	{
		if (Ptr.IsValid())
			State->StoreGlobalObject(Ptr.Get());
	}
	for (auto Pair : NamedGlobalObjects)
	{
		if (Pair.Value.IsValid())
			State->StoreGlobalObject(Pair.Value.Get(), Pair.Key);
	}

	// Store any data that is currently active in the game world in the state object
	StoreWorld(World, false, true);

	State->SetTitle(Title);
	State->SetTimestamp(FDateTime::Now());
	State->SetCustomSaveInfo(ExtraInfo);
	if (ScreenshotData)
		State->SetScreenshot(*ScreenshotData);
	
	// UGameplayStatics::SaveGameToSlot prefixes our save with a lot of crap that we don't need
	// And also wraps it with FObjectAndNameAsStringProxyArchive, which again we don't need
	// Plus it writes it all to memory first, which we don't need another copy of. Write direct to file
	// I'm not sure if the save game system doesn't do this because of some console hardware issues, but
	// I'll worry about that at some later point
	IFileManager& FileMgr = IFileManager::Get();
	auto Archive = TUniquePtr<FArchive>(FileMgr.CreateFileWriter(*GetSaveGameFilePath(SlotName)));

	bool SaveOK;
	if(Archive)
	{
		State->SaveToArchive(*Archive);
		// Always explicitly close to catch errors from flush/close
		Archive->Close();

		if (Archive->IsError() || Archive->IsCriticalError())
		{
			UE_LOG(LogSpudSubsystem, Error, TEXT("Error while saving game to %s"), *SlotName);
			SaveOK = false;
		}
		else
		{
			UE_LOG(LogSpudSubsystem, Log, TEXT("Save to slot %s: Success"), *SlotName);
			SaveOK = true;
		}
	}
	else
	{
		UE_LOG(LogSpudSubsystem, Error, TEXT("Error while creating save game for slot %s"), *SlotName);
		SaveOK = false;
	}

	SaveComplete(SlotName, SaveOK);

}

void USpudSubsystem::SaveComplete(const FString& SlotName, bool bSuccess)
{
	SlotNameInProgress = "";
	TitleInProgress = FText();
	ExtraInfoInProgress = nullptr;
	CurrentState = ESpudSystemState::RunningIdle;
	PostSaveGame.Broadcast(SlotName, bSuccess);
}



void USpudSubsystem::StoreWorld(UWorld* World, bool bReleaseLevels, bool bBlocking)
{
	for (auto && Level : World->GetLevels())
	{
		StoreLevel(Level, bReleaseLevels, bBlocking);
	}	
}

void USpudSubsystem::StoreLevel(ULevel* Level, bool bRelease, bool bBlocking)
{
	const FString LevelName = USpudState::GetLevelName(Level);
	PreLevelStore.Broadcast(LevelName);
	GetActiveState()->StoreLevel(Level, bRelease, bBlocking);
	PostLevelStore.Broadcast(LevelName, true);
}

void USpudSubsystem::LoadGame(const FString& SlotName)
{
	if (!ServerCheck(true))
	{
		LoadComplete(SlotName, false);
		return;
	}

	if (CurrentState != ESpudSystemState::RunningIdle)
	{
		// TODO: ignore or queue?
		UE_LOG(LogSpudSubsystem, Error, TEXT("TODO: Overlapping calls to save/load, resolve this"));
		LoadComplete(SlotName, false);
		return;
	}

	CurrentState = ESpudSystemState::LoadingGame;
	PreLoadGame.Broadcast(SlotName);

	UE_LOG(LogSpudSubsystem, Verbose, TEXT("Loading Game from slot %s"), *SlotName);		

	auto State = GetActiveState();

	State->ResetState();

	// TODO: async load

	IFileManager& FileMgr = IFileManager::Get();
	auto Archive = TUniquePtr<FArchive>(FileMgr.CreateFileReader(*GetSaveGameFilePath(SlotName)));

	if(Archive)
	{
		// Load only global data and page in level data as needed
		State->LoadFromArchive(*Archive, false);
		Archive->Close();

		if (Archive->IsError() || Archive->IsCriticalError())
		{
			UE_LOG(LogSpudSubsystem, Error, TEXT("Error while loading game from %s"), *SlotName);
			LoadComplete(SlotName, false);
			return;
		}
	}
	else
	{
		UE_LOG(LogSpudSubsystem, Error, TEXT("Error while opening save game for slot %s"), *SlotName);		
		LoadComplete(SlotName, false);
		return;
	}

	// Just do the reverse of what we did
	// Global objects first before map, these should be only objects which survive map load
	for (auto Ptr : GlobalObjects)
	{
		if (Ptr.IsValid())
			State->RestoreGlobalObject(Ptr.Get());
	}
	for (auto Pair : NamedGlobalObjects)
	{
		if (Pair.Value.IsValid())
			State->RestoreGlobalObject(Pair.Value.Get(), Pair.Key);
	}

	// This is deferred, final load process will happen in PostLoadMap
	SlotNameInProgress = SlotName;
	UE_LOG(LogSpudSubsystem, Verbose, TEXT("(Re)loading map: %s"), *State->GetPersistentLevel());		
	UGameplayStatics::OpenLevel(GetWorld(), FName(State->GetPersistentLevel()));
}


void USpudSubsystem::LoadComplete(const FString& SlotName, bool bSuccess)
{
	CurrentState = ESpudSystemState::RunningIdle;
	SlotNameInProgress = "";
	PostLoadGame.Broadcast(SlotName, bSuccess);
}

bool USpudSubsystem::DeleteSave(const FString& SlotName)
{
	if (!ServerCheck(true))
		return false;
	
	IFileManager& FileMgr = IFileManager::Get();
	return FileMgr.Delete(*GetSaveGameFilePath(SlotName), false, true);
}

void USpudSubsystem::AddPersistentGlobalObject(UObject* Obj)
{
	GlobalObjects.AddUnique(TWeakObjectPtr<UObject>(Obj));	
}

void USpudSubsystem::AddPersistentGlobalObjectWithName(UObject* Obj, const FString& Name)
{
	NamedGlobalObjects.Add(Name, Obj);
}

void USpudSubsystem::RemovePersistentGlobalObject(UObject* Obj)
{
	GlobalObjects.Remove(TWeakObjectPtr<UObject>(Obj));
	
	for (auto It = NamedGlobalObjects.CreateIterator(); It; ++It)
	{
		if (It.Value().Get() == Obj)
			It.RemoveCurrent();
	}
}

void USpudSubsystem::ClearLevelState(const FString& LevelName)
{
	GetActiveState()->ClearLevel(LevelName);
	
}

void USpudSubsystem::AddRequestForStreamingLevel(UObject* Requester, FName LevelName, bool BlockingLoad)
{
	if (!ServerCheck(false))
		return;

	auto && Request = LevelRequests.FindOrAdd(LevelName);
	Request.Requesters.AddUnique(Requester);
	if (Request.bPendingUnload)
	{
		Request.bPendingUnload = false; // no load required, just flip the unload flag
		Request.LastRequestExpiredTime = 0;
	}
	else if (Request.Requesters.Num() == 1)
	{
		// Load on the first request only		
		LoadStreamLevel(LevelName, BlockingLoad);		
	}
}

void USpudSubsystem::WithdrawRequestForStreamingLevel(UObject* Requester, FName LevelName)
{
	if (!ServerCheck(false))
		return;

	if (auto Request = LevelRequests.Find(LevelName))
	{
		Request->Requesters.Remove(Requester);
		if (Request->Requesters.Num() == 0)
		{
			// This level can be unloaded after time delay
			Request->bPendingUnload = true;
			Request->LastRequestExpiredTime = UGameplayStatics::GetTimeSeconds(GetWorld());
			StartUnloadTimer();
		}
	}
}

void USpudSubsystem::StartUnloadTimer()
{
	if (!StreamLevelUnloadTimerHandle.IsValid())
	{
		// Set up a timer which repeatedly checks for actual unload
		// This doesn't need to be every tick, just every 0.5s
		GetWorld()->GetTimerManager().SetTimer(StreamLevelUnloadTimerHandle, this, &USpudSubsystem::CheckStreamUnload, 0.5, true);
	}	
}


void USpudSubsystem::StopUnloadTimer()
{
	if (StreamLevelUnloadTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(StreamLevelUnloadTimerHandle);
	}	
}

void USpudSubsystem::CheckStreamUnload()
{
	const float UnloadBeforeTime = UGameplayStatics::GetTimeSeconds(GetWorld()) - StreamLevelUnloadDelay;
	bool bAnyStillWaiting = false;
	for (auto && Pair : LevelRequests)
	{
		const FName& LevelName = Pair.Key;
		FStreamLevelRequests& Request = Pair.Value;
		if (Request.bPendingUnload)
		{
			if (Request.Requesters.Num() == 0 &&
				Request.LastRequestExpiredTime <= UnloadBeforeTime)
			{
				Request.bPendingUnload = false;
				UnloadStreamLevel(LevelName);
			}
			else
				bAnyStillWaiting = true;
		}
	}

	// Only run the timer while we have something to do
	if (!bAnyStillWaiting)
		StopUnloadTimer();
}




void USpudSubsystem::LoadStreamLevel(FName LevelName, bool Blocking)
{
	FScopeLock PendingLoadLock(&LevelsPendingLoadMutex);
	PreLoadStreamingLevel.Broadcast(LevelName);
	
	FLatentActionInfo Latent;
	Latent.ExecutionFunction = "PostLoadStreamLevel";
	Latent.CallbackTarget = this;
	int32 RequestID = LoadUnloadRequests++; // overflow is OK
	Latent.UUID = RequestID; // this eliminates duplicate calls so should be unique
	Latent.Linkage = RequestID;
	LevelsPendingLoad.Add(RequestID, LevelName);

	// Upgrade to a blocking call if this is the first streaming level since map change (ensure appears in time)
	if (FirstStreamRequestSinceMapLoad)
	{
		Blocking = true;
		FirstStreamRequestSinceMapLoad = false;
	}

	// We don't make the level visible until the post-load callback
	UGameplayStatics::LoadStreamLevel(GetWorld(), LevelName, false, Blocking, Latent);
}

void USpudSubsystem::PostLoadStreamLevel(int32 LinkID)
{
	FScopeLock PendingLoadLock(&LevelsPendingLoadMutex);
	
	// We should be able to obtain the level name
	if (LevelsPendingLoad.Contains(LinkID))
	{
		FName LevelName = LevelsPendingLoad.FindAndRemoveChecked(LinkID);

		// This might look odd but for physics restoration to work properly we need a very specific
		// set of circumstances:
		// 1. Level must be made visible first
		// 2. We need to wait for all the objects to be ticked at least once
		// 3. Then we restore
		//
		// Failure to do this means SetPhysicsLinearVelocity etc just does *nothing* silently

		// Make visible
		auto StreamLevel = UGameplayStatics::GetStreamingLevel(GetWorld(), LevelName);
		if (StreamLevel)
		{
			StreamLevel->SetShouldBeVisible(true);
		}		

		// Defer the restore to the game thread, streaming calls happen in loading thread?
		// However, quickly ping the state to force it to pre-load the leveldata
		// that way the loading occurs in this thread, less latency
		GetActiveState()->PreLoadLevelData(LevelName.ToString());			

		AsyncTask(ENamedThreads::GameThread, [this, LevelName]()
        {
			// But also add a slight delay so we get a tick in between so physics works
			FTimerHandle H;
			GetWorld()->GetTimerManager().SetTimer(H, [this, LevelName]()
			{
				PostLoadStreamLevelGameThread(LevelName);				
			}, 0.01, false);
        });		
	}
	else
	{
		UE_LOG(LogSpudSubsystem, Error, TEXT("PostLoadStreamLevel called but not for a level we loaded??"));
	}
}


void USpudSubsystem::PostLoadStreamLevelGameThread(FName LevelName)
{
	PostLoadStreamingLevel.Broadcast(LevelName);
	auto StreamLevel = UGameplayStatics::GetStreamingLevel(GetWorld(), LevelName);

	if (StreamLevel)
	{
		ULevel* Level = StreamLevel->GetLoadedLevel();
		if (!Level)
		{
			UE_LOG(LogSpudSubsystem, Warning, TEXT("PostLoadStreamLevel called for %s but level is null; probably unloaded again?"), *LevelName.ToString());
			return;
		}
		PreLevelRestore.Broadcast(LevelName.ToString());
		// It's important to note that this streaming level won't be added to UWorld::Levels yet
		// This is usually where things like the TActorIterator get actors from, ULevel::Actors
		// we have the ULevel here right now, so restore it directly
		GetActiveState()->RestoreLevel(Level);

		// NB: after restoring the level, we could release MOST of the memory for this level
		// However, we don't for 2 reasons:
		// 1. Destroyed actors for this level are logged continuously while running, so that still needs to be active
		// 2. We can assume that we'll need to write data back to save when this level is unloaded. It's actually less
		//    memory thrashing to re-use the same memory we have until unload, since it'll likely be almost identical in structure
		StreamLevel->SetShouldBeVisible(true);
		SubscribeLevelObjectEvents(Level);
		PostLevelRestore.Broadcast(LevelName.ToString(), true);
	}
}

void USpudSubsystem::UnloadStreamLevel(FName LevelName)
{
	auto StreamLevel = UGameplayStatics::GetStreamingLevel(GetWorld(), LevelName);

	if (StreamLevel)
	{
		PreUnloadStreamingLevel.Broadcast(LevelName);
		ULevel* Level = StreamLevel->GetLoadedLevel();
		if (!Level)
		{
			// Already unloaded
			return;
		}
		UnsubscribeLevelObjectEvents(Level);
	
		if (CurrentState != ESpudSystemState::LoadingGame)
		{
			// save the state, if not loading game
			// when loading game we will unload the current level and streaming and don't want to restore the active state from that
			// After storing, the level data is released so doesn't take up memory any more
			StoreLevel(Level, true, false);
		}
		
		// Now unload
		FScopeLock PendingUnloadLock(&LevelsPendingUnloadMutex);

		FLatentActionInfo Latent;
		Latent.ExecutionFunction = "PostUnloadStreamLevel";
		Latent.CallbackTarget = this;
		int32 RequestID = LoadUnloadRequests++; // overflow is OK
		Latent.UUID = RequestID; // this eliminates duplicate calls so should be unique
		Latent.Linkage = RequestID;
		LevelsPendingUnload.Add(RequestID, LevelName);
		UGameplayStatics::UnloadStreamLevel(GetWorld(), LevelName, Latent, false);
	}	
}

void USpudSubsystem::ForceReset()
{
	CurrentState = ESpudSystemState::RunningIdle;
}

void USpudSubsystem::SetUserDataModelVersion(int32 Version)
{
	GCurrentUserDataModelVersion = Version;
}


int32 USpudSubsystem::GetUserDataModelVersion() const
{
	return GCurrentUserDataModelVersion;
}

void USpudSubsystem::PostUnloadStreamLevel(int32 LinkID)
{
	FScopeLock PendingUnloadLock(&LevelsPendingUnloadMutex);
	
	const FName LevelName = LevelsPendingUnload.FindAndRemoveChecked(LinkID);

	// Pass back to the game thread, streaming calls happen in loading thread?
	AsyncTask(ENamedThreads::GameThread, [this, LevelName]()
    {
        PostUnloadStreamLevelGameThread(LevelName);				
    });
}


void USpudSubsystem::PostUnloadStreamLevelGameThread(FName LevelName)
{
	PostUnloadStreamingLevel.Broadcast(LevelName);
}

void USpudSubsystem::SubscribeAllLevelObjectEvents()
{
	const auto World = GetWorld();
	if (IsValid(World))
	{
		for (ULevel* Level : World->GetLevels())
		{
			SubscribeLevelObjectEvents(Level);			
		}
	}
}

void USpudSubsystem::UnsubscribeAllLevelObjectEvents()
{
	const auto World = GetWorld();
	if (IsValid(World))
	{
		for (ULevel* Level : World->GetLevels())
		{
			UnsubscribeLevelObjectEvents(Level);			
		}
	}
}


void USpudSubsystem::SubscribeLevelObjectEvents(ULevel* Level)
{
	if (Level)
	{
		for (auto Actor : Level->Actors)
		{
			if (!SpudPropertyUtil::IsPersistentObject(Actor))
				continue;			
			// We don't care about runtime spawned actors, only level actors
			// Runtime actors will just be omitted, level actors need to be logged as destroyed
			if (!SpudPropertyUtil::IsRuntimeActor(Actor))
				Actor->OnDestroyed.AddDynamic(this, &USpudSubsystem::OnActorDestroyed);			
		}		
	}	
}

void USpudSubsystem::UnsubscribeLevelObjectEvents(ULevel* Level)
{
	if (Level)
	{
		for (auto Actor : Level->Actors)
		{
			if (!SpudPropertyUtil::IsPersistentObject(Actor))
				continue;

			if (!SpudPropertyUtil::IsRuntimeActor(Actor))
				Actor->OnDestroyed.RemoveDynamic(this, &USpudSubsystem::OnActorDestroyed);			
		}		
	}	
}

void USpudSubsystem::OnActorDestroyed(AActor* Actor)
{
	if (CurrentState == ESpudSystemState::RunningIdle)
	{
		auto Level = Actor->GetLevel();
		// Ignore actor destruction caused by levels being unloaded
		if (Level && !Level->bIsBeingRemoved)
		{
			auto State = GetActiveState();
			State->StoreLevelActorDestroyed(Actor);
		}
	}
}

TArray<USpudSaveGameInfo*> USpudSubsystem::GetSaveGameList(bool bIncludeQuickSave, bool bIncludeAutoSave)
{

	TArray<FString> SaveFiles;
	ListSaveGameFiles(SaveFiles);

	TArray<USpudSaveGameInfo*> Ret;
	for (auto && File : SaveFiles)
	{
		FString SlotName = FPaths::GetBaseFilename(File);

		if ((!bIncludeQuickSave && SlotName == SPUD_QUICKSAVE_SLOTNAME) ||
			(!bIncludeAutoSave && SlotName == SPUD_AUTOSAVE_SLOTNAME))
		{
			continue;			
		}

		auto Info = GetSaveGameInfo(SlotName);
		if (Info)
			Ret.Add(Info);
	}

	return Ret;
}

USpudSaveGameInfo* USpudSubsystem::GetSaveGameInfo(const FString& SlotName)
{
	IFileManager& FM = IFileManager::Get();
	// We want to parse just the very first part of the file, not all of it
	FString AbsoluteFilename = FPaths::Combine(GetSaveGameDirectory(), SlotName + ".sav");
	auto Archive = TUniquePtr<FArchive>(FM.CreateFileReader(*AbsoluteFilename));

	if(!Archive)
	{
		UE_LOG(LogSpudSubsystem, Error, TEXT("Unable to open %s for reading info"), *AbsoluteFilename);
		return nullptr;
	}
		
	auto Info = NewObject<USpudSaveGameInfo>();
	Info->SlotName = SlotName;

	USpudState::LoadSaveInfoFromArchive(*Archive, *Info);
	Archive->Close();
		
	return Info;
}

USpudSaveGameInfo* USpudSubsystem::GetLatestSaveGame()
{
	auto SaveGameList = GetSaveGameList();
	USpudSaveGameInfo* Best = nullptr;
	for (auto Curr : SaveGameList)
	{
		if (!Best || Curr->Timestamp > Best->Timestamp)
			Best = Curr;		
	}
	return Best;
}


USpudSaveGameInfo* USpudSubsystem::GetQuickSaveGame()
{
	return GetSaveGameInfo(SPUD_QUICKSAVE_SLOTNAME);
}

USpudSaveGameInfo* USpudSubsystem::GetAutoSaveGame()
{
	return GetSaveGameInfo(SPUD_AUTOSAVE_SLOTNAME);
}

FString USpudSubsystem::GetSaveGameDirectory()
{
	return FString::Printf(TEXT("%sSaveGames/"), *FPaths::ProjectSavedDir());
}

FString USpudSubsystem::GetSaveGameFilePath(const FString& SlotName)
{
	return FString::Printf(TEXT("%s%s.sav"), *GetSaveGameDirectory(), *SlotName);
}

void USpudSubsystem::ListSaveGameFiles(TArray<FString>& OutSaveFileList)
{
	IFileManager& FM = IFileManager::Get();

	FM.FindFiles(OutSaveFileList, *GetSaveGameDirectory(), TEXT(".sav"));	
}

FString USpudSubsystem::GetActiveGameFolder()
{
	return FString::Printf(TEXT("%sCurrentGame/"), *FPaths::ProjectSavedDir());
}

FString USpudSubsystem::GetActiveGameFilePath(const FString& Name)
{
	return FString::Printf(TEXT("%sSaveGames/%s.sav"), *GetActiveGameFolder(), *Name);
}


class FUpgradeAllSavesAction : public FPendingLatentAction
{
public:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	struct FUpgradeTask : public FNonAbandonableTask
	{
		bool bUpgradeAlways;
		FSpudUpgradeSaveDelegate UpgradeCallback;
		
		FUpgradeTask(bool InUpgradeAlways, FSpudUpgradeSaveDelegate InCallback) : bUpgradeAlways(InUpgradeAlways), UpgradeCallback(InCallback) {}

		bool SaveNeedsUpgrading(const USpudState* State)
		{
			if (State->SaveData.GlobalData.IsUserDataModelOutdated())
				return true;

			for (auto& Pair : State->SaveData.LevelDataMap)
			{
				if (Pair.Value->IsUserDataModelOutdated())
					return true;				
			}

			return false;
		}

		void DoWork()
		{
			if (!UpgradeCallback.IsBound())
				return;
			
			IFileManager& FileMgr = IFileManager::Get();
			TArray<FString> SaveFiles;
			USpudSubsystem::ListSaveGameFiles(SaveFiles);

			for (auto && SaveFile : SaveFiles)
			{
				FString AbsoluteFilename = FPaths::Combine(USpudSubsystem::GetSaveGameDirectory(), SaveFile);
				auto Archive = TUniquePtr<FArchive>(FileMgr.CreateFileReader(*AbsoluteFilename));

				if(Archive)
				{
					auto State = NewObject<USpudState>();
					// Load all data because we want to upgrade
					State->LoadFromArchive(*Archive, true);
					Archive->Close();

					if (Archive->IsError() || Archive->IsCriticalError())
					{
						UE_LOG(LogSpudSubsystem, Error, TEXT("Error while loading game to check for upgrades: %s"), *SaveFile);
						continue;
					}

					if (bUpgradeAlways || SaveNeedsUpgrading(State))
					{
						if (UpgradeCallback.Execute(State))
						{
							// Move aside old save
							FString BackupFilename = AbsoluteFilename + ".bak"; 
							FileMgr.Move(*BackupFilename, *AbsoluteFilename, true, true);
							// Now save
							auto OutArchive = TUniquePtr<FArchive>(FileMgr.CreateFileWriter(*AbsoluteFilename));
							if (OutArchive)
							{
								State->SaveToArchive(*OutArchive);
							}
						}
					}
				}
			}

		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FUpgradeTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	
	};

	FAsyncTask<FUpgradeTask> UpgradeTask;

	FUpgradeAllSavesAction(bool UpgradeAlways, FSpudUpgradeSaveDelegate InUpgradeCallback, const FLatentActionInfo& LatentInfo)
        : ExecutionFunction(LatentInfo.ExecutionFunction)
        , OutputLink(LatentInfo.Linkage)
        , CallbackTarget(LatentInfo.CallbackTarget)
        , UpgradeTask(UpgradeAlways, InUpgradeCallback)
	{
		// We do the actual upgrade work in a background task, this action is just to monitor when it's done
		UpgradeTask.StartBackgroundTask();
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		// This is essentially a game thread tick. Finish the latent action when the background task is done
		Response.FinishAndTriggerIf(UpgradeTask.IsDone(), ExecutionFunction, OutputLink, CallbackTarget);
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return "Upgrade All Saves";
	}
#endif
};


void USpudSubsystem::UpgradeAllSaveGames(bool bUpgradeEvenIfNoUserDataModelVersionDifferences,
                                         FSpudUpgradeSaveDelegate SaveNeedsUpgradingCallback,
                                         FLatentActionInfo LatentInfo)
{
	
	FLatentActionManager& LatentActionManager = GetGameInstance()->GetLatentActionManager();
	if (LatentActionManager.FindExistingAction<FUpgradeAllSavesAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
	{
		LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
		                                 new FUpgradeAllSavesAction(bUpgradeEvenIfNoUserDataModelVersionDifferences,
		                                                            SaveNeedsUpgradingCallback, LatentInfo));
	}
}

USpudCustomSaveInfo* USpudSubsystem::CreateCustomSaveInfo()
{
	return NewObject<USpudCustomSaveInfo>();
}



// FTickableGameObject begin


void USpudSubsystem::Tick(float DeltaTime)
{
	if (ScreenshotTimeout > 0)
	{
		ScreenshotTimeout -= DeltaTime;
		if (ScreenshotTimeout <= 0)
		{
			ScreenshotTimeout = 0;
			ScreenshotTimedOut();
		}
	}
}

ETickableTickType USpudSubsystem::GetTickableTickType() const
{
	// This is for timeout purposes
	return ETickableTickType::Always;
}

bool USpudSubsystem::IsTickableWhenPaused() const
{
	// We need the screenshot failsafe timeout even when paused
	return true;
}

TStatId USpudSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USpudSubsystem, STATGROUP_Tickables);
}


// FTickableGameObject end

PRAGMA_ENABLE_OPTIMIZATION
