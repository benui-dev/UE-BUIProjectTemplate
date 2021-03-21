// SerializerExample.h

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "SerializerExample.generated.h"

UCLASS()
class ASerializerExample : public AActor
{
	GENERATED_BODY()

public:

	// To register and unregister static multi-context delegate.
	ASerializerExample();
	virtual void BeginDestroy() override;

	// To register and unregister per-object world delegate.
	virtual void BeginPlay() override;
	virtual void EndPlay( const EEndPlayReason::Type EndPlayReason ) override;

	// To debug during world tick.
	virtual void Tick( float DeltaTime ) override;


	virtual void Serialize( FArchive& Ar ) override;

};