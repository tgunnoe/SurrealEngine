# The draw counter. Usage: python3 -I -B frand.py <engine log> [<smoke log>]
# Counts Object.FRand trace lines before the first GameInfo Login and the first PostLogin trace line, the per-caller
# table before Login, and the first player spawn from the smoke log's first STATE line. CAPPED_BEFORE_POSTLOGIN 1
# means the 20000-call stop line came before PostLogin: the counts are void and the run is repeated with a narrower filter.
import re, sys, os, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tracelib import read_trace, first_index

rows, cap_at = read_trace(sys.argv[1])
n = 0; before_login = before_post = None
table = collections.Counter()
for r in rows:
    if r['func'] == 'FRand':
        n += 1
        if before_login is None:
            table[(r['label'], r['cls'], r['caller'])] += 1
    elif r['func'] == 'Login' and before_login is None:
        before_login = n
    elif r['func'] == 'PostLogin' and before_post is None:
        before_post = n
pl = first_index(rows, 'PostLogin')
print('CAPPED_BEFORE_POSTLOGIN', int(cap_at is not None and (pl is None or cap_at <= pl)))
print('FRAND_BEFORE_LOGIN', before_login)
print('FRAND_BEFORE_POSTLOGIN', before_post)
for (label, cls, caller), c in sorted(table.items(), key=lambda kv: (-kv[1], kv[0])):
    print('CALLER', c, label, cls, 'via', caller)
if len(sys.argv) > 2:
    spawn = None
    for line in open(sys.argv[2], errors='replace'):
        m = re.match(r'STATE \S+ \S+ at \((-?[0-9.]+), (-?[0-9.]+), (-?[0-9.]+)\)', line)
        if m:
            spawn = m.groups(); break
    print('FIRST_SPAWN', spawn)
