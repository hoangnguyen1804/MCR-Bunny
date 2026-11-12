"""Build a Windows x64 executable with the pinned Zig compiler; no samples used."""
import importlib.metadata
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
BUILD.mkdir(exist_ok=True)
try:
    version = importlib.metadata.version("ziglang")
except importlib.metadata.PackageNotFoundError:
    raise SystemExit("Install the compiler first: python -m pip install ziglang==0.14.1")
if version != "0.14.1":
    raise SystemExit(f"Expected ziglang 0.14.1; found {version}")
environment = os.environ.copy()
environment.setdefault("ZIG_GLOBAL_CACHE_DIR", str(ROOT / ".zig-global-cache"))
environment.setdefault("ZIG_LOCAL_CACHE_DIR", str(ROOT / ".zig-cache"))
zig = [sys.executable, "-m", "ziglang"]
subprocess.run(
    zig + ["rc", "/fo", "build/app.res", "app.rc"],
    cwd=ROOT, env=environment, check=True,
)
subprocess.run(
    zig + [
        "cc", "-target", "x86_64-windows-gnu", "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-Wno-unused-command-line-argument",
        "-municode", "-Wl,--subsystem,windows", "main.c", "wheel_logic.c", "build/app.res",
        "-luser32", "-lgdi32", "-lwinmm", "-static", "-o", "build/AutoWheel.exe",
    ],
    cwd=ROOT, env=environment, check=True,
)
print(BUILD / "AutoWheel.exe")
