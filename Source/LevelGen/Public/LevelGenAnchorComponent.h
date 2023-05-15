#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LevelGenAnchorComponent.generated.h"

UENUM()
enum class ETagRestriction
{
	None,
	Whitelist,
	Blacklist
};

/**
 * A LevelGenAnchorComponent represents an anchor used to connect two blocks together.
 * This component has the option to limit what other anchors it can connect with, either by whitelisting or
 * blacklisting (see below).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LEVELGEN_API ULevelGenAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULevelGenAnchorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelGen)
	TArray<FName> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelGen)
	ETagRestriction TagRestrictionType = ETagRestriction::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelGen,
		meta = ( EditCondition = "TagRestrictionType == ETagRestriction::Whitelist" ))
	TArray<FName> Whitelist;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelGen,
		meta = ( EditCondition = "TagRestrictionType == ETagRestriction::Blacklist" ))
	TArray<FName> Blacklist;

	/* Check if the tag filters accept other anchor's tags. */
	UFUNCTION()
	bool DoesAcceptAnchor(const ULevelGenAnchorComponent* OtherAnchor) const;

	UFUNCTION()
	FORCEINLINE void SetConnectedAnchor(ULevelGenAnchorComponent* OtherAnchor) { ConnectedAnchor = OtherAnchor; }

	UFUNCTION()
	FORCEINLINE bool IsConnected() const { return ConnectedAnchor != nullptr; }

	// temporary values // todo - describe this
	UPROPERTY()
	ULevelGenAnchorComponent* TempConnectedAnchor;

	UPROPERTY()
	AActor* TempDeadEnd;

private:
	UPROPERTY()
	ULevelGenAnchorComponent* ConnectedAnchor;
};
