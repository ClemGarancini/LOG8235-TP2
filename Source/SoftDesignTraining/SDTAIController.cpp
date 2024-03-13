// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAIController.h"
#include "SoftDesignTraining.h"
#include "SDTCollectible.h"
#include "SDTFleeLocation.h"
#include "SDTPathFollowingComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
//#include "UnrealMathUtility.h"
#include "SDTUtils.h"
#include "EngineUtils.h"
#include <iostream>
#include <typeinfo>
#include "NavigationSystem/Public/NavigationSystem.h"
#include "CoreMinimal.h"

ASDTAIController::ASDTAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<USDTPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
 
}

void ASDTAIController::GoToBestTarget(float deltaTime)
{
    OnMoveToTarget();
    // Move to target depending on current behavior
    // This function is called while m_ReachedTarget is true.
    // Check void ASDTBaseAIController::Tick for how it works.
}

void ASDTAIController::OnMoveToTarget()
{
    m_ReachedTarget = false;
}

void ASDTAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    m_ReachedTarget = true;
}

void ASDTAIController::ShowNavigationPath()
{
    //FNavPathSharedPtr path = PathFollowingComponent->GetPath();
}

void ASDTAIController::ChooseBehavior(float deltaTime)
{
    UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent();

    EPathFollowingRequestResult::Type moveRequestResult1 = MoveToLocation(GetPawn()->GetActorLocation(), 10.f);
    TArray<FNavPathPoint> pathRequested = pathFollowingComponent->GetPath()->GetPathPoints();
    switch (moveRequestResult1) {
    case EPathFollowingRequestResult::AlreadyAtGoal:
        UE_LOG(LogTemp, Warning, TEXT("AlreadyAtGoal"));
        break;
    case EPathFollowingRequestResult::RequestSuccessful:
        UE_LOG(LogTemp, Warning, TEXT("Success"));
        if (pathRequested.Num() > 1) {
            for (int i = 0; i < pathRequested.Num() - 1; i++) {
                DrawDebugLine(GetWorld(), pathRequested[i].Location, pathRequested[i + 1].Location, FColor::Green);
            }
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("not enough points"));
        }
        break;
    case EPathFollowingRequestResult::Failed:
        UE_LOG(LogTemp, Warning, TEXT("Failed"));
        break;
    }

    UpdatePlayerInteraction(deltaTime);
}

void ASDTAIController::UpdatePlayerInteraction(float deltaTime)
{
    //finish jump before updating AI state
    if (AtJumpSegment)
        return;

    APawn* selfPawn = GetPawn();
    if (!selfPawn)
        return;

    ACharacter* playerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!playerCharacter)
        return;

    FVector detectionStartLocation = selfPawn->GetActorLocation() + selfPawn->GetActorForwardVector() * m_DetectionCapsuleForwardStartingOffset;
    FVector detectionEndLocation = detectionStartLocation + selfPawn->GetActorForwardVector() * m_DetectionCapsuleHalfLength * 2;

    TArray<TEnumAsByte<EObjectTypeQuery>> detectionTraceObjectTypes;
    detectionTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(COLLISION_COLLECTIBLE));
    detectionTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(COLLISION_PLAYER));

    TArray<FHitResult> allDetectionHits;
    GetWorld()->SweepMultiByObjectType(allDetectionHits, detectionStartLocation, detectionEndLocation, FQuat::Identity, detectionTraceObjectTypes, FCollisionShape::MakeSphere(m_DetectionCapsuleRadius));

    FHitResult detectionHit;
    GetHightestPriorityDetectionHit(allDetectionHits, detectionHit);

    // Set behavior based on hit
    // This is the place where you decide to switch your path towards a new path
    // Check ASDTAIController::AIStateInterrupted to stop your current path

    DrawDebugCapsule(GetWorld(), detectionStartLocation + m_DetectionCapsuleHalfLength * selfPawn->GetActorForwardVector(), m_DetectionCapsuleHalfLength, m_DetectionCapsuleRadius, selfPawn->GetActorQuat() * selfPawn->GetActorUpVector().ToOrientationQuat(), FColor::Blue);
}

void ASDTAIController::GetHightestPriorityDetectionHit(const TArray<FHitResult>& hits, FHitResult& outDetectionHit)
{
    for (const FHitResult& hit : hits)
    {
        if (UPrimitiveComponent* component = hit.GetComponent())
        {
            if (component->GetCollisionObjectType() == COLLISION_PLAYER)
            {
                //we can't get more important than the player
                outDetectionHit = hit;
                return;
            }
            else if (component->GetCollisionObjectType() == COLLISION_COLLECTIBLE)
            {
                outDetectionHit = hit;
            }
        }
    }
}

void ASDTAIController::AIStateInterrupted()
{
    StopMovement();
    m_ReachedTarget = true;
}

void ASDTAIController::FindPathToNearestCollectible(TArray<FVector> collectibleLocations)
{
    FVector pawnLocation = GetPawn()->GetActorLocation();
    Algo::Sort(collectibleLocations, [&](FVector lhs, FVector rhs)
        {
            return (FVector::Dist(pawnLocation, lhs) < FVector::Dist(pawnLocation, rhs));
        });
    
    //for (FV/*ector collectible : collectibleLocations) {
    //    EPathFollowingRequestResult::Type moveRequestResult = MoveToLocation(collectible);
    //    if (moveRequestResult == EPathFollowingRequestResult::RequestSuccessful) {

    //    }
    //}*/

    //UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    //if (navSystem) {
    //    /*INavigationDataInterface* currentNavData = navSystem->GetMainNavData();
    //    if (currentNavData) {
    //        FString navMeshName = currentNavData->GetName();
    //        UE_LOG(LogTemp, Warning, TEXT("navMesh: %s"), *navMeshName);
    //    }
    //    else {
    //        UE_LOG(LogTemp, Warning, TEXT("noNavmesh"));
    //    }*/


    //    FNavLocation startNavLocation, endNavLocation;
    //    FPathFindingQuery pathQuery;
    //    TWeakObjectPtr<ANavigationData> weakPtrNavData(navSystem->GetDefaultNavDataInstance());
    //    pathQuery.NavData = weakPtrNavData;
    //    pathQuery.StartLocation = startLocation;
    //    pathQuery.EndLocation = endLocation;

    //    FPathFindingResult pathFound = navSystem->FindPathSync(pathQuery);
    //    bool bPathFound = pathFound.IsSuccessful();
    //    if (bPathFound) {
    //        TArray<FVector> pathPoints;
    //        for (FNavPathPoint pathPoint : pathFound.Path->GetPathPoints()) {
    //            pathPoints.Add(pathPoint.Location);
    //        }
    //        UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent();
    //        
    //    }
    //    else {
    //        UE_LOG(LogTemp, Warning, TEXT("PathNotFound"));
    //    }
    
}