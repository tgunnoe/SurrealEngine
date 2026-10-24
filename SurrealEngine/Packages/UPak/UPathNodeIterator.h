#pragma once

#include "Packages/Engine/Actors/UActor.h"

class UNavigationPoint;

class UPakPathNodeIterator : public UActor
{
public:
	using UActor::UActor;

	void BuildPath(vec3& start, vec3& end);
	UNavigationPoint* GetFirst();
	UNavigationPoint* GetPrevious();
	UNavigationPoint* GetCurrent();
	UNavigationPoint* GetNext();
	UNavigationPoint* GetLast();
	UNavigationPoint* GetLastVisible();

	FixedArrayView<UNavigationPoint*, 64> NodePath() { return FixedArray<UNavigationPoint*, 64>(PropOffsets_UPakPathNodeIterator.NodePath); }
	int& NodeCount() { return Value<int>(PropOffsets_UPakPathNodeIterator.NodeCount); }
	int& NodeIndex() { return Value<int>(PropOffsets_UPakPathNodeIterator.NodeIndex); }
	int& NodeCost() { return Value<int>(PropOffsets_UPakPathNodeIterator.NodeCost); }
	vec3& NodeStart() { return Value<vec3>(PropOffsets_UPakPathNodeIterator.NodeStart); }
};
