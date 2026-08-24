#!/usr/bin/env python3
"""Generate the looping geometry GIFs embedded by README.md.

The renderer owns animation timing. This script only builds the application,
captures its deterministic RGB24 stream, and encodes the resulting frames.
"""

from __future__ import annotations

import argparse
import contextlib
import dataclasses
import datetime as dt
import hashlib
import json
import locale
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, Iterator, List, Mapping, MutableMapping, Optional, Sequence


MINIMUM_PYTHON = (3, 9)
MAXIMUM_RAW_BYTES = 8 * 1024 * 1024 * 1024
FIFTY_MIB = 50 * 1024 * 1024
ONE_HUNDRED_MIB = 100 * 1024 * 1024
GEOMETRY_SLUG = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")
PALETTE_FILTER = (
    "[0:v]vflip,split=2[frames][palette_source];"
    "[palette_source]palettegen=max_colors=96:stats_mode=diff[palette];"
    "[frames][palette]paletteuse="
    "dither=bayer:bayer_scale=4:diff_mode=rectangle"
)


class GeneratorError(RuntimeError):
    """A user-facing generation failure."""


@dataclasses.dataclass(frozen=True)
class Host:
    name: str
    triplet: str
    vcpkg_executable_name: str
    application_name: str


@dataclasses.dataclass(frozen=True)
class BuildTools:
    cmake: Path
    ninja: Path
    c_compiler: Path
    cxx_compiler: Path


class Log:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.path.write_text("", encoding="utf-8")

    @staticmethod
    def timestamp() -> str:
        return dt.datetime.now(dt.timezone.utc).isoformat()

    def write(self, message: str, level: str = "INFO") -> None:
        line = f"[{self.timestamp()}] [{level}] {message}"
        print(line, flush=True)
        with self.path.open("a", encoding="utf-8", newline="") as stream:
            stream.write(line + "\n")

    def command(self, description: str, arguments: Sequence[object]) -> None:
        rendered = shlex.join([os.fspath(argument) for argument in arguments])
        self.write(f"{description}: {rendered}")

    def raw(self, text: str) -> None:
        if not text:
            return
        print(text, end="" if text.endswith("\n") else "\n", flush=True)
        with self.path.open("a", encoding="utf-8", newline="") as stream:
            stream.write(text)
            if not text.endswith("\n"):
                stream.write("\n")


