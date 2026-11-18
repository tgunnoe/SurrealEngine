#pragma once

#include "PropertyOffsets.h"
#include <cstddef>

// Whether a begin-play loop of a map load leaves the actor at `index` of the level alone.
//
// Upstream's loops visit every actor, including the ones that InitGame and the begin-play events spawn during the load,
// so such an actor runs PreBeginPlay, BeginPlay, PostBeginPlay and SetInitialState once from its own spawn and again from
// the loops. Unreal Tournament's Actor.uc declares
//     // set to prevent re-initializing of actors spawned during level startup
//     var bool bScriptInitialized;
// and SetInitialState sets it, which says the original does not run them a second time. Deus Ex needs the second
// SetInitialState (an item that DeusExCarcass hides after its own spawn returns to its state through it) and Unreal Gold's
// own script does not settle the question, so both keep upstream's loops: their Actor classes declare no such flag, and an
// absent property's offset is ~(size_t)0, which throws when read, so the offset is tested before the flag is.
//
// An actor is skipped when the property exists, its index is at or past `loadCount` (the actor count taken before
// InitGame, so a placed actor is never skipped) and its flag is set. Every loop that calls this must pass the same count.
// This is a standing divergence from upstream: delete the function and its calls when the loops stop visiting actors
// spawned during the load, or when the stock scripts show that an actor whose SetInitialState has run is meant to run
// its begin-play events again.
template<typename FlagReader>
inline bool SkipBeginPlayAtLoad(size_t index, size_t loadCount, const PropertyDataOffset& flagOffset, FlagReader&& flagIsSet)
{
	if (flagOffset.DataOffset == ~(size_t)0)
		return false;
	return index >= loadCount && flagIsSet();
}
