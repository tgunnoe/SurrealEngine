"""The Deus Ex 1112fm sweep scenario: every game-specific literal."""
from triage.scenario import Scenario

_STATUS_NATIVES = "Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')"

DEUSEX = Scenario(
    label="Deus Ex 1112fm",
    triage_intro=(
        "Non-clean maps: {nonclean} of {total}. A map is clean only when it passes, loaded\n"
        "the requested map, rendered, and logged no error line and no\n"
        "gameplay-class stub. Every non-clean map has a section. Other engine\n"
        "runs shared the display during this sweep, so `fps_avg` is not evidence.\n\n"),
    triage_evidence=(
        "Evidence for a stub-only row is its stub list (stubs.json) and its\n"
        "exit code and SUMMARY-bearing pass verdict (sweep.json).\n\n"),
    stubs_footer=(
        "\nClassification follows the gameplay-class definition in the run\n"
        "boundary: a stub is gameplay when its owner descends from Actor,\n"
        "Pawn, PlayerPawn, LevelInfo or another listed root. DumpLocation\n"
        "descends only from Object, so it is non-gameplay by that definition\n"
        "even though gameplay script calls it.\n"),
    summary_footer=(
        "\nA map becomes clean only when every signature that lists it above is fixed.\n"),
    # Owner class -> superclass chain, read from the export tables of the Deus Ex
    # 1112fm packages (Engine.u, DeusEx.u, Core.u): each class export carries its
    # super class. Classification needs the chain because a class counts as
    # gameplay when it descends from one of the gameplay roots.
    hierarchy={
        "Actor": ["Actor", "Object"],
        "DumpLocation": ["DumpLocation", "Object"],
        "LevelInfo": ["LevelInfo", "ZoneInfo", "Info", "Actor", "Object"],
        "Pawn": ["Pawn", "Actor", "Object"],
        "PlayerPawn": ["PlayerPawn", "Pawn", "Actor", "Object"],
    },
    gameplay_roots=frozenset({
        "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
        "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
        "LevelInfo", "ZoneInfo", "Canvas", "HUD",
    }),
    stub_owner_file={
        "LevelInfo": "SurrealEngine/Native/NLevelInfo.cpp",
        "Actor": "SurrealEngine/Native/NActor.cpp",
        "Pawn": "SurrealEngine/Native/NPawn.cpp",
        "PlayerPawn": "SurrealEngine/Native/NPlayerPawn.cpp",
        "DumpLocation": "SurrealEngine/Native/NDumpLocation.cpp",
    },
    signature_rules=(
        (None, "contains", "Failed to spawn player actor",
         "login: Failed to spawn player actor"),
        (None, "contains", "Iterator statement without an iterator",
         "script: Iterator statement without an iterator (ReachablePathnodes)"),
    ),
    error_owner={
        "login: Failed to spawn player actor": (
            "SurrealEngine/Engine.cpp (login path, throws at line 1004) and the "
            "DeusEx GameInfo.Login script that spawns the player; the exception "
            "text carries no script cause, so the failing spawn step is not yet "
            "identified",
            "Docs/Status.md line 41 ('Sometimes opening a map crashes the engine with a \"Failed to spawn the player actor\" error.')"),
        "script: Iterator statement without an iterator (ReachablePathnodes)": (
            "SurrealEngine/Native/NPawn.cpp NPawn::ReachablePathnodes (line 332) "
            "is a LogUnimplemented stub that never sets up the iterator, so "
            "SurrealEngine/VM/Frame.cpp line 569 throws; this log also holds "
            "'STUB Pawn.ReachablePathnodes'",
            _STATUS_NATIVES),
        "crash: FATAL signal 11": (
            "unknown native or VM subsystem: the logs show SIGSEGV after LOGIN "
            "with no SCRIPT_ERROR, MISSING_NATIVE or EXCEPTION line before it, "
            "and the map did not crash when re-run alone, so the faulting frame "
            "is not identified",
            "none applies"),
    },
    status_natives=_STATUS_NATIVES,
    bullet="- ",
    status_label="Status.md",
    verdict_template="verdict: {verdict}, exit code {exit_code}",
    reproduced_template=("reproduced: {reproduced} (second run exit code {exit_code}; "
                         "run1 first line `{run1}`; run2 first line `{run2}`)"),
    counts_template=("Maps: {maps}. clean: {clean}. fail: {fail}. crash: {crash}. "
                     "timeout: {timeout}. pass-verdict but non-clean (stub-only): {stub_only}. "
                     "warnings: {warnings}."),
    exe="SurrealEngine",
    game_placeholder="<game folder>",
    exit_after_seconds=60,
    repro_timeout_seconds=105,
    repro_logdir="/tmp/repro-deusex",
    row_count=88,
    screenshots_null=True,
    errors_in_section=True,
    summary_counts_line=True,
    stubs_md_count=True,
)
