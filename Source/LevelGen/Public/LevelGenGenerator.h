#pragma once

#include "CoreMinimal.h"
#include "LevelGenAnchorComponent.h"
#include "GameFramework/Actor.h"
#include "LevelGenGenerator.generated.h"

// Workaround to get 2-dimensional arrays (why they aren't in the engine yet is beyond me...).
USTRUCT()
struct FBlockGroup
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TSubclassOf<AActor>> Group;
};

/**
 * A LevelGenGenerator is an actor used for setting the initial point for level generation.
 * Generated levels consist of "blocks" chosen beforehand by the user by editing the `InitialBlock` and `Blocks`
 * properties. 
 */
UCLASS()
class LEVELGEN_API ALevelGenGenerator : public AActor
{
	GENERATED_BODY()

public:
	ALevelGenGenerator();

protected:
	virtual void BeginPlay() override;

public:
	/* First block spawned when starting level generation. Will be spawned exactly at the generator's location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Generation)
	TSubclassOf<AActor> InitialBlock;

	/* Pool of blocks used during level generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Generation)
	TArray<TSubclassOf<AActor>> Blocks;

	/** 
	 * Start generating the level.
	 * @param Count Number of blocks to spawn. Keep in mind that in some rare cases the number of spawned blocks might
	 * not be exactly equal to Count, however it will try to make it as close as possible.
	 */
	UFUNCTION(BlueprintCallable)
	void GenerateBlocks(int Count);

private:
	/**
	 * Add a block of provided class, attached to a set anchor. The anchor which will be connected to RootAnchor is
	 * chosen at random until one fits. This function might fail, e.g. if there isn't enough space for the block,
	 * regardless of anchor choice.
	 * @param BlockClass Class of the requested block.
	 * @param RootAnchor Anchor of the neighbouring block, to which some anchor of the new block will be connected.
	 * @return Pointer to the spawned block or NULL if the function failed.
	 */
	UFUNCTION()
	AActor* AddBlock(TSubclassOf<AActor> BlockClass, ULevelGenAnchorComponent* RootAnchor);

	/**
	 * Add a random block with a set number of anchors, attached to a set anchor. This function might fail, e.g. if
	 * AddBlock (which is called by this function) fails.
	 * @param AnchorCount Number of anchors the newly spawned block should have.
	 * @param RootAnchor Anchor of the neighbouring block, to which some anchor of the new block will be connected.
	 * @return Pointer to the spawned block or NULL if the function failed.
	 */
	UFUNCTION()
	AActor* AddRandomBlockFromGroup(int AnchorCount, ULevelGenAnchorComponent* RootAnchor);

	/**
	 * Get an array of anchors owned by an actor.
	 * @param Actor Actor for which the array should be determined.
	 * @return TArray of anchors owned by Actor.
	 */
	UFUNCTION()
	TArray<ULevelGenAnchorComponent*> GetFreeAnchorsFromActor(const AActor* Actor) const;

	/**
	 * Count how many LevelGenAnchorComponents exist owned by an actor.
	 * @param Actor Actor for which the number should be determined.
	 * @return Number of anchors owned by Actor.
	 */
	UFUNCTION()
	int GetOverallAnchorCountFromActor(const AActor* Actor) const;

	/**
	 * Count how many LevelGenAnchorComponents exist in an actor class.
	 * @param BlockClass Class for which the number should be determined.
	 * @return Number of anchors in BlockClass.
	 */
	UFUNCTION()
	int GetOverallAnchorCountFromClass(const TSubclassOf<AActor> BlockClass) const;

	/**
	 * 2-dimensional array, grouping blocks with the same number of anchors. Invariant:
	 * 
	 * AnchorCountGroups[N].Group == array of blocks with N anchors
	 */
	TArray<FBlockGroup> AnchorCountGroups;
	
	FORCEINLINE TArray<TSubclassOf<AActor>> GetDeadEndBlocks() { return AnchorCountGroups[1].Group; }

	/* Array of blocks spawned so far. */
	UPROPERTY()
	TArray<AActor*> SpawnedBlocks;
};
