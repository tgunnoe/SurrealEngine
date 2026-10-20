#include "Precomp.h"
#include "TestRunner.h"
#include "Collision/TopLevel/Blocking.h"

// A moving actor is stopped by a hit when the rule in Blocking.h says so. The level stops whatever
// collides with the world. A mover's brush is world geometry to whatever collides with the world,
// so the mover's own flag decides: bBlockPlayers when the moving actor is a player or a projectile,
// bBlockActors otherwise, whatever flags the moving actor carries. Any other actor stops the moving
// one only when the flags of both agree, and nothing that rides the moving actor stops it.

namespace
{
	// A stock projectile or decoration has no block flags and collides with the world.
	MovingActor WorldColliderWithNoFlags(bool isPlayerOrProjectile)
	{
		MovingActor moving;
		moving.isPlayerOrProjectile = isPlayerOrProjectile;
		moving.collidesWorld = true;
		return moving;
	}

	HitTarget MoverBrush(bool blocksPlayers, bool blocksActors)
	{
		HitTarget hit;
		hit.kind = HitKind::MoverBrush;
		hit.blocksPlayers = blocksPlayers;
		hit.blocksActors = blocksActors;
		return hit;
	}

	// A pawn is solid to players and to other actors alike.
	HitTarget SolidPawn()
	{
		HitTarget hit;
		hit.kind = HitKind::Actor;
		hit.blocksPlayers = true;
		hit.blocksActors = true;
		return hit;
	}
}

TEST(BlockingProjectileIsStoppedByMoverBrush)
{
	// A projectile has no block flags, yet a mover that blocks players stops it.
	MovingActor projectile = WorldColliderWithNoFlags(true);
	CHECK(HitBlocks(projectile, MoverBrush(true, false), false));
}

TEST(BlockingDecorationIsStoppedByMoverBrush)
{
	// A decoration has no block flags, yet a mover that blocks actors stops it.
	MovingActor decoration = WorldColliderWithNoFlags(false);
	CHECK(HitBlocks(decoration, MoverBrush(false, true), false));
}

TEST(BlockingProjectileIsNotStoppedByPawn)
{
	// Only a mover's brush is ruled by its own flags. A pawn is an actor, which stops a moving actor
	// only when that actor's own flag agrees, so a projectile with no flags is not stopped by a pawn
	// however solid the pawn is.
	MovingActor projectile = WorldColliderWithNoFlags(true);
	CHECK(!HitBlocks(projectile, SolidPawn(), false));
}

TEST(BlockingMoverFlagDecidesForPlayer)
{
	// A player carries both flags, so only the mover's own flag can tell the two movers apart:
	// bBlockPlayers stops a player and bBlockActors does not.
	MovingActor player;
	player.isPlayerOrProjectile = true;
	player.collidesWorld = true;
	player.blocksPlayers = true;
	player.blocksActors = true;

	CHECK(HitBlocks(player, MoverBrush(true, false), false));
	CHECK(!HitBlocks(player, MoverBrush(false, true), false));
	CHECK(!HitBlocks(player, MoverBrush(false, false), false));
}

TEST(BlockingLevelAndBasedActors)
{
	// The level stops what collides with the world and nothing else.
	MovingActor mover = WorldColliderWithNoFlags(false);
	HitTarget level;
	CHECK(HitBlocks(mover, level, false));
	CHECK(!HitBlocks(MovingActor(), level, false));

	// What the moving actor stands on does not stop it, unless its own base is asked to; what stands
	// on the moving actor never stops it. A lift is no exception.
	MovingActor walker;
	walker.collidesWorld = true;
	walker.blocksActors = true;
	for (HitKind kind : { HitKind::Actor, HitKind::MoverBrush })
	{
		HitTarget below;
		below.kind = kind;
		below.blocksActors = true;
		below.isBaseOfMoving = true;
		CHECK(!HitBlocks(walker, below, false));
		CHECK(HitBlocks(walker, below, true));

		HitTarget above;
		above.kind = kind;
		above.blocksActors = true;
		above.isBasedOnMoving = true;
		CHECK(!HitBlocks(walker, above, false));
		CHECK(!HitBlocks(walker, above, true));
	}
}

TEST(BlockingMoverFlagDecidesForNonPlayerPawn)
{
	// A non-player pawn that blocks actors is stopped by a mover that blocks actors and not by one
	// that only blocks players: for a mover's brush the mover's flag decides.
	MovingActor pawn;
	pawn.collidesWorld = true;
	pawn.blocksActors = true;

	CHECK(HitBlocks(pawn, MoverBrush(false, true), false));
	CHECK(!HitBlocks(pawn, MoverBrush(true, false), false));
}
