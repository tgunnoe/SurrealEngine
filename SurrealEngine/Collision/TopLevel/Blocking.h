#pragma once

// What a moving actor ran into. A mover's brush is the one kind of actor that a trace reports
// for the world's geometry, so the rule below tells it apart from every other actor.
enum class HitKind
{
	Level,
	MoverBrush,
	Actor
};

// The moving actor, as the blocking rule reads it.
struct MovingActor
{
	bool isPlayerOrProjectile = false; // these are stopped by bBlockPlayers, everything else by bBlockActors
	bool collidesWorld = false;
	bool blocksPlayers = false;
	bool blocksActors = false;
};

// What the moving actor ran into. The level has no flags of its own, so a hit on it leaves them false.
struct HitTarget
{
	HitKind kind = HitKind::Level;
	bool isPlayerOrProjectile = false;
	bool blocksPlayers = false;
	bool blocksActors = false;
	bool isBaseOfMoving = false; // the moving actor is based on the hit actor
	bool isBasedOnMoving = false; // the hit actor is based on the moving actor
};

// Whether an actor's own flags stop the moving one: both flags of the pair that the kind of the
// moving actor, or of the hit actor, selects have to be set.
inline bool ActorBlocks(const MovingActor& moving, const HitTarget& hit)
{
	if (moving.isPlayerOrProjectile || hit.isPlayerOrProjectile)
		return hit.blocksPlayers && moving.blocksPlayers;
	return hit.blocksActors && moving.blocksActors;
}

// Whether a hit stops the moving actor. Nothing that rides the moving actor stops it, and what it
// rides does not either unless the caller asks for its own base to block (it does when it checks
// whether it can stand). The level stops an actor that collides with the world.
//
// A mover's brush is world geometry to whatever collides with the world, so the mover's own flag
// decides, whatever flags the moving actor carries.
inline bool HitBlocks(const MovingActor& moving, const HitTarget& hit, bool isOwnBaseBlocking)
{
	if (hit.isBasedOnMoving || (hit.isBaseOfMoving && !isOwnBaseBlocking))
		return false;

	switch (hit.kind)
	{
	case HitKind::Level:
		return moving.collidesWorld;
	case HitKind::MoverBrush:
		return moving.collidesWorld && (moving.isPlayerOrProjectile ? hit.blocksPlayers : hit.blocksActors);
	case HitKind::Actor:
		return ActorBlocks(moving, hit);
	}
	return false;
}
