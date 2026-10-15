# STUB messages across the Unreal Gold sweep

Distinct `STUB` lines in the 55 logs (counted with
`grep -h '^STUB' *.log | sort -u | wc -l`): 0.
Rows in this table: 0.

| Stub text | Maps hit | Class | Reason |
| :--- | ---: | :--- | :--- |

No map logged a `STUB` line (`stub_hits` is 0 in every row of
`sweep.json`), so `stubs.json` holds an empty list for each map and no
stub makes a map non-clean.