def utc_timestamp() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def write_status(path: Path, status: MutableMapping[str, object]) -> None:
    status["updatedAtUtc"] = utc_timestamp()
    temporary = path.with_name(f".{path.name}.{os.getpid()}.tmp")
    temporary.write_text(
        json.dumps(status, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    os.replace(temporary, path)


@contextlib.contextmanager
def exclusive_lock(path: Path) -> Iterator[None]:
    """Hold one byte of a cross-platform, non-blocking process lock."""

    path.parent.mkdir(parents=True, exist_ok=True)
    try:
        stream = path.open("a+b")
    except OSError as error:
        raise GeneratorError(
            f"The generator lock could not be opened: '{path}'."
        ) from error

    try:
        stream.seek(0, os.SEEK_END)
        if stream.tell() == 0:
            stream.write(b"0")
            stream.flush()
        stream.seek(0)

        try:
            if os.name == "nt":
                import msvcrt

                msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl

                fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except (OSError, PermissionError) as error:
            raise GeneratorError(
                f"Another README animation generator is already running for this checkout. "
                f"Lock: '{path}'"
            ) from error

        yield
    finally:
        try:
            stream.seek(0)
            if os.name == "nt":
                import msvcrt

                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                import fcntl

                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)
        except OSError:
            pass
        stream.close()


def environment_encoding() -> str:
    return locale.getpreferredencoding(False) or sys.getfilesystemencoding() or "utf-8"


def environment_value(
    environment: Mapping[str, str], name: str
) -> Optional[str]:
    value = environment.get(name)
    if value is not None or os.name != "nt":
        return value
    folded_name = name.casefold()
    return next(
        (item for key, item in environment.items() if key.casefold() == folded_name),
        None,
    )


def copy_environment() -> Dict[str, str]:
    if os.name == "nt":
        return {key.upper(): value for key, value in os.environ.items()}
    return dict(os.environ)


def run_logged(
    arguments: Sequence[object],
    *,
    cwd: Path,
    environment: Mapping[str, str],
    log: Log,
    description: str,
) -> None:
    command = [os.fspath(argument) for argument in arguments]
    log.command(description, command)

    process = subprocess.Popen(
        command,
        cwd=cwd,
        env=dict(environment),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding=environment_encoding(),
        errors="replace",
        bufsize=1,
    )
    assert process.stdout is not None
    for line in process.stdout:
        log.raw(line)
    return_code = process.wait()
    if return_code != 0:
        raise GeneratorError(f"{description} failed with exit code {return_code}.")


def run_captured(
    arguments: Sequence[object],
    *,
    cwd: Path,
    environment: Mapping[str, str],
    log: Log,
    description: str,
) -> str:
    command = [os.fspath(argument) for argument in arguments]
    log.command(description, command)
    result = subprocess.run(
        command,
        cwd=cwd,
        env=dict(environment),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding=environment_encoding(),
        errors="replace",
        check=False,
    )
    log.raw(result.stdout)
    log.raw(result.stderr)
    if result.returncode != 0:
        raise GeneratorError(
            f"{description} failed with exit code {result.returncode}."
        )
    return result.stdout


def resolve_application(name: str, environment: Mapping[str, str]) -> Optional[Path]:
    resolved = shutil.which(name, path=environment_value(environment, "PATH"))
    if resolved is None and os.name == "nt" and not name.lower().endswith(".exe"):
        resolved = shutil.which(
            f"{name}.exe", path=environment_value(environment, "PATH")
        )
    return Path(resolved).resolve() if resolved else None


def detect_host() -> Host:
    system = platform.system()
    machine = platform.machine().lower()
    if machine not in {"amd64", "x86_64"}:
        raise GeneratorError(
            f"README animation generation currently supports x86_64 hosts, not '{machine}'."
        )
    if system == "Windows":
        return Host("Windows", "x64-windows-static-md", "vcpkg.exe", "quantum-wave-sphere.exe")
    if system == "Linux":
        return Host("Linux", "x64-linux", "vcpkg", "quantum-wave-sphere")
    if system == "Darwin":
        raise GeneratorError(
            "macOS is unsupported: quantum-wave-sphere requires OpenGL 4.6 Core and GLSL 460."
        )
    raise GeneratorError(f"Unsupported host operating system: {system}")


def find_vswhere(environment: Mapping[str, str]) -> Optional[Path]:
    from_path = resolve_application("vswhere", environment)
    if from_path is not None:
        return from_path

    for variable in ("ProgramFiles(x86)", "ProgramFiles"):
        root = environment_value(environment, variable)
        if root:
            candidate = Path(root) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
            if candidate.is_file():
                return candidate.resolve()
    return None


def find_visual_studio(
    environment: Mapping[str, str], repository: Path, log: Log
) -> Path:
    vswhere = find_vswhere(environment)
    if vswhere is not None:
        try:
            output = run_captured(
                [
                    vswhere,
                    "-latest",
                    "-products",
                    "*",
                    "-requires",
                    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                    "-property",
                    "installationPath",
                ],
                cwd=repository,
                environment=environment,
                log=log,
                description="Locate Visual Studio",
            )
            for line in output.splitlines():
                candidate = Path(line.strip())
                if (
                    candidate
                    / "VC"
                    / "Auxiliary"
                    / "Build"
                    / "vcvars64.bat"
                ).is_file():
                    return candidate.resolve()
            log.write(
                "vswhere found no Visual Studio installation with vcvars64.bat; "
                "checking standard installation roots.",
                "WARNING",
            )
        except GeneratorError as error:
            log.write(f"vswhere could not locate Visual Studio: {error}", "WARNING")

    candidates: List[Path] = []
    configured = environment_value(environment, "VSINSTALLDIR")
    if configured:
        candidates.append(Path(configured))

    for variable in ("ProgramFiles", "ProgramFiles(x86)"):
        program_files = environment_value(environment, variable)
        if not program_files:
            continue
        visual_studio_root = Path(program_files) / "Microsoft Visual Studio"
        if not visual_studio_root.is_dir():
            continue
        try:
            versions = sorted(
                (path for path in visual_studio_root.iterdir() if path.is_dir()),
                key=lambda path: path.name,
                reverse=True,
            )
            for version in versions:
                candidates.extend(
                    sorted(
                        (path for path in version.iterdir() if path.is_dir()),
                        key=lambda path: path.name,
                    )
                )
        except OSError:
            continue

    seen = set()
    for candidate in candidates:
        resolved = candidate.resolve()
        identity = os.fspath(resolved).casefold()
        if identity in seen:
            continue
        seen.add(identity)
        vcvars = resolved / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
        if vcvars.is_file():
            log.write(f"Located Visual Studio without vswhere: {resolved}")
            return resolved

    raise GeneratorError(
        "Visual Studio with the MSVC x64 tools was not found through vswhere, "
        "VSINSTALLDIR, or the standard installation roots."
    )


def import_msvc_environment(
    visual_studio: Path,
    environment: MutableMapping[str, str],
    repository: Path,
    log: Log,
) -> None:
    vcvars = visual_studio / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not vcvars.is_file():
        raise GeneratorError(f"vcvars64.bat was not found: {vcvars}")
    command_processor = environment_value(environment, "ComSpec") or shutil.which(
        "cmd.exe"
    )
    if not command_processor:
        raise GeneratorError("cmd.exe is unavailable; the MSVC environment cannot be imported.")

    command_line = f'call "{vcvars}" >nul && set'
    log.write(f"Importing the x64 MSVC environment from {vcvars}")
    result = subprocess.run(
        command_line,
        cwd=repository,
        env=dict(environment),
        executable=os.fspath(command_processor),
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding=environment_encoding(),
        errors="replace",
        check=False,
    )
    if result.returncode != 0:
        log.raw(result.stderr)
        raise GeneratorError(
            f"vcvars64.bat failed with exit code {result.returncode}."
        )
    for line in result.stdout.splitlines():
        key, separator, value = line.partition("=")
        if separator and key and not key.startswith("="):
            environment[key.upper() if os.name == "nt" else key] = value
    if resolve_application("cl", environment) is None:
        raise GeneratorError("vcvars64.bat completed, but cl.exe is still unavailable.")


def resolve_vcpkg_root(repository: Path, environment: Mapping[str, str]) -> Path:
    configured = environment_value(environment, "VCPKG_ROOT")
    return Path(configured).expanduser().resolve() if configured else (repository / ".tools" / "vcpkg").resolve()


def fetch_vcpkg_tool(
    tool: str,
    *,
    vcpkg: Path,
    repository: Path,
    environment: Mapping[str, str],
    log: Log,
) -> Path:
    output = run_captured(
        [vcpkg, "fetch", tool, "--x-stderr-status"],
        cwd=repository,
        environment=environment,
        log=log,
        description=f"Fetch portable {tool}",
    )
    for line in reversed(output.splitlines()):
        candidate_text = line.strip().strip('"')
        if not candidate_text:
            continue
        candidate = Path(candidate_text)
        if not candidate.is_absolute():
            candidate = repository / candidate
        if candidate.is_file():
            return candidate.resolve()
    raise GeneratorError(f"vcpkg fetch {tool} returned no existing executable path.")


def linux_compiler_family(path: Path) -> Optional[str]:
    name = path.name.lower()
    if "clang" in name:
        return "Clang"
    if "gcc" in name or "g++" in name:
        return "GCC"
    return None


def resolve_linux_compilers(
    environment: Mapping[str, str],
) -> tuple[Path, Path]:
    configured_c = environment_value(environment, "CC")
    configured_cxx = environment_value(environment, "CXX")
    if bool(configured_c) != bool(configured_cxx):
        raise GeneratorError(
            "Set both CC and CXX for README animation generation, or leave both unset."
        )

    if configured_c and configured_cxx:
        c_compiler = resolve_application(configured_c, environment)
        cxx_compiler = resolve_application(configured_cxx, environment)
        if c_compiler is None:
            raise GeneratorError(f"CC does not name an executable: {configured_c}")
        if cxx_compiler is None:
            raise GeneratorError(f"CXX does not name an executable: {configured_cxx}")
        c_family = linux_compiler_family(c_compiler)
        cxx_family = linux_compiler_family(cxx_compiler)
        if c_family is None or cxx_family is None or c_family != cxx_family:
            raise GeneratorError(
                "CC and CXX must identify one coherent GCC or Clang compiler pair."
            )
        return c_compiler, cxx_compiler

    for c_name, cxx_name in (("gcc", "g++"), ("clang", "clang++"), ("cc", "c++")):
        c_compiler = resolve_application(c_name, environment)
        cxx_compiler = resolve_application(cxx_name, environment)
        if c_compiler is None or cxx_compiler is None:
            continue
        if linux_compiler_family(c_compiler) == linux_compiler_family(cxx_compiler):
            return c_compiler, cxx_compiler

    raise GeneratorError("A coherent GCC or Clang C/C++ compiler pair is required on Linux.")


def initialize_build_tools(
    host: Host,
    repository: Path,
    vcpkg: Path,
    environment: MutableMapping[str, str],
    log: Log,
) -> BuildTools:
    cmake = resolve_application("cmake", environment)
    ninja = resolve_application("ninja", environment)

    visual_studio: Optional[Path] = None
    if host.name == "Windows":
        if resolve_application("cl", environment) is None or cmake is None or ninja is None:
            visual_studio = find_visual_studio(environment, repository, log)
        if resolve_application("cl", environment) is None:
            assert visual_studio is not None
            import_msvc_environment(visual_studio, environment, repository, log)
        if cmake is None and visual_studio is not None:
            candidate = visual_studio / "Common7" / "IDE" / "CommonExtensions" / "Microsoft" / "CMake" / "CMake" / "bin" / "cmake.exe"
            if candidate.is_file():
                cmake = candidate.resolve()
        if ninja is None and visual_studio is not None:
            candidate = visual_studio / "Common7" / "IDE" / "CommonExtensions" / "Microsoft" / "CMake" / "Ninja" / "ninja.exe"
            if candidate.is_file():
                ninja = candidate.resolve()

    if cmake is None:
        cmake = fetch_vcpkg_tool(
            "cmake", vcpkg=vcpkg, repository=repository, environment=environment, log=log
        )
    if ninja is None:
        ninja = fetch_vcpkg_tool(
            "ninja", vcpkg=vcpkg, repository=repository, environment=environment, log=log
        )

    environment["PATH"] = os.pathsep.join(
        [os.fspath(ninja.parent), environment_value(environment, "PATH") or ""]
    )

    if host.name == "Windows":
        compiler = resolve_application("cl", environment)
        if compiler is None:
            raise GeneratorError("cl.exe is unavailable after Visual Studio initialization.")
        c_compiler = compiler
        cxx_compiler = compiler
    else:
        c_compiler, cxx_compiler = resolve_linux_compilers(environment)

    log.write(f"Using CMake: {cmake}")
    log.write(f"Using Ninja: {ninja}")
    log.write(f"Using C compiler: {c_compiler}")
    log.write(f"Using C++ compiler: {cxx_compiler}")
    return BuildTools(cmake, ninja, c_compiler, cxx_compiler)


def find_executable(build_directory: Path, host: Host) -> Optional[Path]:
    for candidate in (
        build_directory / host.application_name,
        build_directory / "Release" / host.application_name,
    ):
        if candidate.is_file():
            return candidate.resolve()
    return None


def resolve_geometries(
    executable: Path,
    requested: Optional[List[str]],
    *,
    repository: Path,
    environment: Mapping[str, str],
    log: Log,
) -> List[str]:
    output = run_captured(
        [executable, "--list-geometries"],
        cwd=repository,
        environment=environment,
        log=log,
        description="List available geometries",
    )
    available = [line.strip() for line in output.splitlines() if line.strip()]
    if not available or any(GEOMETRY_SLUG.fullmatch(slug) is None for slug in available):
        raise GeneratorError("The executable returned an invalid geometry list.")

    selected = requested if requested else available
    result: List[str] = []
    for slug in selected:
        if GEOMETRY_SLUG.fullmatch(slug) is None:
            raise GeneratorError(f"Invalid geometry slug: {slug}")
        if slug not in available:
            raise GeneratorError(
                f"Unknown geometry '{slug}'. Available values: {', '.join(available)}"
            )
        if slug not in result:
            result.append(slug)
    if not result:
        raise GeneratorError("No geometries were selected for capture.")
    return result


def validate_gif(
    gif_path: Path,
    *,
    width: int,
    height: int,
    expected_frames: int,
    frames_per_second: int,
    ffmpeg: Path,
    ffprobe: Path,
    repository: Path,
    environment: Mapping[str, str],
    log: Log,
) -> Dict[str, object]:
    size = gif_path.stat().st_size
    if size >= ONE_HUNDRED_MIB:
        raise GeneratorError(
            f"Generated GIF is {size} bytes; ordinary Git files must remain below 100 MiB."
        )
    if size > FIFTY_MIB:
        log.write(f"Generated GIF is {size} bytes, above GitHub's 50 MiB warning threshold.", "WARNING")

    probe_text = run_captured(
        [
            ffprobe,
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-count_frames",
            "-show_entries",
            "stream=codec_name,width,height,nb_read_frames,duration",
            "-of",
            "json",
            gif_path,
        ],
        cwd=repository,
        environment=environment,
        log=log,
        description="Validate generated GIF metadata",
    )
    try:
        probe = json.loads(probe_text)
        streams = probe["streams"]
        stream = streams[0]
        probed_frames = int(stream["nb_read_frames"])
        duration = float(stream["duration"])
    except (KeyError, IndexError, TypeError, ValueError, json.JSONDecodeError) as error:
        raise GeneratorError(f"ffprobe returned invalid GIF metadata for '{gif_path}'.") from error

    if len(streams) != 1 or stream.get("codec_name") != "gif":
        raise GeneratorError(f"Expected one GIF stream in '{gif_path}'.")
    if int(stream.get("width", -1)) != width or int(stream.get("height", -1)) != height:
        raise GeneratorError(f"Expected {width}x{height} GIF dimensions in '{gif_path}'.")
    if probed_frames != expected_frames:
        raise GeneratorError(
            f"Expected {expected_frames} frames in '{gif_path}', but ffprobe counted {probed_frames}."
        )
    expected_duration = expected_frames / frames_per_second
    if abs(duration - expected_duration) > 0.1:
        raise GeneratorError(
            f"Expected a {expected_duration:.3f}-second GIF, but ffprobe reported "
            f"{duration:.3f} seconds for '{gif_path}'."
        )
    if duration > 30.1:
        raise GeneratorError(f"Generated GIF duration exceeds the 30-second cap: {duration}")

    run_logged(
        [ffmpeg, "-v", "error", "-i", gif_path, "-f", "null", "-"],
        cwd=repository,
        environment=environment,
        log=log,
        description="Decode generated GIF",
    )
    with gif_path.open("rb") as stream_file:
        infinite_loop_extension = b"\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00"
        if infinite_loop_extension not in stream_file.read():
            raise GeneratorError(
                f"Generated GIF has no infinite-loop extension with loop count zero: {gif_path}"
            )

    return {"bytes": size, "frames": probed_frames, "durationSeconds": duration}


def validate_raw_capture(raw_path: Path, frame_bytes: int) -> tuple[int, int]:
    raw_size = raw_path.stat().st_size
    if raw_size == 0 or raw_size % frame_bytes != 0:
        raise GeneratorError(
            f"Raw capture size {raw_size} is not a positive multiple of {frame_bytes}."
        )

    frame_count = raw_size // frame_bytes
    sample_indices = sorted(
        {
            0,
            frame_count // 4,
            frame_count // 2,
            (3 * frame_count) // 4,
            frame_count - 1,
        }
    )
    sampled_hashes = set()
    has_visible_content = False
    with raw_path.open("rb") as stream:
        for frame_index in sample_indices:
            stream.seek(frame_index * frame_bytes)
            frame = stream.read(frame_bytes)
            if len(frame) != frame_bytes:
                raise GeneratorError(
                    f"Could not read frame {frame_index} completely from '{raw_path}'."
                )
            sampled_hashes.add(hashlib.sha256(frame).digest())
            has_visible_content = has_visible_content or min(frame) != max(frame)

    if not has_visible_content:
        raise GeneratorError(f"Raw capture appears blank: {raw_path}")
    if frame_count > 1 and len(sampled_hashes) < 2:
        raise GeneratorError(f"Raw capture appears frozen: {raw_path}")
    return frame_count, raw_size


def bounded_integer(name: str, minimum: int, maximum: int):
    def parse(value: str) -> int:
        try:
            parsed = int(value)
        except ValueError as error:
            raise argparse.ArgumentTypeError(f"{name} must be an integer.") from error
        if parsed < minimum or parsed > maximum:
            raise argparse.ArgumentTypeError(
                f"{name} must be between {minimum} and {maximum}."
            )
        return parsed

    return parse


def parse_arguments(repository: Path) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build quantum-wave-sphere and generate every looping README GIF."
    )
    parser.add_argument(
        "--output-directory",
        default=os.fspath(repository / "docs" / "media" / "geometries"),
        help="GIF destination (default: docs/media/geometries).",
    )
    parser.add_argument(
        "--work-directory",
        default=os.fspath(repository / "out" / "readme-animation-generation"),
        help="Ignored temporary/status directory.",
    )
    parser.add_argument("--width", type=bounded_integer("width", 128, 1920), default=640)
    parser.add_argument("--height", type=bounded_integer("height", 72, 1080), default=360)
    parser.add_argument(
        "--fps", type=bounded_integer("fps", 1, 60), default=24, dest="frames_per_second"
    )
    parser.add_argument(
        "--geometry",
        action="append",
        dest="geometries",
        help="Capture one slug; repeat to select several. Defaults to all app geometries.",
    )
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--keep-raw", action="store_true")
    return parser.parse_args()


