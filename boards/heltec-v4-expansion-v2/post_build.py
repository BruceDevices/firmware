"""Name and verify Heltec release artifacts after Bruce's standard image merger."""

from pathlib import Path
import runpy
from SCons.Script import Import

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
build_dir = Path(env.subst("$BUILD_DIR"))
package_release = runpy.run_path(
    str(project_dir / "boards" / "heltec-v4-expansion-v2" / "package_release.py")
)["package_release"]


def package_heltec(target, source, env):
    try:
        metadata = package_release(project_dir, build_dir)
    except (OSError, ValueError) as error:
        print(f"[Heltec package] {error}")
        env.Exit(1)
    print(
        "[Heltec package] FACTORY and LAUNCHER-INSTALL verified; application SHA-256 "
        + metadata["application"]["sha256"]
    )


# Global post:build.py registers its merge callback first. This callback then
# rejects missing/stale/mismatched merged output before publishing named copies.
env.AddPostAction(str(build_dir / "firmware.bin"), package_heltec)
