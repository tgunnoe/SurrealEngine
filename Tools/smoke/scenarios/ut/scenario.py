"""The Unreal Tournament 436 sweep scenario without bots: every game-specific literal."""
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

UT = Scenario(
    label="UT 436",
    triage_intro=(
        "Non-clean maps: {nonclean} of {total}. A map is clean only when it passes, loaded\n"
        "the requested map, rendered, and logged no error line and no\n"
        "gameplay-class stub. Every non-clean map has a section. `SCRIPT_WARNING`\n"
        "lines are counted in each section and do not by themselves make a map\n"
        "non-clean. The sweep used no bots and no fire press, so `match_started`\n"
        "is not counted. `fps_avg` is recorded per row in sweep.json, and a row\n"
        "below 50 is weak evidence of absence: a map that rendered few frames had\n"
        "little chance to log an error or a stub.\n\n"),
    triage_evidence=(
        "Evidence for a row is its verdict line, `maps_loaded`, failing lines and\n"
        "warnings line (sweep.json, errors.json) and its stub list (stubs.json).\n\n"),
    stubs_footer=(
        "\nA stub is gameplay when its owner descends from one of the gameplay roots\n"
        "listed in scenarios/ut436_tables.py. An owner missing from that\n"
        "table is unclassified and counts as gameplay until the table is\n"
        "extended.\n"),
    summary_footer=(
        "\nA map becomes clean only when every signature that lists it above is fixed.\n"),
    hierarchy=HIERARCHY,
    gameplay_roots=GAMEPLAY_ROOTS,
    bare_owners=BARE_OWNERS,
    stub_owner_file=STUB_OWNER_FILE,
    signature_rules=(
        (None, "contains", "Failed to spawn player actor",
         "login: Failed to spawn player actor"),
        (None, "contains", "Unsupported expression value property type",
         "script: Unsupported expression value property type"),
    ),
    status_natives=_STATUS,
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
