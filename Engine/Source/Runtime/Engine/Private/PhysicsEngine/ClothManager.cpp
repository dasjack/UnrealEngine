// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ClothManager.h"
#include "PhysicsPublic.h"
#include "Components/SkeletalMeshComponent.h"

FClothManager::FClothManager(UWorld* InAssociatedWorld)
: AssociatedWorld(InAssociatedWorld)
 {
	//simulate cloth tick
	StartClothTickFunction.bCanEverTick = true;
	StartClothTickFunction.Target = this;
	StartClothTickFunction.TickGroup = TG_StartCloth;
	StartClothTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);

	//prepare cloth data ticks
	PrepareClothDataArray[(int32)PrepareClothSchedule::IgnorePhysics].TickFunction.TickGroup = TG_StartPhysics;
	PrepareClothDataArray[(int32)PrepareClothSchedule::WaitOnPhysics].TickFunction.TickGroup = TG_PreCloth;

	for(FClothManagerData& PrepareClothData : PrepareClothDataArray)
	{
		PrepareClothData.TickFunction.bCanEverTick = true;
		PrepareClothData.TickFunction.Target = &PrepareClothData;
		PrepareClothData.TickFunction.bRunOnAnyThread = true;	//this value seems to be ignored
		PrepareClothData.TickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);
	}
 }

void FClothManagerData::PrepareCloth(float DeltaTime)
{
#if WITH_APEX_CLOTHING
	if(SkeletalMeshComponents.Num())
	{
		IsPreparingCloth.AtomicSet(true);
		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{	
			SkeletalMeshComponent->SubmitClothSimulationContext();	//make sure user params are passed internally

			FClothSimulationContext& ClothSimulationContext = SkeletalMeshComponent->InternalClothSimulationContext;
			SkeletalMeshComponent->ParallelTickClothing(DeltaTime, ClothSimulationContext);
		}

		IsPreparingCloth.AtomicSet(false);
	}
#endif
}

void FClothManager::RegisterForPrepareCloth(USkeletalMeshComponent* SkeletalMeshComponent, PrepareClothSchedule PrepSchedule)
{
	check(PrepSchedule != PrepareClothSchedule::MAX);
	PrepareClothDataArray[(int32)PrepSchedule].SkeletalMeshComponents.Add(SkeletalMeshComponent);
}

void FClothManager::StartCloth()
{
	bool bNeedSimulateCloth = false;

	FGraphEventArray ThingsToComplete;
	for (FClothManagerData& PrepareData : PrepareClothDataArray)
	{
		if(PrepareData.SkeletalMeshComponents.Num())
		{
			if (PrepareData.PrepareCompletion.IsValid())
			{
				ThingsToComplete.Add(PrepareData.PrepareCompletion);
			}

			PrepareData.SkeletalMeshComponents.Reset();
			bNeedSimulateCloth = true;
		}
	}

	if(ThingsToComplete.Num())
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FClothManager_WaitPrepareCloth);
		FTaskGraphInterface::Get().WaitUntilTasksComplete(ThingsToComplete, ENamedThreads::GameThread);
	}
	
	if(bNeedSimulateCloth)
	{
		if (FPhysScene* PhysScene = AssociatedWorld->GetPhysicsScene())
		{
			PhysScene->StartCloth();
		}
	}
}


void FStartClothTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	Target->StartCloth();
}

FString FStartClothTickFunction::DiagnosticMessage()
{
	return TEXT("FStartClothTickFunction");
}

TAutoConsoleVariable<int32> CVarParallelCloth(TEXT("p.ParallelCloth"), 0, TEXT("If turned on, preparing cloth will happen off the game thread."));

class FPrepareClothTickTask
{
	FClothManagerData* PrepareClothData;
	float DeltaTime;

public:
	FPrepareClothTickTask(FClothManagerData* InPrepareClothData, float InDeltaTime)
		: PrepareClothData(InPrepareClothData)
		, DeltaTime(InDeltaTime)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FPrepareClothTickTask, STATGROUP_TaskGraphTasks);
	}
	static ENamedThreads::Type GetDesiredThread()
	{
		return CVarParallelCloth.GetValueOnGameThread() ? ENamedThreads::AnyThread : ENamedThreads::GameThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		PrepareClothData->PrepareCloth(DeltaTime);
	}
};


void FPrepareClothTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FPrepareClothTickFunction_ExecuteTick);

	check(!Target->PrepareCompletion.IsValid())
	Target->PrepareCompletion = TGraphTask<FPrepareClothTickTask>::CreateTask().ConstructAndDispatchWhenReady(Target, DeltaTime);
}

FString FPrepareClothTickFunction::DiagnosticMessage()
{
	return TEXT("FPrepareClothTickFunction");
}
