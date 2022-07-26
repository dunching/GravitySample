﻿// Copyright Ben Sutherland 2022. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingNavSystemTypes.h"
#include "SVOQuerySettings.generated.h"

//----------------------------------------------------------------------//
//
// FSVOQuerySettings definition
//
// Defines settings for pathfinding queries
// 
//----------------------------------------------------------------------//
USTRUCT(BlueprintType)
struct FLYINGNAVSYSTEM_API FSVOQuerySettings
{
	GENERATED_BODY()
	
	friend struct FSVOPathfindingGraph;

	FSVOQuerySettings():
		PathfindingAlgorithm(EPathfindingAlgorithm::LazyThetaStar),
		bAllowPartialPaths(false),
		HeuristicScale(1.f),
		bUseUnitCost(false),
		bUseNodeCompensation(false),
		bUsePawnCentreForPathFollowing(true),
		DebugPathColor(FLinearColor::Red)
	{}

	explicit FSVOQuerySettings(const FSVOData& InNavData,
	                           const EPathfindingAlgorithm InPathfindingAlgorithm = EPathfindingAlgorithm::LazyThetaStar,
	                           const bool bAllowPartialPaths = false,
	                           const float HeuristicScale = 1.f,
	                           const bool bUseUnitCost = false,
	                           const bool bUseNodeCompensation = false,
	                           const bool bUseActorCentreAsMiddle = true,
	                           const FLinearColor DebugPathColor = FLinearColor::Red):
		PathfindingAlgorithm(InPathfindingAlgorithm),
		bAllowPartialPaths(bAllowPartialPaths),
		HeuristicScale(HeuristicScale),
		bUseUnitCost(bUseUnitCost),
		bUseNodeCompensation(bUseNodeCompensation),
		bUsePawnCentreForPathFollowing(bUseActorCentreAsMiddle),
		DebugPathColor(DebugPathColor),
		SVOData(InNavData.AsShared())
	{}

	FORCEINLINE void SetNavData(const FSVOData& InNavData) { SVOData = InNavData.AsShared(); }

	// Algorithm to use for pathfinding. A* is the fastest, but produces jagged paths. Theta* is the slowest and finds the shortest path. Lazy Theta* is faster but less accurate than Theta* (recommended).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	EPathfindingAlgorithm PathfindingAlgorithm;

	// Find a path despite the goal not being accessible - WARNING: can be slow
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	bool bAllowPartialPaths;

	// How much to scale the A* heuristic by. High values can speed up pathfinding, at the cost of accuracy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	float HeuristicScale;

	// Makes all nodes, regardless of size, the same cost. Speeds up pathfinding at the cost of accuracy (AI prefers open spaces).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	bool bUseUnitCost;

	// Compensates node size even more, by multiplying node cost by 1 for a leaf node, and 0.2f for the root node.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	bool bUseNodeCompensation;

	// Root: TotalCost *= (1-MaxNodeCompensation)
	static constexpr float MaxNodeCompensation = 0.8f;
	
	// Compensate path points to make flying pawns follow the path through their centre, rather than their feet.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	bool bUsePawnCentreForPathFollowing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pathfinding)
	FLinearColor DebugPathColor;
	
#if PATH_BENCHMARK
	// Editor only benchmark metric
	mutable int64 NumIterations = 0;
#endif

	// Used as GetHeuristicCost's multiplier, required for graph template class
	FORCEINLINE FCoord GetHeuristicScale() const { return HeuristicScale; }
	
	// Estimate of cost from CurrentNodeRef to EndNodeRef
	FCoord GetHeuristicCost(const FSVOLink CurrentNodeRef, const FSVOLink EndNodeRef) const
	{
		// Standard euclidean heuristic
		return (SVOData->GetPositionForLinkCheckTemp(CurrentNodeRef) - SVOData->GetPositionForLinkCheckTemp(EndNodeRef)).Size();
	}
	
	// Real cost of traveling from CurrentNodeRef directly to NeighbourNodeRef
	FCoord GetTraversalCost(const FSVOLink CurrentNodeRef, const FSVOLink NeighbourNodeRef) const
	{
		return bUseUnitCost ? 1.f : (SVOData->GetPositionForLinkCheckTemp(CurrentNodeRef) - SVOData->GetPositionForLinkCheckTemp(NeighbourNodeRef)).Size();
	}
	
	// Whether traversing given edge is allowed
	FORCEINLINE static bool IsTraversalAllowed(const FSVOLink NodeA, const FSVOLink NodeB)
	{
		check(NodeA.IsValid() && NodeB.IsValid()) // Should only get valid NodeRefs
        return true;
	}
	
	// Whether to accept solutions that do not reach the goal
	FORCEINLINE bool WantsPartialSolution() const{ return bAllowPartialPaths; }
	
protected:
	FSVODataConstPtr SVOData;
};