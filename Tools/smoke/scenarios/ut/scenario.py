"""The Unreal Tournament 436 sweep scenario without bots: every game-specific literal."""
from triage.scenario import Scenario

UT = Scenario(
    label="UT 436",
    triage_intro=(
        "Non-clean maps: {nonclean} of {total}. A map is clean only when it passes, loaded\n"
        "the requested map, rendered, and logged no error line and no\n"
        "gameplay-class stub. Every non-clean map has a section. `SCRIPT_WARNING`\n"
        "lines are counted in each section and do not by themselves make a map\n"
        "non-clean. Every map ran for 60 seconds with no bots and no fire press,\n"
        "in the game type the map itself selects, so `match_started` is not\n"
        "counted. `fps_avg` is recorded per row in sweep.json, and a row below 50\n"
        "is weak evidence of absence: a map that rendered few frames had little\n"
        "chance to log an error or a stub.\n\n"),
    triage_evidence=(
        "Evidence for a row is its verdict line, `maps_loaded`, failing lines and\n"
        "warnings line (sweep.json, errors.json) and its stub list (stubs.json).\n\n"),
    stubs_footer=(
        "\nA stub is gameplay when its owner descends from one of the gameplay roots\n"
        "listed in scenarios/ut/scenario.py. An owner missing from that\n"
        "table is unclassified and counts as gameplay until the table is\n"
        "extended.\n"),
    summary_footer=(
        "\nA map becomes clean only when every signature that lists it above is fixed.\n"),
    # The hierarchy, gameplay roots, bare owners and stub owner files below are
    # copied from scenarios/ut-bots/scenario.py: same game, same packages. The
    # two scenarios must be kept in step, with the botmatch one as the source.
    #
    # Owner class -> superclass chain, read from the export tables of the UT 436
    # packages in System/ (each class export carries its super class). A class
    # counts as gameplay when its chain meets a gameplay root. Owners that
    # descend from Actor but are left out on purpose, as in the source
    # (StatLog, StatLogFile, UdpLink, TcpLink, InternetLink): this rule would
    # call them gameplay, and a stub in one is better shown as unclassified
    # than counted silently.
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
    signature_rules=(
        (None, "contains", "Failed to spawn player actor",
         "login: Failed to spawn player actor"),
        (None, "contains", "Unsupported expression value property type",
         "script: Unsupported expression value property type"),
        ("FATAL", "startswith", "signal", "crash: FATAL signal {last}"),
    ),
    bullet="- ",
    status_label="Status.md",
    verdict_template="verdict: {verdict}, exit code {exit_code}",
    reproduced_template=("reproduced: {reproduced} (second run exit code {exit_code}; "
                         "run1 first line `{run1}`; run2 first line `{run2}`)"),
    counts_template=("Maps: {maps}. clean: {clean} of {maps}. fail: {fail}. crash: {crash}. "
                     "timeout: {timeout}. pass-verdict but non-clean: {stub_only}. "
                     "warnings: {warnings}."),
    exe="SurrealEngine",
    game_placeholder="<game folder>",
    exit_after_seconds=60,
    repro_timeout_seconds=105,
    repro_logdir="/tmp/repro-ut",
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
