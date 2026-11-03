"""The Unreal Tournament 436 botmatch sweep scenario: every game-specific literal."""
import os
import sys

from triage.scenario import Scenario

# The tables module sits beside the scenario directories; put its directory on the
# path here so this file imports under the entry point and under the tests alike.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from ut436_tables import BARE_OWNERS, GAMEPLAY_ROOTS, HIERARCHY, STUB_OWNER_FILE  # noqa: E402

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
        "afterwards. Every map was launched with `?NumBots=4`, including maps that\n"
        "are not matches, whose game types may ignore it, and the AS-, CTF- and\n"
        "DOM- maps under the game their prefix names (Assault, CTFGame,\n"
        "Domination), except CTF-Tutorial, which keeps the training game it\n"
        "stores. `fps_avg` is recorded per row in sweep.json, and a row below 50\n"
        "is weak evidence of absence: a map that rendered few frames had little\n"
        "chance to log an error or a stub.\n\n"),
    triage_evidence=(
        "Evidence for a row is its verdict line, `maps_loaded`, failing lines and\n"
        "`match_started` line (sweep.json, errors.json) and its stub list\n"
        "(stubs.json).\n\n"),
    stubs_footer=(
        "\nA stub is gameplay when its owner descends from one of the gameplay roots\n"
        "listed in scenarios/ut436_tables.py. An owner missing from that\n"
        "table is unclassified and counts as gameplay until the table is\n"
        "extended.\n"),
    summary_footer=(
        "\nCandidates are ranked by the maps they fully unblock. The\n"
        "`match did not start` signature is an observation about the run, not a\n"
        "located defect.\n"),
    hierarchy=HIERARCHY,
    gameplay_roots=GAMEPLAY_ROOTS,
    bare_owners=BARE_OWNERS,
    stub_owner_file=STUB_OWNER_FILE,
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
    url_game_by_prefix=(("AS", "Botpack.Assault"), ("CTF", "Botpack.CTFGame"),
                        ("DOM", "Botpack.Domination")),
    url_game_except=("CTF-Tutorial",),
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
    warning_register="../WARNINGS.md",
    register_heading="ut-bots",
)
