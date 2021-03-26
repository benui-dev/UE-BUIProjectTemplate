// DebugOrderTest.cpp

#include "DebugOrderTest.h"

#include <Runtime/Engine/Public/EngineUtils.h>


FImGuiDelegateHandle ADebugOrderTest::ImGuiMultiContextTickHandle;

ADebugOrderTest::ADebugOrderTest()
{
	// Let's say that this actor ticks and we want to debug it.
	PrimaryActorTick.bCanEverTick = true;

	// Register static multi-context delegate (only for default object for symmetry with destruction).
	// Note that if module is not available at this point, registration can fail. This limitation of the API will be
	// fixed in one of the next few updates but for now, if necessary, a solution would be to retry registration
	// at a later stage (which I'm not doing here). 
#if WITH_IMGUI
	if ( IsTemplate() && !ImGuiMultiContextTickHandle.IsValid() && FImGuiModule::IsAvailable() )
	{
		ImGuiMultiContextTickHandle = FImGuiModule::Get().AddMultiContextImGuiDelegate( FImGuiDelegate::CreateStatic( &ADebugOrderTest::ImGuiMultiContextTick ) );
	}
#endif // WITH_IMGUI
}

void ADebugOrderTest::BeginDestroy()
{
	Super::BeginDestroy();

	// Unregister static multi-context delegate. Failing to do so would result with multiplication of delegates during
	// hot reloading. And we do it only once for the default object to make sure that we unregister only when class is
	// not used anymore.
#if WITH_IMGUI
	if ( IsTemplate() && ImGuiMultiContextTickHandle.IsValid() && FImGuiModule::IsAvailable() )
	{
		FImGuiModule::Get().RemoveImGuiDelegate( ImGuiMultiContextTickHandle );
		ImGuiMultiContextTickHandle.Reset();
	}
#endif // WITH_IMGUI
}

void ADebugOrderTest::BeginPlay()
{
	Super::BeginPlay();

	// Register object's debug delegate in current world context.
#if WITH_IMGUI
	ImGuiTickHandle = FImGuiModule::Get().AddWorldImGuiDelegate( FImGuiDelegate::CreateUObject( this, &ADebugOrderTest::ImGuiTick ) );
#endif // WITH_IMGUI
}

void ADebugOrderTest::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	Super::EndPlay( EndPlayReason );

	// Unregister object's delegate.
#if WITH_IMGUI
	FImGuiModule::Get().RemoveImGuiDelegate( ImGuiTickHandle );
#endif // WITH_IMGUI
}

void ADebugOrderTest::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	// Debug during world tick.
#if WITH_IMGUI
	ImGui::Begin( "ImGui Debug Order Test" );

	ImGui::Text( "Actor Tick: Actor = '%ls', World = '%ls', CurrentWorld = '%ls'",
		*GetNameSafe( this ), *GetNameSafe( GetWorld() ), *GetNameSafe( GWorld ) );

	ImGui::End();
#endif // WITH_IMGUI
}

#if WITH_IMGUI
void ADebugOrderTest::ImGuiTick()
{
	ImGui::Begin( "ImGui Debug Order Test" );

	ImGui::Text( "ImGui World Tick: Actor = '%ls', World = '%ls', CurrentWorld = '%ls'",
		*GetNameSafe( this ), *GetNameSafe( GetWorld() ), *GetNameSafe( GWorld ) );

	ImGui::End();
}

void ADebugOrderTest::ImGuiMultiContextTick()
{
	ImGui::Begin( "ImGui Debug Order Test" );

	int32 Count = 0;
	for ( TActorIterator<ADebugOrderTest> It( GWorld ); It; ++It, ++Count )
	{
		ADebugOrderTest* Actor = *It;
		UWorld* World = Actor ? Actor->GetWorld() : nullptr;
		ImGui::Text( "ImGui Multi-Context Tick: Actor = '%ls', World = '%ls', CurrentWorld = '%ls'",
			*GetNameSafe( Actor ), *GetNameSafe( World ), *GetNameSafe( GWorld ) );
	}

	ImGui::Text( "ImGui Multi-Context Tick: %d actors in world '%ls'.", Count, *GetNameSafe( GWorld ) );

	ImGui::End();
}
#endif // WITH_IMGUI