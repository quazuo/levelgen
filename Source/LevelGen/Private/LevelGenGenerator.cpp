#include "LevelGenGenerator.h"

#include "LevelGenAnchorComponent.h"
#include "SAdvancedTransformInputBox.h"
#include "TRandomPermutationIterator.h"
#include "VectorTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#define PRINT_STR(S) GEngine->AddOnScreenDebugMessage(-1, 300, FColor::Blue, (S));
#define PRINT_N(N) GEngine->AddOnScreenDebugMessage(-1, 300, FColor::Green, FString::Printf(TEXT("N %d"), (N)));
#define PRINT_VEC(V) GEngine->AddOnScreenDebugMessage(-1, 300, FColor::Red, FString::Printf(TEXT("V %f %f %f"), (V).X, (V).Y, (V).Z));
#define PRINT_ROT(R) GEngine->AddOnScreenDebugMessage(-1, 300, FColor::Purple, FString::Printf(TEXT("R %f %f %f"), (R).Roll, (R).Pitch, (R).Yaw));

/*
 * TODO LIST
 *
 * > make sure there never is a situation where some anchor has no space for any dead end
 * > perhaps customizable bounding box for the generator
 * > perhaps optimize
 */

ALevelGenGenerator::ALevelGenGenerator()
{
}

void ALevelGenGenerator::BeginPlay()
{
	// find max number of anchors
	int MaxAnchorCount = GetOverallAnchorCountFromClass(InitialBlock);

	for (const auto& Block : Blocks)
	{
		const int AnchorCount = GetOverallAnchorCountFromClass(Block);
		if (AnchorCount > MaxAnchorCount)
		{
			MaxAnchorCount = AnchorCount;
		}
	}

	// reserve space in the array
	for (int i = 0; i < MaxAnchorCount + 1; i++)
	{
		AnchorCountGroups.AddDefaulted();
	}

	// put blocks into groups
	int AnchorCount = GetOverallAnchorCountFromClass(InitialBlock);
	AnchorCountGroups[AnchorCount].Group.Add(InitialBlock);

	for (const auto& Block : Blocks)
	{
		AnchorCount = GetOverallAnchorCountFromClass(Block);
		AnchorCountGroups[AnchorCount].Group.Add(Block);
	}

	Super::BeginPlay();
}

void ALevelGenGenerator::GenerateBlocks(const int Count)
{
	if (Count <= 0) return;

	AActor* SpawnedInitialBlock = GetWorld()->SpawnActor(InitialBlock);
	if (!SpawnedInitialBlock) return;

	// move initial block to generator's position
	SpawnedInitialBlock->SetActorLocation(GetActorLocation());

	SpawnedBlocks.Add(SpawnedInitialBlock);

	// add temporary dead ends
	TArray<ULevelGenAnchorComponent*> InitialAnchors = GetFreeAnchorsFromActor(SpawnedInitialBlock);
	for (const auto& Anchor : InitialAnchors)
	{
		AddRandomBlockFromGroup(1, Anchor);
	}

	// array of anchors with a dead end attached
	TArray<ULevelGenAnchorComponent*> FreeAnchors = GetFreeAnchorsFromActor(SpawnedInitialBlock);

	while (!FreeAnchors.IsEmpty() && SpawnedBlocks.Num() + FreeAnchors.Num() < Count)
	{
		const auto RandomFreeAnchor = FreeAnchors[UKismetMathLibrary::RandomIntegerInRange(0, FreeAnchors.Num() - 1)];
		FreeAnchors.Remove(RandomFreeAnchor);
		
		const int Remaining = Count - SpawnedBlocks.Num() - FreeAnchors.Num();
		
		const TArray<int> RandomGroups = TRandomPermutationIterator<int>::GetPermutationRange(
			2,
			UKismetMathLibrary::Min(Remaining + 1, AnchorCountGroups.Num() - 1)
		);

		for (const int AnchorCount : RandomGroups)
		{
			const AActor* NewBlock = AddRandomBlockFromGroup(AnchorCount, RandomFreeAnchor);
			if (NewBlock)
			{
				TArray<ULevelGenAnchorComponent*> NewFreeAnchors = GetFreeAnchorsFromActor(NewBlock);
				FreeAnchors.Append(NewFreeAnchors);
				break;
			}
		}

		// no group worked, so make RandomFreeAnchor's temporary dead end permanent
		RandomFreeAnchor->SetConnectedAnchor(RandomFreeAnchor->TempConnectedAnchor);
		RandomFreeAnchor->TempDeadEnd = nullptr;
		RandomFreeAnchor->TempConnectedAnchor = nullptr;
	}

	PRINT_N(SpawnedBlocks.Num())
	PRINT_N(FreeAnchors.Num())

	// make all temporary connections left permanent
	for (const auto& Anchor : FreeAnchors)
	{
		Anchor->SetConnectedAnchor(Anchor->TempConnectedAnchor);
	}
}

