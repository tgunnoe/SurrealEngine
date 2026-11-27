"""Class tables shared by the two Unreal Tournament 436 sweep scenarios.

The botmatch and the no-bots sweeps run the same game and the same packages,
so they classify stubs from the same four tables.
"""

# HIERARCHY: owner class -> superclass chain, read from the export tables of the
# UT 436 packages in System/ (each class export carries its super class). A class
# counts as gameplay when its chain meets a gameplay root. Owners that descend
# from Actor but are left out on purpose (StatLog, StatLogFile, UdpLink, TcpLink,
# InternetLink): this rule would call them gameplay, and a stub in one is better
# shown as unclassified than counted silently.
HIERARCHY = {
    "Actor": ["Actor", "Object"],
    "Pawn": ["Pawn", "Actor", "Object"],
    "PlayerPawn": ["PlayerPawn", "Pawn", "Actor", "Object"],
    "Inventory": ["Inventory", "Actor", "Object"],
    "Weapon": ["Weapon", "Inventory", "Actor", "Object"],
    "Projectile": ["Projectile", "Actor", "Object"],
    "Decoration": ["Decoration", "Actor", "Object"],
    "Mover": ["Mover", "Brush", "Actor", "Object"],
    "Trigger": ["Trigger", "Triggers", "Actor", "Object"],
    "NavigationPoint": ["NavigationPoint", "Actor", "Object"],
    "GameInfo": ["GameInfo", "Info", "Actor", "Object"],
    "LevelInfo": ["LevelInfo", "ZoneInfo", "Info", "Actor", "Object"],
    "ZoneInfo": ["ZoneInfo", "Info", "Actor", "Object"],
    "Canvas": ["Canvas", "Object"],
    "HUD": ["HUD", "Actor", "Object"],
    "Brush": ["Brush", "Actor", "Object"],
    "Light": ["Light", "Actor", "Object"],
    "Pickup": ["Pickup", "Inventory", "Actor", "Object"],
    "Ammo": ["Ammo", "Pickup", "Inventory", "Actor", "Object"],
    "Bot": ["Bot", "Pawn", "Actor", "Object"],
    "Scout": ["Scout", "Pawn", "Actor", "Object"],
    "Spawnpoint": ["Spawnpoint", "NavigationPoint", "Actor", "Object"],
    "PlayerStart": ["PlayerStart", "NavigationPoint", "Actor", "Object"],
    "Triggers": ["Triggers", "Actor", "Object"],
    "Effects": ["Effects", "Actor", "Object"],
    "Carcass": ["Carcass", "Decoration", "Actor", "Object"],
    "Teleporter": ["Teleporter", "NavigationPoint", "Actor", "Object"],
    "Keypoint": ["Keypoint", "Actor", "Object"],
    "Info": ["Info", "Actor", "Object"],
    "Decal": ["Decal", "Actor", "Object"],
    "Console": ["Console", "Object"],
    "PlayerReplicationInfo": ["PlayerReplicationInfo", "ReplicationInfo", "Info", "Actor", "Object"],
    "ReplicationInfo": ["ReplicationInfo", "Info", "Actor", "Object"],
    "GameReplicationInfo": ["GameReplicationInfo", "ReplicationInfo", "Info", "Actor", "Object"],
    "Spectator": ["Spectator", "PlayerPawn", "Pawn", "Actor", "Object"],
    "Mutator": ["Mutator", "Info", "Actor", "Object"],
    "ScriptedPawn": ["ScriptedPawn", "Pawn", "Actor", "Object"],
    "Commandlet": ["Commandlet", "Object"],
    "Engine": ["Engine", "Subsystem", "Object"],
    "Object": ["Object"],
    "Subsystem": ["Subsystem", "Object"],
    "Player": ["Player", "Object"],
    "Palette": ["Palette", "Object"],
    "Texture": ["Texture", "Bitmap", "Object"],
    "Time": ["Time", "Object"],
}
GAMEPLAY_ROOTS = frozenset({
    "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
    "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
    "LevelInfo", "ZoneInfo", "Canvas", "HUD", "Brush", "Light", "Pickup",
    "Ammo", "Bot", "Scout", "Spawnpoint", "PlayerStart", "Triggers",
    "Effects", "Carcass", "Teleporter", "Keypoint", "Info", "Decal",
    "Console", "PlayerReplicationInfo", "ReplicationInfo",
    "GameReplicationInfo", "Spectator", "Mutator", "Projector",
    "ScriptedPawn", "Weapons", "ParticleIterator",
})
BARE_OWNERS = {"GetNetworkNumber": "GameInfo"}
STUB_OWNER_FILE = {
    "Actor": "SurrealEngine/Native/NActor.cpp",
    "Canvas": "SurrealEngine/Native/NCanvas.cpp",
    "Commandlet": "SurrealEngine/Native/NCommandlet.cpp",
    "Console": "SurrealEngine/Native/NConsole.cpp",
    "Decal": "SurrealEngine/Native/NDecal.cpp",
    "GameInfo": "SurrealEngine/Native/NGameInfo.cpp",
    "LevelInfo": "SurrealEngine/Native/NLevelInfo.cpp",
    "NavigationPoint": "SurrealEngine/Native/NNavigationPoint.cpp",
    "Object": "SurrealEngine/Native/NObject.cpp",
    "Pawn": "SurrealEngine/Native/NPawn.cpp",
    "PlayerPawn": "SurrealEngine/Native/NPlayerPawn.cpp",
    "ZoneInfo": "SurrealEngine/Native/NZoneInfo.cpp",
}
