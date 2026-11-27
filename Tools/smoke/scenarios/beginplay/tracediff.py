# Line-for-line identity of two runs' trace lines after the bracketed seconds are stripped, over the load segment
# (the lines before the first Login trace line) or, with --whole, over the whole run.
# Usage: python3 -I -B tracediff.py <reference log> <subject log> [--whole]
# Prints TRACE_IDENTICAL <n> (exit 0) or TRACE_DIFFERS with the first differences (exit 1).
import sys, os, difflib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tracelib import strip_seconds, CAP, TRACE

WHOLE = '--whole' in sys.argv
sys.argv = [a for a in sys.argv if a != '--whole']

def lines(path):
    out, capped, found = [], False, False
    for l in open(path, errors='replace'):
        if CAP.search(l): capped = True
        if ': trace: ' not in l: continue
        m = TRACE.match(l.rstrip('\n'))
        if not WHOLE and m and m.group('func') == 'Login':
            found = True; break
        out.append(strip_seconds(l))
    return out, capped, found

a, ca, fa = lines(sys.argv[1]); b, cb, fb = lines(sys.argv[2])
print('LOGIN_ROW reference', int(fa), 'subject', int(fb), 'CAPPED reference', int(ca), 'subject', int(cb))
if a == b:
    print('TRACE_IDENTICAL', len(a))
else:
    print('TRACE_DIFFERS reference', len(a), 'subject', len(b))
    for i, d in enumerate(difflib.unified_diff(a, b, 'reference', 'subject', lineterm='', n=0)):
        if i >= 40: break
        print(d)
    sys.exit(1)
