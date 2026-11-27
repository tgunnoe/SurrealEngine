"""When a match counts as started, for the sweep and for the re-run comparison.

One rule in one place: a sweep row and a re-run cannot disagree on what a
start is. Only the log's STATE and EXEC lines matter.
"""

WALKING = "PlayerWalking"


def match_started(lines):
    """True when the player was walking at the fire press or walked after it.

    `lines` are the log's lines. The press is the first EXEC line; an
    EXEC_FAILED or EXEC_SKIPPED line is another kind and never the press. The
    last STATE line before the press is the state in effect when it ran, and a
    log with no press has no match.
    """
    state = None
    pressed = False
    for line in lines:
        kind, _, rest = line.partition(" ")
        if kind == "STATE":
            fields = rest.split()
            if len(fields) >= 2:
                state = fields[1]
                if pressed and state == WALKING:
                    return True
        elif kind == "EXEC" and not pressed:
            pressed = True
            if state == WALKING:
                return True
    return False