def absolute_path(value: str, repository: Path) -> Path:
    path = Path(value).expanduser()
    return path.resolve() if path.is_absolute() else (repository / path).resolve()


def generate() -> int:
    if sys.version_info < MINIMUM_PYTHON:
        raise GeneratorError(
            f"Python {MINIMUM_PYTHON[0]}.{MINIMUM_PYTHON[1]} or newer is required."
        )

    repository = Path(__file__).resolve().parent.parent
    arguments = parse_arguments(repository)
    host = detect_host()
    if host.name == "Linux" and not os.environ.get("DISPLAY"):
        raise GeneratorError(
            "Linux capture requires DISPLAY because quantum-wave-sphere uses GLFW's X11 backend."
        )

    maximum_raw_size = (
        arguments.width
        * arguments.height
        * 3
        * 30
        * arguments.frames_per_second
    )
    if maximum_raw_size > MAXIMUM_RAW_BYTES:
        raise GeneratorError(
            "The requested size and frame rate could create more than 8 GiB of raw frames per geometry."
        )

    output_directory = absolute_path(arguments.output_directory, repository)
    work_directory = absolute_path(arguments.work_directory, repository)
    raw_directory = work_directory / "raw"
    build_directory = repository / "out" / "build" / "readme-animation-generator" / host.name
    status_path = work_directory / "status.json"
    log_path = work_directory / "generation.log"
    lock_path = repository / "out" / "readme-animation-generator.lock"

    output_directory.mkdir(parents=True, exist_ok=True)
    work_directory.mkdir(parents=True, exist_ok=True)
    raw_directory.mkdir(parents=True, exist_ok=True)

    with exclusive_lock(lock_path):
        log = Log(log_path)
        environment = copy_environment()
        started_at = utc_timestamp()
        status: MutableMapping[str, object] = {
            "schemaVersion": 2,
            "status": "starting",
            "stage": "initializing",
            "processId": os.getpid(),
            "startedAtUtc": started_at,
            "updatedAtUtc": started_at,
            "finishedAtUtc": None,
            "repositoryRoot": os.fspath(repository),
            "platform": host.name,
            "buildDirectory": os.fspath(build_directory),
            "outputDirectory": os.fspath(output_directory),
            "workDirectory": os.fspath(work_directory),
            "width": arguments.width,
            "height": arguments.height,
            "framesPerSecond": arguments.frames_per_second,
            "keepRaw": arguments.keep_raw,
            "skipBuild": arguments.skip_build,
            "geometries": [],
            "currentGeometry": None,
            "completedGeometries": [],
            "assets": [],
            "error": None,
        }
        write_status(status_path, status)

        try:
            log.write("README animation generation started.")
            log.write(
                f"Capture settings: {arguments.width}x{arguments.height}, "
                f"{arguments.frames_per_second} fps; renderer owns duration."
            )

            ffmpeg = resolve_application("ffmpeg", environment)
            ffprobe = resolve_application("ffprobe", environment)
            if ffmpeg is None:
                raise GeneratorError("ffmpeg was not found on PATH.")
            if ffprobe is None:
                sibling = ffmpeg.with_name("ffprobe.exe" if os.name == "nt" else "ffprobe")
                ffprobe = sibling.resolve() if sibling.is_file() else None
            if ffprobe is None:
                raise GeneratorError("ffprobe was not found on PATH or beside ffmpeg.")
            log.write(f"Using ffmpeg: {ffmpeg}")
            log.write(f"Using ffprobe: {ffprobe}")

            if not arguments.skip_build:
                vcpkg_root = resolve_vcpkg_root(repository, environment)
                vcpkg = vcpkg_root / host.vcpkg_executable_name
                toolchain = vcpkg_root / "scripts" / "buildsystems" / "vcpkg.cmake"
                if not vcpkg.is_file() or not toolchain.is_file():
                    raise GeneratorError(
                        f"Bootstrap the pinned vcpkg checkout for {host.name} first: "
                        f"{vcpkg_root}"
                    )
                environment["VCPKG_ROOT"] = os.fspath(vcpkg_root)

                status["status"] = "running"
                status["stage"] = "configuring-build"
                write_status(status_path, status)
                tools = initialize_build_tools(
                    host, repository, vcpkg, environment, log
                )
                installed_directory = repository / "out" / "vcpkg_installed" / host.name
                configure = [
                    tools.cmake,
                    "-S",
                    repository,
                    "-B",
                    build_directory,
                    "-G",
                    "Ninja",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DBUILD_TESTING=OFF",
                    f"-DCMAKE_MAKE_PROGRAM={tools.ninja}",
                    f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
                    f"-DVCPKG_INSTALLED_DIR={installed_directory}",
                    f"-DVCPKG_TARGET_TRIPLET={host.triplet}",
                    f"-DCMAKE_C_COMPILER={tools.c_compiler}",
                    f"-DCMAKE_CXX_COMPILER={tools.cxx_compiler}",
                ]
                run_logged(
                    configure,
                    cwd=repository,
                    environment=environment,
                    log=log,
                    description="Configure README animation build",
                )
                status["stage"] = "building"
                write_status(status_path, status)
                run_logged(
                    [
                        tools.cmake,
                        "--build",
                        build_directory,
                        "--config",
                        "Release",
                        "--target",
                        "quantum_wave_sphere",
                    ],
                    cwd=repository,
                    environment=environment,
                    log=log,
                    description="Build README animation executable",
                )
            else:
                log.write("Skipping configure and build because --skip-build was requested.")

            executable = find_executable(build_directory, host)
            if executable is None:
                raise GeneratorError(
                    f"{host.application_name} was not found under '{build_directory}'."
                )
            log.write(f"Using capture executable: {executable}")

            geometries = resolve_geometries(
                executable,
                arguments.geometries,
                repository=repository,
                environment=environment,
                log=log,
            )
            status["geometries"] = geometries
            status["status"] = "running"
            status["stage"] = "capturing"
            write_status(status_path, status)
            log.write(f"Selected geometries: {', '.join(geometries)}")

            frame_bytes = arguments.width * arguments.height * 3
            common_frame_count: Optional[int] = None
            for geometry in geometries:
                raw_path = raw_directory / f"{geometry}.rgb"
                part_path = output_directory / f".{geometry}.part.gif"
                final_path = output_directory / f"{geometry}.gif"
                raw_path.unlink(missing_ok=True)
                part_path.unlink(missing_ok=True)

                try:
                    status["currentGeometry"] = geometry
                    status["stage"] = "capturing"
                    write_status(status_path, status)
                    run_logged(
                        [
                            executable,
                            "--capture-raw",
                            raw_path,
                            "--geometry",
                            geometry,
                            "--fps",
                            str(arguments.frames_per_second),
                            "--width",
                            str(arguments.width),
                            "--height",
                            str(arguments.height),
                        ],
                        cwd=repository,
                        environment=environment,
                        log=log,
                        description=f"Capture geometry '{geometry}'",
                    )
                    if not raw_path.is_file():
                        raise GeneratorError(f"Capture produced no raw file: {raw_path}")
                    frame_count, raw_size = validate_raw_capture(raw_path, frame_bytes)
                    if common_frame_count is None:
                        common_frame_count = frame_count
                        status["frameCount"] = frame_count
                    elif frame_count != common_frame_count:
                        raise GeneratorError(
                            f"Geometry '{geometry}' produced {frame_count} frames; expected {common_frame_count}."
                        )
                    log.write(
                        f"Raw capture verified for '{geometry}': {frame_count} frames, {raw_size} bytes."
                    )

                    status["stage"] = "encoding"
                    write_status(status_path, status)
                    run_logged(
                        [
                            ffmpeg,
                            "-hide_banner",
                            "-loglevel",
                            "warning",
                            "-y",
                            "-f",
                            "rawvideo",
                            "-pixel_format",
                            "rgb24",
                            "-video_size",
                            f"{arguments.width}x{arguments.height}",
                            "-framerate",
                            str(arguments.frames_per_second),
                            "-i",
                            raw_path,
                            "-filter_complex",
                            PALETTE_FILTER,
                            "-an",
                            "-fps_mode",
                            "passthrough",
                            "-loop",
                            "0",
                            part_path,
                        ],
                        cwd=repository,
                        environment=environment,
                        log=log,
                        description=f"Encode geometry '{geometry}'",
                    )
                    if not part_path.is_file():
                        raise GeneratorError(f"ffmpeg produced no GIF: {part_path}")

                    status["stage"] = "validating"
                    write_status(status_path, status)
                    validation = validate_gif(
                        part_path,
                        width=arguments.width,
                        height=arguments.height,
                        expected_frames=frame_count,
                        frames_per_second=arguments.frames_per_second,
                        ffmpeg=ffmpeg,
                        ffprobe=ffprobe,
                        repository=repository,
                        environment=environment,
                        log=log,
                    )
                    os.replace(part_path, final_path)

                    completed = list(status["completedGeometries"])
                    completed.append(geometry)
                    status["completedGeometries"] = completed
                    assets = list(status["assets"])
                    assets.append(
                        {
                            "geometry": geometry,
                            "gifPath": os.fspath(final_path),
                            "rawPath": os.fspath(raw_path) if arguments.keep_raw else None,
                            **validation,
                            "completedAtUtc": utc_timestamp(),
                        }
                    )
                    status["assets"] = assets
                    write_status(status_path, status)
                    log.write(f"Published '{final_path}' ({validation['bytes']} bytes).")
                finally:
                    part_path.unlink(missing_ok=True)
                    if not arguments.keep_raw:
                        raw_path.unlink(missing_ok=True)

            status["status"] = "completed"
            status["stage"] = "completed"
            status["currentGeometry"] = None
            status["finishedAtUtc"] = utc_timestamp()
            write_status(status_path, status)
            log.write("README animation generation completed successfully.")
            return 0
        except Exception as error:
            status["status"] = "failed"
            status["stage"] = "failed"
            status["finishedAtUtc"] = utc_timestamp()
            status["error"] = {
                "message": str(error),
                "exceptionType": type(error).__name__,
            }
            with contextlib.suppress(Exception):
                write_status(status_path, status)
            log.write(str(error), "ERROR")
            return 1


def main() -> int:
    try:
        return generate()
    except (GeneratorError, OSError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
