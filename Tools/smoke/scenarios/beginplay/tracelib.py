# Parsing shared by frand.py, tracecount.py and tracediff.py: the engine log's --trace-script lines.
# A line is "[<level seconds>] <caller function>: trace: <label> <Class> <state>: <Function>(<args>)".
# The prefix names a script function of the call stack (the frame below the top one when the line is written),
# which is what "via Actor.PreBeginPlay" in a draw table means. The label of an unnamed object holds a space.
import re

TRACE = re.compile(
    r'^\[(?P<t>[0-9.]+)\] (?P<caller>.+?): trace: '
    r'(?P<label>(?:<unnamed [^>]*>)|\S+) (?P<cls>\S+) (?P<state>\S+): (?P<func>\w+)\((?P<args>.*)\)\s*$')
CAP = re.compile(r'trace: stopped after (\d+) calls')

def read_trace(path):
    """Return (parsed trace rows in file order, the number of rows before the 20000-call stop line or None)."""
    rows, cap_at = [], None
    with open(path, 'r', errors='replace') as f:
        for line in f:
            if cap_at is None and CAP.search(line):
                cap_at = len(rows)
            m = TRACE.match(line.rstrip('\n'))
            if m:
                rows.append(m.groupdict())
    return rows, cap_at

def strip_seconds(line):
    return re.sub(r'^\[[0-9.]+\] ', '', line.rstrip('\n'))

def first_index(rows, func):
    for i, r in enumerate(rows):
        if r['func'] == func:
            return i
    return None

def load_segment(rows, cap_at):
    """(rows before the first Login row, Login found, the stop line fell inside the segment).
    Everything the begin-play loops do happens in that segment; what follows is play, whose spawns depend on timing."""
    i = first_index(rows, 'Login')
    if i is None:
        return rows, False, cap_at is not None
    return rows[:i], True, cap_at is not None and cap_at <= i
