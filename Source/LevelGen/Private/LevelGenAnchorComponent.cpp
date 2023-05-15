#include "LevelGenAnchorComponent.h"

ULevelGenAnchorComponent::ULevelGenAnchorComponent()
{
}

bool ULevelGenAnchorComponent::DoesAcceptAnchor(const ULevelGenAnchorComponent* OtherAnchor) const
{
	if (TagRestrictionType == ETagRestriction::Whitelist)
	{
		// all (and at least one) of OtherAnchor's tags must be on the whitelist
		if (OtherAnchor->Tags.IsEmpty() || OtherAnchor->Tags.ContainsByPredicate([this](const FName& Tag) -> bool
		{
			return !Whitelist.Contains(Tag);
		}))
		{
			return false;
		}
	}
	else if (TagRestrictionType == ETagRestriction::Blacklist)
	{
		// none of OtherAnchor's tags may be on the blacklist
		if (OtherAnchor->Tags.ContainsByPredicate([this](const FName& Tag) -> bool
		{
			return this->Blacklist.Contains(Tag);
		}))
		{
			return false;
		}
	}

	return true;
}
