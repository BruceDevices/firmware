# Heltec release packaging tests

```sh
python tests/heltec_packaging/test_package_release.py
```

These dependency-free host tests exercise the board's actual packaging code.
They validate ESP32-S3 image identity, partition entries and MD5, application
size limits, stale or damaged merged images, excluded filesystem payloads,
identical factory/Launcher output and SHA-256 metadata. Fixtures are synthetic
and created in temporary directories; no firmware compilation or device access
is performed.

The tests establish package structure and consistency. A successful Launcher
installation, boot and return to its menu require separate physical testing.
