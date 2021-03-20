// DebugOrderTest.h

#pragma once

#include "ImGuiCommon.h"

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "DebugOrderTest.generated.h"

UCLASS()
class ADebugOrderTest : public AActor
{
	GENERATED_BODY()

public:

	// To register and unregister static multi-context delegate.
	ADebugOrderTest();
	virtual void BeginDestroy() override;

	// To register and unregister per-object world delegate.
	virtual void BeginPlay() override;
	virtual void EndPlay( const EEndPlayReason::Type EndPlayReason ) override;

	// To debug during world tick.
	virtual void Tick( float DeltaTime ) override;

#if WITH_IMGUI
	void ImGuiTick();
	static void ImGuiMultiContextTick();

	FImGuiDelegateHandle ImGuiTickHandle;
	static FImGuiDelegateHandle ImGuiMultiContextTickHandle;
#endif // WITH_IMGUI
};