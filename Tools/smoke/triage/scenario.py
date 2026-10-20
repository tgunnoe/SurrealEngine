"""Everything that differs between games, as data.

No default names a game: a scenario that leaves a field unset gets an
empty value and the corresponding output is empty or the check is off.
"""
import string
from dataclasses import dataclass, field

# The engine folds only the ASCII letters when it compares names, so str.upper() would
# match prefixes it keeps apart.
ASCII_UPPER = str.maketrans(string.ascii_lowercase, string.ascii_uppercase)


def game_text(pairs, name):
    """The `?game=<class>` text for a map, or "" when no prefix of `pairs` matches.

    `pairs` holds (prefix, class) pairs. A map's prefix is the part of its name before the
    first "-" (a name with no "-" has none and never matches), compared without regard to
    ASCII case. The sweep and a re-run both build the URL through this, so they cannot
    name different games.
    """
    prefix, dash, _ = name.partition("-")
    if not dash:
        return ""
    folded = prefix.translate(ASCII_UPPER)
    for wanted, game in pairs:
        if wanted.translate(ASCII_UPPER) == folded:
            return "?game=" + game
    return ""


@dataclass(frozen=True)
class Scenario:
    # Titles and prose. Templates use str.format fields noted per field.
    label: str = ""                  # "<label> sweep triage" etc.
    triage_intro: str = ""           # fields: nonclean, total
    triage_evidence: str = ""
    stubs_footer: str = ""
    summary_footer: str = ""

    # Classification.
    hierarchy: dict = field(default_factory=dict)       # owner -> chain
    gameplay_roots: frozenset = frozenset()
    bare_owners: dict = field(default_factory=dict)     # bare stub name -> owner
    stub_owner_file: dict = field(default_factory=dict)  # owner -> source file

    # Error attribution.
    # (kind or None, "contains" | "startswith", needle, signature); the
    # signature may use {last}, the final word of the error text. FATAL
    # lines are never matched: a crash row has one signature derived from its
    # sweep row, and a FATAL line in any other row reads "FATAL: <text>".
    signature_rules: tuple = ()
    error_owner: dict = field(default_factory=dict)     # signature -> (owner, cite)
    status_natives: str = ""         # cite for unimplemented natives

    # Document formats. Fixed lines are prefixed with bullet.
    bullet: str = ""
    status_label: str = ""
    verdict_template: str = ""       # fields: verdict, exit_code
    # fields: reproduced, exit_code, run1, run2
    reproduced_template: str = ""
    # fields: maps, clean, fail, crash, timeout, stub_only, warnings, matches_started;
    # warnings is the total SCRIPT_WARNING count, or "not recorded" unless every sweep
    # row has one; matches_started counts the rows whose match_started is true
    counts_template: str = ""

    # Re-running.
    exe: str = ""                    # name shown in repro lines
    game_placeholder: str = ""       # game folder shown in repro lines
    exit_after_seconds: int = 0
    repro_timeout_seconds: int = 0
    repro_logdir: str = ""
    url_options: tuple = ()          # joined onto --url=<map>, e.g. ("?NumBots=4",)
    # (prefix, class) pairs: a map whose name prefix matches gets ?game=<class> right
    # after its name, before url_options, e.g. (("AS", "Botpack.Assault"),)
    url_game_by_prefix: tuple = ()
    exec: str = ""                   # passed as --exec=<exec> when not empty

    # Checker options.
    # A map with a recorded SCRIPT_WARNING count above zero is non-clean. Off by
    # default: the original games log Accessed None constantly. A row with no
    # recorded count is unknown and never makes a map non-clean.
    warnings_fail_clean: bool = False
    row_count: int | None = None
    screenshots_null: bool = False
    errors_in_section: bool = False
    gameplay_stubs_in_section: bool = False
    summary_counts_line: bool = False
    stubs_md_count: bool = False
    require_first_five: bool = False
    reproduced_exit_code: bool = False
    maps_loaded_equal: bool = False
    verdict_line_equal: bool = False
    map_match_case_insensitive: bool = False
    # A row whose sweep recorded match_started false is non-clean, reason
    # "match did not start". A row with no recorded flag is unknown, as with warnings.
    require_match_start: bool = False
    # With warning_register set (a path relative to the scenario directory, where
    # the sidecars are read from), the check fails for every site errors.json
    # records that has no row under the register's `### <register_heading>`, so a
    # site is read and entered before a sweep passes. Empty is off.
    warning_register: str = ""
    register_heading: str = ""
