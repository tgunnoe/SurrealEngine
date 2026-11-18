#pragma once

#include "PropertyOffsets.h"
#include <cstddef>

// Whether the begin-play loops of a map load leave the actor at `index` alone. This version visits every actor.
template<typename FlagReader>
inline bool SkipBeginPlayAtLoad(size_t index, size_t loadCount, const PropertyDataOffset& flagOffset, FlagReader&& flagIsSet)
{
	(void)index;
	(void)loadCount;
	(void)flagOffset;
	(void)flagIsSet;
	return false;
}
