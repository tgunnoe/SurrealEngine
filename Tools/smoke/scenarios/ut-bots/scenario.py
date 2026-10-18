"""The Unreal Tournament 436 botmatch sweep scenario: every game-specific literal."""
from triage.scenario import Scenario

_STATUS = ("Docs/Status.md line 70 (Unreal Tournament: '436 version of the game launches. Menu options "
           "will work and botmatches can be played. Bots will behave more or less like they do in "
           "Unreal and some maps might have some functionality missing.')")

UTBOTS = Scenario(
    label="UT 436 botmatch",
    triage_intro=(
        "Non-clean maps: {nonclean} of {total}. A map is clean only when it passes, loaded\n"
        "the requested map, rendered, logged no error line and no gameplay-class\n"
        "stub, and its match started: the player's state was `PlayerWalking` when\n"
        "the fire press ran five seconds after login, or became `PlayerWalking`\n"
        "afterwards. Every map ran with four bots (`?NumBots=4`) in the game type\n"
        "the map itself selects, including maps that are not matches. `fps_avg` is\n"
        "not evidence in this sweep: every run rendered at about one frame per\n"
        "second on a shared host.\n\n"),
    triage_evidence=(
        "Evidence for a row is its verdict line, `maps_loaded`, failing lines and\n"
        "`match_started` line (sweep.json, errors.json) and its stub list\n"
        "(stubs.json).\n\n"),
    stubs_footer=(
        "\nA stub is gameplay when its owner descends from one of the gameplay roots\n"
        "listed in scenarios/ut-bots/scenario.py. An owner missing from that\n"
        "table is unclassified and counts as gameplay until the table is\n"
        "extended.\n"),
    summary_footer=(
        "\nCandidates are ranked by the maps they fully unblock. The\n"
        "`match did not start` signature is an observation about the run, not a\n"
        "located defect.\n"),
    # Owner class -> superclass chain, read from the export tables of the UT 436
    # packages in System/ (each class export carries its super class). A class
    # counts as gameplay when its chain meets a gameplay root. Owners the
    # no-bots UT triage listed as non-gameplay by name but that descend from
    # Actor (StatLog, StatLogFile, UdpLink, TcpLink, InternetLink) are left out
    # on purpose: this rule would call them gameplay, and a stub in one is
    # better shown as unclassified than counted silently.
    hierarchy={
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
    },
    gameplay_roots=frozenset({
        "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
        "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
        "LevelInfo", "ZoneInfo", "Canvas", "HUD", "Brush", "Light", "Pickup",
        "Ammo", "Bot", "Scout", "Spawnpoint", "PlayerStart", "Triggers",
        "Effects", "Carcass", "Teleporter", "Keypoint", "Info", "Decal",
        "Console", "PlayerReplicationInfo", "ReplicationInfo",
        "GameReplicationInfo", "Spectator", "Mutator", "Projector",
        "ScriptedPawn", "Weapons", "ParticleIterator",
    }),
    bare_owners={"GetNetworkNumber": "GameInfo"},
    stub_owner_file={
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
    },
    error_owner={
        "match did not start": (
            "cause: the map's DefaultGameType is UTIntro, whose Login hides the player "
            "and views through a SpectatorCam and which has no match to start (BotPack.u, "
            "class UTIntro); the map is a cinematic, not a botmatch; the player's state "
            "is `None` from within two seconds of login, so the fire press finds no "
            "state to act on",
            "none applies"),
    },
    status_natives=_STATUS,
    bullet="- ",
    status_label="Status.md",
    verdict_template="verdict: {verdict}, exit code {exit_code}",
    reproduced_template=("reproduced: {reproduced} (second run exit code {exit_code}; "
                         "run1 first line `{run1}`; run2 first line `{run2}`)"),
    counts_template=("Maps: {maps}. clean: {clean} of {maps}. fail: {fail}. crash: {crash}. "
                     "timeout: {timeout}. pass-verdict but non-clean: {stub_only}. "
                     "warnings: {warnings}. matches started: {matches_started} of {maps}."),
    exe="SurrealEngine",
    game_placeholder="<game folder>",
    exit_after_seconds=60,
    repro_timeout_seconds=105,
    repro_logdir="/tmp/repro-ut-bots",
    url_options=("?NumBots=4",),
    exec="5:Fire",
    require_match_start=True,
    row_count=83,
    screenshots_null=True,
    errors_in_section=True,
    gameplay_stubs_in_section=True,
    summary_counts_line=True,
    stubs_md_count=True,
    require_first_five=True,
    reproduced_exit_code=True,
    maps_loaded_equal=True,
    verdict_line_equal=True,
    map_match_case_insensitive=True,
)
