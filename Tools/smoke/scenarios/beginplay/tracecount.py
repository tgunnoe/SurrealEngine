# Per-object, per-function trace-line counts and their comparison. Usage:
#   python3 -I -B tracecount.py count <engine log> [--whole]     one "<label> <Class> <Function> <n>" line per key
#   python3 -I -B tracecount.py compare <reference log> <subject log> [--whole]   prints COUNTS_EQUAL or the differences
# Only the load segment (the rows before the first Login row) is counted unless --whole is given. An object is its label
# and class; objects in one log only are listed and are not differences of the common set. CAPPED_IN_LOAD 1 on either
# log voids the comparison: the row is repeated with a narrower filter.
import sys, os, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tracelib import read_trace, load_segment

WHOLE = '--whole' in sys.argv
sys.argv = [a for a in sys.argv if a != '--whole']

def counts(path):
    rows, cap_at = read_trace(path)
    found, capped = True, cap_at is not None
    if not WHOLE:
        rows, found, capped = load_segment(rows, cap_at)
    return collections.Counter((r['label'], r['cls'], r['func']) for r in rows), found, capped

if sys.argv[1] == 'count':
    c, found, capped = counts(sys.argv[2])
    for (label, cls, func), n in sorted(c.items()):
        print(label, cls, func, n)
    print('LOGIN_ROW', int(found), 'CAPPED_IN_LOAD', int(capped))
else:
    a, fa, ca = counts(sys.argv[2]); b, fb, cb = counts(sys.argv[3])
    print('LOGIN_ROW reference', int(fa), 'subject', int(fb), 'CAPPED_IN_LOAD reference', int(ca), 'subject', int(cb))
    oa = {(l, k) for (l, k, _f) in a}; ob = {(l, k) for (l, k, _f) in b}
    both = oa & ob
    diffs = [(k, a.get(k, 0), b.get(k, 0)) for k in sorted(set(a) | set(b)) if (k[0], k[1]) in both and a.get(k, 0) != b.get(k, 0)]
    for l, k in sorted(oa - ob): print('ONLY_REFERENCE', l, k)
    for l, k in sorted(ob - oa): print('ONLY_SUBJECT', l, k)
    for k, x, y in diffs: print('DIFF', *k, 'reference', x, 'subject', y)
    print('COUNTS_EQUAL' if not diffs else 'COUNTS_DIFFER', 'objects_common', len(both), 'only_reference', len(oa - ob), 'only_subject', len(ob - oa))
