// SerializerExample.cpp

#include "SerializerExample.h"

#include <Runtime/Engine/Public/EngineUtils.h>


ASerializerExample::ASerializerExample()
{
	// Let's say that this actor ticks and we want to debug it.
	PrimaryActorTick.bCanEverTick = true;

}

void ASerializerExample::BeginDestroy()
{
	Super::BeginDestroy();

}

void ASerializerExample::BeginPlay()
{
	Super::BeginPlay();

}

void ASerializerExample::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	Super::EndPlay( EndPlayReason );

}

void ASerializerExample::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void ASerializerExample::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

}
