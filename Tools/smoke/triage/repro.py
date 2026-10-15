"""Re-run maps individually and compare against earlier logs."""


def compare(scn, old_log, new_log):
    """Return (verdict, first_line_run1, first_line_run2)."""
    raise NotImplementedError


def run(scn, exe, game, logdir, out, maps):
    raise NotImplementedError
