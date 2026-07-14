# Release Notes - v1.0.0

Welcome to the **v1.0.0** release of the Bruce Firmware project! This major update brings a complete ecosystem for third-party app development (Bruce App Packages) and a brand-new visual design tool.

## 🚀 Key Features

### 1. Bruce UI Designer (Pro)
We've introduced a powerful, web-based drag-and-drop UI designer to help you build interfaces for Bruce faster than ever:
- **Infinite Canvas & Multi-Scene Artboards:** Work on multiple device screens simultaneously in a Figma-style workspace.
- **Cross-Scene Drag & Drop:** Move UI elements seamlessly between different screens.
- **Auto Code Generation:** Design your UI visually and instantly copy the generated C++ code to paste into your Bruce firmware apps.
- **Web Deployment:** The designer is now automatically deployed to GitHub Pages for instant access anywhere.

### 2. Bruce App Package (BAP) Ecosystem
You can now build and run third-party applications dynamically from the SD card without flashing the entire firmware!
- **BAP Architecture:** Fully documented API and execution model (`docs/bruce_app_package.md`).
- **Dynamic ELF Loader:** Implemented `bap_loader` and `elf_loader` in the core firmware to load compiled binaries into memory and execute them safely.
- **Bruce Build Tool (`bbt.py`):** A custom Python script that packages your compiled `.elf` apps into Bruce App Manifest (`.bam`) files.

### 3. Developer SDK & Templates
- **Template App:** Added a `template_app` folder containing `hello_bruce.c`, a custom linker script (`app.ld`), and `CMakeLists.txt`. It's a ready-to-go boilerplate for creating your own BAP apps.
- **API Headers:** Added `include/bruce_api.h` providing a stable ABI (Application Binary Interface) for apps to interact with the core firmware (UI drawing, buttons, delays).
- **Documentation:** Extensive documentation added in `docs/` covering everything from file formats to API references and app development workflows.

## 🛠️ Fixes & Optimizations
- Automated Git workflows to manage GitHub Pages deployment and automated builds.
- Refactored core menu system (`AppsMenu`) to scan for and list installed `.bap` files directly from the SD card.
- Cleaned up repository structure and updated `.gitignore` to prevent BAP build artifacts from cluttering the repository.

---

*Thank you for contributing to the Bruce ecosystem! 🦇*