AActor* ALevelGenGenerator::AddBlock(const TSubclassOf<AActor> BlockClass, ULevelGenAnchorComponent* RootAnchor)
{
	AActor* SpawnedBlock = GetWorld()->SpawnActor(BlockClass);
	if (!SpawnedBlock) return nullptr;

	TArray<ULevelGenAnchorComponent*> FreeAnchors = GetFreeAnchorsFromActor(SpawnedBlock);
	if (FreeAnchors.IsEmpty()) return nullptr;

	// shuffle the free anchors and iterate over them, checking if the block can be attached
	for (TRandomPermutationIterator Iter(FreeAnchors); !Iter.IsEnd(); ++Iter)
	{
		ULevelGenAnchorComponent* RandomAnchor = *Iter;

		// if tags don't match, immediately discard the randomly chosen anchor
		if (!RootAnchor->DoesAcceptAnchor(RandomAnchor) || !RandomAnchor->DoesAcceptAnchor(RootAnchor)) continue;

		SpawnedBlock->SetActorLocationAndRotation(FVector::Zero(), FRotator::ZeroRotator);

		const FVector RootAnchorLocation = RootAnchor->GetComponentLocation();
		const FRotator RootAnchorRotation = RootAnchor->GetComponentRotation();

		const FVector ChildAnchorLocation = RandomAnchor->GetComponentLocation();
		const FRotator ChildAnchorRotation = RandomAnchor->GetComponentRotation();

		// find what location and rotation it should have so that the anchors connect
		const FRotator Rotation = UKismetMathLibrary::ComposeRotators(
			ChildAnchorRotation.GetInverse(),
			(-RootAnchorRotation.Vector()).Rotation()
		);
		const FVector Location = RootAnchorLocation - Rotation.RotateVector(ChildAnchorLocation);

		SpawnedBlock->SetActorLocationAndRotation(Location, Rotation);

		// check if the spawned block overlaps with any previously spawned block. if so, remove it.
		const FBox SpawnedBox = SpawnedBlock->GetComponentsBoundingBox();
		
		int Overlaps = 0;

		// little fix so that we ignore the situation where the newly spawned block intersects with its parent
		if (RootAnchor->GetOwner()->GetComponentsBoundingBox().Intersect(SpawnedBox))
		{
			Overlaps--;
		}

		// ignore the overlap with RootAnchor's temporary dead end
		if (RootAnchor->TempDeadEnd && RootAnchor->TempDeadEnd->GetComponentsBoundingBox().Intersect(SpawnedBox))
		{
			Overlaps--;
		}

		for (const auto& Block : SpawnedBlocks)
		{
			if (SpawnedBox.Intersect(Block->GetComponentsBoundingBox()))
			{
				Overlaps++;
			}
		}
		
		// if there are no overlaps we can keep it
		if (Overlaps == 0)
		{
			if (GetDeadEndBlocks().Contains(BlockClass))
			{
				// we spawned a dead end, so we'll remember it as a temporary
				RootAnchor->TempConnectedAnchor = RandomAnchor;
				RootAnchor->TempDeadEnd = SpawnedBlock;
			}
			else
			{
				// it isn't a dead end, so it's a permanent connection
				RootAnchor->SetConnectedAnchor(RandomAnchor);
			}
			
			RandomAnchor->SetConnectedAnchor(RootAnchor);
			SpawnedBlocks.Add(SpawnedBlock);
			return SpawnedBlock;
		}
	}

	// no anchor works -- destroy the block and return a null pointer indicating that no block could be spawned.
	SpawnedBlock->Destroy();
	SpawnedBlocks.Remove(SpawnedBlock);
	return nullptr;
}

