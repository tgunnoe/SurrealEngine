"""The Unreal Gold 226b sweep scenario: every game-specific literal."""
from triage.scenario import Scenario

UNREAL = Scenario(
    label="Unreal Gold 226b",
    triage_intro=(
        "Non-clean maps: {nonclean} of {total}. A map is clean only when it passes, loaded\n"
        "the requested map, rendered, and logged no error line and no\n"
        "gameplay-class stub. Every non-clean map has a section. `SCRIPT_WARNING`\n"
        "lines are counted in each section and do not by themselves make a map\n"
        "non-clean. `fps_avg` is recorded per row in sweep.json, and a row below 50\n"
        "is weak evidence of absence: a map that rendered few frames had little\n"
        "chance to log an error or a stub.\n\n"),
    triage_evidence=(
        "Evidence for a row is its verdict line, `maps_loaded`, failing lines and\n"
        "warnings line (sweep.json, errors.json) and its stub list (stubs.json).\n\n"),
    stubs_footer=(
        "\nA stub is gameplay when its owner descends from one of the gameplay roots\n"
        "listed in scenarios/unreal/scenario.py. An owner missing from that\n"
        "table is unclassified and counts as gameplay until the table is\n"
        "extended.\n"),
    summary_footer=(
        "\nA map becomes clean only when every signature that lists it above is fixed.\n"),
    # Owner class -> superclass chain, read from the export tables of the Unreal
    # Gold 226b packages Core.u, Engine.u, UnrealShare.u and UnrealI.u in System/:
    # each class export (class index 0) carries its super class, an index into the
    # same package's exports or imports, and the first package to export a name
    # decides it. The chains were printed by a throwaway script kept outside the
    # repository, run as `chains.py "<Unreal Gold folder>/System" <ClassName>...`.
    # A class counts as gameplay when its chain meets a gameplay root. Decal, a
    # root in the shared set, is not exported by any of the four packages, so it
    # has no row.
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
        "ScriptedPawn": ["ScriptedPawn", "Pawn", "Actor", "Object"],
        "Triggers": ["Triggers", "Actor", "Object"],
        "Effects": ["Effects", "Actor", "Object"],
        "Carcass": ["Carcass", "Decoration", "Actor", "Object"],
        "Teleporter": ["Teleporter", "NavigationPoint", "Actor", "Object"],
        "Keypoint": ["Keypoint", "Actor", "Object"],
        "Info": ["Info", "Actor", "Object"],
        "Console": ["Console", "Object"],
        "PlayerReplicationInfo": ["PlayerReplicationInfo", "ReplicationInfo", "Info", "Actor", "Object"],
        "ReplicationInfo": ["ReplicationInfo", "Info", "Actor", "Object"],
        "GameReplicationInfo": ["GameReplicationInfo", "ReplicationInfo", "Info", "Actor", "Object"],
        "Spectator": ["Spectator", "PlayerPawn", "Pawn", "Actor", "Object"],
        "Mutator": ["Mutator", "Info", "Actor", "Object"],
    },
    gameplay_roots=frozenset({
        "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
        "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
        "LevelInfo", "ZoneInfo", "Canvas", "HUD", "Brush", "Light", "Pickup",
        "Ammo", "ScriptedPawn", "Triggers", "Effects", "Carcass", "Teleporter",
        "Keypoint", "Info", "Decal", "Console", "PlayerReplicationInfo",
        "ReplicationInfo", "GameReplicationInfo", "Spectator", "Mutator",
    }),
    stub_owner_file={
        "Actor": "SurrealEngine/Native/NActor.cpp",
        "Canvas": "SurrealEngine/Native/NCanvas.cpp",
        "Console": "SurrealEngine/Native/NConsole.cpp",
        "GameInfo": "SurrealEngine/Native/NGameInfo.cpp",
        "LevelInfo": "SurrealEngine/Native/NLevelInfo.cpp",
        "NavigationPoint": "SurrealEngine/Native/NNavigationPoint.cpp",
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
    repro_logdir="/tmp/repro-unreal",
    row_count=55,
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
