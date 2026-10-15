# Triage: Unreal Gold 226b, 60 s per map

Nine of 55 maps are not clean; all nine fail for one reason (see `SUMMARY.md`). Error lines are verbatim from the logs with the trailing native call stack removed (it holds local addresses and build paths). `reproduced:` follows the sweep's rule: `yes` = same first failing line; `flaky` = a different first failing line; `no` = second run clean.

Mechanism: the native operator `SubtractEqual_FloatFloat` is handed a None out-parameter (`PlayerReplicationInfo` of a pawn that has none). `ExpressionValue::CheckType` throws "Accessed None" (`SurrealEngine/VM/ExpressionValue.h`, lines 186-188); the catch in `Frame::CallNative` (`SurrealEngine/VM/Frame.cpp`, lines 318 and 345) logs "Script error: " plus the message, which the smoke log records as `SCRIPT_ERROR`, and returns `NothingValue`, so the score is unchanged and the script continues. The statement-level case (`StatementResult::AccessedNone`, `Frame.cpp` lines 591-593) logs a plain "Accessed None" without that prefix, so the smoke log never records it.

Victims: the recorded stacks are not the idle player. 13 of the 18 recorded lines are `ScriptedPawn.Retreating.TakeDamage` or `FallingState` into `Pawn.Died` for Nali killed by Krall, Skaarj, Slith, Titan or their projectiles; 4 are deaths with no killer (drowning, falling out of the world, a scripted damage trigger).

Script facts, as supplied by review of the game scripts (not re-read by this sweep): `SinglePlayer.Killed` (UnrealShare.u) runs `if ( Other.IsA('Nali') ) killer.PlayerReplicationInfo.Score -= 2;` with no guard; `GameInfo.ScoreKill` (Engine.u) runs `if( (killer == Other) || (killer == None) ) Other.PlayerReplicationInfo.Score -= 1;` with no guard; `GameInfo.Killed` calls `ScoreKill` unconditionally; a `PlayerReplicationInfo` is spawned only for `bIsPlayer` pawns. Monsters and Nali therefore have none in the original game either, and stock script reaches the same None. The behaviour looks faithful.

Open question for the project owner: whether a faithful `Accessed None` should count against clean. The engine-side defect is that the same condition is reported differently depending on whether the None is reached as a native argument (`SCRIPT_ERROR`) or in a statement or call target (an unprefixed log line the harness never sees). `Docs/Status.md` has no matching entry.

## Bluff

