"""Extract per-map STUB and error sidecars from smoke logs."""
import re

ERROR_KINDS = ("SCRIPT_ERROR", "MISSING_NATIVE", "EXCEPTION", "FATAL")
HEX = re.compile(r"0x[0-9a-fA-F]+")


def classify(scn, owner):
    raise NotImplementedError


def owner_of(scn, text):
    raise NotImplementedError


def strip_native_stack(text):
    raise NotImplementedError


def parse(path):
    raise NotImplementedError


def extract(scn, logdir, outdir):
    raise NotImplementedError
