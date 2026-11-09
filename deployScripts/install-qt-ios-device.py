#!/usr/bin/env python3
"""Install Qt's iOS Device kit from the public Qt download catalog."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET


def download(url, destination):
    subprocess.run(
        ["curl", "--fail", "--silent", "--show-error", "--location", "--retry", "3",
         "--output", str(destination), url],
        check=True,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version")
    parser.add_argument("output", type=Path, help="Destination kit directory")
    args = parser.parse_args()

    version_digits = args.version.replace(".", "")
    catalog = f"qt6_{version_digits}"
    repo = f"https://download.qt.io/online/qtsdkrepository/mac_x64/ios/{catalog}/{catalog}_device"
    package_prefix = f"qt.qt6.{version_digits}"
    modules = ("qtlocation", "qtmultimedia", "qtpositioning", "qtshadertools")
    package_names = [f"{package_prefix}.qtforios.device"]
    package_names += [f"{package_prefix}.addons.{module}.ios_device" for module in modules]
    args.output.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as temporary_dir:
        work = Path(temporary_dir)
        metadata = work / "Updates.xml"
        download(f"{repo}/Updates.xml", metadata)
        packages = {package.findtext("Name"): package for package in ET.parse(metadata).findall("PackageUpdate")}

        for name in package_names:
            package = packages.get(name)
            if package is None:
                raise SystemExit(f"Qt iOS Device package missing from catalog: {name}")
            version = package.findtext("Version")
            archives = package.findtext("DownloadableArchives", "").split(",")
            for archive in archives:
                archive = archive.strip()
                if not archive:
                    raise SystemExit(f"Qt iOS Device package has no archive: {name}")
                print(f"Installing {archive}", flush=True)
                url = f"{repo}/{name}/{version}{archive}"
                archive_path = work / archive
                checksum_path = work / "archive.sha1"
                download(f"{url}.sha1", checksum_path)
                download(url, archive_path)
                with archive_path.open("rb") as contents:
                    checksum = hashlib.file_digest(contents, "sha1").hexdigest()
                if checksum != checksum_path.read_text().strip().lower():
                    raise SystemExit(f"Checksum mismatch: {archive}")
                subprocess.run(["cmake", "-E", "tar", "xf", str(archive_path)], cwd=args.output, check=True)
                archive_path.unlink()

    if not (args.output / "lib/cmake/Qt6/qt.toolchain.cmake").is_file():
        raise SystemExit(f"Qt iOS Device toolchain missing from {args.output}")


if __name__ == "__main__":
    main()