verdict: fail, exit 0 (script_errors=7, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["Bluff"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at GameInfo.ScoreKill line 1001: SubtractEqual_FloatFloat(Other.PlayerReplicationInfo.Score, intToFloat(1)) at GameInfo.Killed line 923: ScoreKill(Killer, Other) at SinglePlayer.Killed line 20: Killed(Killer, Other, DamageType) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.FellOutOfWorld line 353: Died(null, fell, Location)
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Krall.ThrowDamageTarget line 888: AttackSuccess = MeleeDamageTarget(byteToInt(ThrowDamage), { 0.000000, 0.000000, 0.000000 })
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at KraalBolt.Flying.ProcessTouch line 49: Other.TakeDamage(floatToInt(Damage), Instigator, HitLocation, Momentum, zapped) at Projectile.Touch line 49: ProcessTouch(Other, HitLocation)
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at SkaarjProjectile.Flying.ProcessTouch line 23: Other.TakeDamage(floatToInt(Damage), Instigator, HitLocation, Momentum, zapped) at Projectile.Touch line 49: ProcessTouch(Other, HitLocation)
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at GameInfo.ScoreKill line 1001: SubtractEqual_FloatFloat(Other.PlayerReplicationInfo.Score, intToFloat(1)) at GameInfo.Killed line 923: ScoreKill(Killer, Other) at SinglePlayer.Killed line 20: Killed(Killer, Other, DamageType) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at Pawn.PainTimer line 1512: TakeDamage(5, null, Location, { 0.000000, 0.000000, 0.000000 }, drowned)
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/Bluff.log --url=Bluff "/path/to/Unreal Gold"

reproduced: yes (exit 0)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## Ceremony

verdict: fail, exit 0 (script_errors=7, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["Ceremony"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Skaarj.ClawDamageTarget line 697: if (MeleeDamageTarget(byteToInt(ClawDamage), Multiply_FloatVector(intToFloat(Multiply_IntInt(byteToInt(ClawDamage), 900)), Normal(Subtract_VectorVector(Target.Location, Location)))))
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at Actor.HurtRadius line 852: Victims.TakeDamage(floatToInt(Multiply_FloatFloat(damageScale, DamageAmount)), Instigator, Subtract_VectorVector(Victims.Location, Multiply_FloatVector(Multiply_FloatFloat(0.500000, Add_FloatFloat(Victims.CollisionHeight, Victims.CollisionRadius)), Dir)), Multiply_FloatVector(Multiply_FloatFloat(damageScale, Momentum), Dir), DamageName) at SlithProjectile.Explode line 49: HurtRadius(Multiply_FloatFloat(Damage, DrawScale), Multiply_FloatFloat(DrawScale, intToFloat(200)), Corroded, intToFloat(MomentumTransfer), HitLocation) at SlithProjectile.Flying.ProcessTouch line 57: Explode(HitLocation, { 0.000000, 0.000000, 0.000000 }) at Projectile.Touch line 49: ProcessTouch(Other, Add_VectorVector(Other.Location, Multiply_FloatVector(Other.CollisionRadius, Normal(Subtract_VectorVector(Location, Other.Location)))))
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at GameInfo.ScoreKill line 1001: SubtractEqual_FloatFloat(Other.PlayerReplicationInfo.Score, intToFloat(1)) at GameInfo.Killed line 923: ScoreKill(Killer, Other) at SinglePlayer.Killed line 20: Killed(Killer, Other, DamageType) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.TacticalMove.TakeDamage line 3680: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at SpecialEvent.DamageInstigator.Trigger line 48: Other.TakeDamage(Damage, EventInstigator, EventInstigator.Location, { 0.000000, 0.000000, 0.000000 }, DamageType) at Trigger.Touch line 213: A.Trigger(Other, Other.Instigator)
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at SkaarjProjectile.Flying.ProcessTouch line 23: Other.TakeDamage(floatToInt(Damage), Instigator, HitLocation, Momentum, zapped) at Projectile.Touch line 49: ProcessTouch(Other, HitLocation)
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at GameInfo.ScoreKill line 1001: SubtractEqual_FloatFloat(Other.PlayerReplicationInfo.Score, intToFloat(1)) at GameInfo.Killed line 923: ScoreKill(Killer, Other) at SinglePlayer.Killed line 20: Killed(Killer, Other, DamageType) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at Pawn.PainTimer line 1512: TakeDamage(5, null, Location, { 0.000000, 0.000000, 0.000000 }, drowned)
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/Ceremony.log --url=Ceremony "/path/to/Unreal Gold"

reproduced: flaky (exit 0) (first line differs at character 448: the shared prefix is the same signature; the divergence is in the deeper script call stack, i.e. which pawn or state delivered the damage; run 1 `..., HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(`, run 2 `..., HitLocation, Momentum, DamageType) at Actor.HurtRadius line 852: Victims.TakeDamage(floatToInt(Mul`)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## Chizra

verdict: fail, exit 0 (script_errors=2, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["Chizra"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Slith.ClawDamageTarget line 351: MeleeDamageTarget(byteToInt(ClawDamage), Multiply_FloatVector(Multiply_FloatFloat(byteToFloat(ClawDamage), 1000.000000), Normal(Subtract_VectorVector(Target.Location, Location))))
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/Chizra.log --url=Chizra "/path/to/Unreal Gold"

reproduced: yes (exit 0)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## Dig

verdict: fail, exit 0 (script_errors=2, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["Dig"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at TentacleProjectile.Flying.ProcessTouch line 22: Other.TakeDamage(floatToInt(Damage), Instigator, HitLocation, Momentum, stung) at Projectile.Touch line 49: ProcessTouch(Other, HitLocation)
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/Dig.log --url=Dig "/path/to/Unreal Gold"

reproduced: flaky (exit 0) (first line differs at character 81: the shared prefix is the same signature; the divergence is in the deeper script call stack, i.e. which pawn or state delivered the damage; run 1 `...ct.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer`, run 2 `...ct.SubtractEqual_FloatFloat line 175 at GameInfo.ScoreKill line 1001: SubtractEqual_FloatFloat(Other`)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## Dug

verdict: fail, exit 0 (script_errors=1, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["Dug"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Skaarj.SpinDamageTarget line 691: if (MeleeDamageTarget(byteToInt(SpinDamage), Multiply_FloatVector(intToFloat(Multiply_IntInt(byteToInt(SpinDamage), 1000)), Normal(Subtract_VectorVector(Target.Location, Location)))))
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/Dug.log --url=Dug "/path/to/Unreal Gold"

reproduced: flaky (exit 0) (first line differs at character 344: the shared prefix is the same signature; the divergence is in the deeper script call stack, i.e. which pawn or state delivered the damage; run 1 `...amageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigat`, run 2 `...amageType, HitLocation) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, th`)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## SkyCaves

verdict: fail, exit 0 (script_errors=1, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["SkyCaves"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Krall.PoundDamageTarget line 882: if (MeleeDamageTarget(byteToInt(PoundDamage), Multiply_FloatVector(intToFloat(Multiply_IntInt(byteToInt(PoundDamage), 500)), Normal(Subtract_VectorVector(Target.Location, Location)))))
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/SkyCaves.log --url=SkyCaves "/path/to/Unreal Gold"

reproduced: yes (exit 0)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## SkyTown

verdict: fail, exit 0 (script_errors=3, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["SkyTown"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.MeleeAttack.TakeDamage line 4886: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Krall.StrikeDamageTarget line 876: if (MeleeDamageTarget(byteToInt(StrikeDamage), Multiply_FloatVector(intToFloat(Multiply_IntInt(byteToInt(StrikeDamage), 700)), Normal(Subtract_VectorVector(Target.Location, Location)))))
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at KraalBolt.Flying.ProcessTouch line 49: Other.TakeDamage(floatToInt(Damage), Instigator, HitLocation, Momentum, zapped) at Projectile.Touch line 49: ProcessTouch(Other, HitLocation)
```

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Krall.ThrowDamageTarget line 888: AttackSuccess = MeleeDamageTarget(byteToInt(ThrowDamage), { 0.000000, 0.000000, 0.000000 })
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/SkyTown.log --url=SkyTown "/path/to/Unreal Gold"

reproduced: flaky (exit 0) (first line differs at character 565: the shared prefix is the same signature; the divergence is in the deeper script call stack, i.e. which pawn or state delivered the damage; run 1 `... HitLocation, pushdir, hacked) at Krall.StrikeDamageTarget line 876: if (MeleeDamageTarget(byteToInt`, run 2 `... HitLocation, pushdir, hacked) at Krall.ThrowDamageTarget line 888: AttackSuccess = MeleeDamageTarge`)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## SpireVillage

verdict: fail, exit 0 (script_errors=1, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["SpireVillage"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Titan.SlapDamageTarget line 472: if (MeleeDamageTarget(byteToInt(SlapDamage), Multiply_FloatVector(70000.000000, Add_VectorVector(Y, { 0.000000, 0.000000, 1.000000 }))))
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/SpireVillage.log --url=SpireVillage "/path/to/Unreal Gold"

reproduced: yes (exit 0)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.

## TheSunspire

verdict: fail, exit 0 (script_errors=1, stub_hits=0, missing_natives=0, exceptions=0)

maps_loaded: ["TheSunspire"]

First distinct failing lines (log order):

```
SCRIPT_ERROR Accessed None  Script call stack: at Object.SubtractEqual_FloatFloat line 175 at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2)) at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType) at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation) at ScriptedPawn.FallingState.TakeDamage line 4662: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType) at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked) at Skaarj.SpinDamageTarget line 691: if (MeleeDamageTarget(byteToInt(SpinDamage), Multiply_FloatVector(intToFloat(Multiply_IntInt(byteToInt(SpinDamage), 1000)), Normal(Subtract_VectorVector(Target.Location, Location)))))
```

repro: SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/TheSunspire.log --url=TheSunspire "/path/to/Unreal Gold"

reproduced: flaky (exit 0) (first line differs at character 344: the shared prefix is the same signature; the divergence is in the deeper script call stack, i.e. which pawn or state delivered the damage; run 1 `...amageType, HitLocation) at ScriptedPawn.FallingState.TakeDamage line 4662: TakeDamage(Damage, instig`, run 2 `...amageType, HitLocation) at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigat`)

owner: engine VM native-call error path (`SurrealEngine/VM/ExpressionValue.h` CheckType, `SurrealEngine/VM/Frame.cpp` CallNative catch) reporting a None argument of `SubtractEqual_FloatFloat` from script `SinglePlayer.Killed`/`GameInfo.ScoreKill`; reason: the script frames name those functions and the line carries the "Script error: " path; the None itself is faithful stock-script behaviour (see preamble).

Status.md: no matching entry.