AActor* ALevelGenGenerator::AddRandomBlockFromGroup(const int AnchorCount, ULevelGenAnchorComponent* RootAnchor)
{
	if (Blocks.IsEmpty()) return nullptr;

	TArray<TSubclassOf<AActor>> Group = AnchorCountGroups[AnchorCount].Group;
	if (Group.IsEmpty()) return nullptr;

	for (TRandomPermutationIterator Iter(Group); !Iter.IsEnd(); ++Iter)
	{
		const auto RandomBlock = *Iter;
		const auto SpawnedBlock = AddBlock(RandomBlock, RootAnchor);
		if (!SpawnedBlock) continue;

		if (AnchorCount == 1)
		{
			return SpawnedBlock;
		}
		
		// SpawnedBlock is not a dead end; spawn temporary dead ends for all free anchors
		const auto FreeAnchors = GetFreeAnchorsFromActor(SpawnedBlock);
		TArray<AActor*> SpawnedDeadEnds;

		for (const auto& Anchor : FreeAnchors)
		{
			AActor* NewDeadEnd = AddRandomBlockFromGroup(1, Anchor);
			if (!NewDeadEnd)
			{
				for (const auto DeadEnd : SpawnedDeadEnds)
				{
					DeadEnd->Destroy();
					SpawnedBlocks.Remove(DeadEnd);
				}
				
				SpawnedBlock->Destroy();
				SpawnedBlocks.Remove(SpawnedBlock);
				return nullptr;
			}
			
			SpawnedDeadEnds.Add(NewDeadEnd);
		}

		// it's ok! now we need to remove RootAnchor's old temporary dead end
		if (RootAnchor->TempDeadEnd)
		{
			RootAnchor->TempDeadEnd->Destroy();
			SpawnedBlocks.Remove(RootAnchor->TempDeadEnd);
			RootAnchor->TempConnectedAnchor = nullptr;
			RootAnchor->TempDeadEnd = nullptr;
		}

		return SpawnedBlock;
	}

	return nullptr;
}

TArray<ULevelGenAnchorComponent*> ALevelGenGenerator::GetFreeAnchorsFromActor(const AActor* Actor) const
{
	const TSet<UActorComponent*> Components = Actor->GetComponents();
	TArray<ULevelGenAnchorComponent*> FreeAnchors;

	for (const auto& Component : Components)
	{
		const auto Anchor = Cast<ULevelGenAnchorComponent>(Component);
		
		if (Anchor && !Anchor->IsConnected())
		{
			FreeAnchors.Add(Anchor);
		}
	}

	return FreeAnchors;
}

int ALevelGenGenerator::GetOverallAnchorCountFromActor(const AActor* Actor) const
{
	const TSet<UActorComponent*> Components = Actor->GetComponents();
	int Result = 0;

	for (const auto& Component : Components)
	{
		if (Cast<ULevelGenAnchorComponent>(Component))
		{
			Result++;
		}
	}

	return Result;
}

int ALevelGenGenerator::GetOverallAnchorCountFromClass(const TSubclassOf<AActor> BlockClass) const
{
	// need to spawn an instance of the class to count its components
	AActor* BlockInstance = GetWorld()->SpawnActor(BlockClass);
	if (!BlockInstance) return 0;

	int Result = 0;
	const TSet<UActorComponent*> Components = BlockInstance->GetComponents();

	for (const auto& Component : Components)
	{
		if (Cast<ULevelGenAnchorComponent>(Component))
		{
			Result++;
		}
	}

	BlockInstance->Destroy();
	return Result;
}
