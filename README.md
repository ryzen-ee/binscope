# BinScope

<p align="center">
  <img src="ico/main.png" alt="BinScope Icon" width="128" height="128">
</p>

<p align="center">
  <strong>Binary File Analyzer</strong>
</p>

---

## Overview

**BinScope** is a powerful desktop application for analyzing binary files (PE, ELF, MSI, etc.).

> [!NOTE]
> BinScope helps mainly in identifying string content in executables (it works and it helps :D)

---

## Features

### 🔍 File Analysis
- **Overview Tab** — File name, size, format, architecture, hashes (MD5, SHA1, SHA256), entry point, subsystem
- **Sections Tab** — Virtual address, virtual size, raw size, entropy, permissions, suspicious flags
- **Imports Tab** — DLL names, imported functions, addresses, and suspicious API highlighting
- **Strings Tab** — Extracted printable strings categorized as paths, URLs, registry keys, commands
- **Data Tab** — Hex view with address, hex, and ASCII representation

### 🎨 Theme Support
- **Dark Mode** (default)
- **Light Mode**
- Toggle easily from Settings

### ⚙️ Settings
- Switch between dark/light themes
- Enable **Debug Mode** for testing with placeholder data

### 🔧 Additional Features
- Threaded analysis (no UI freezing)
- Progress bar with ETA for large files
- Copy support for tables and text
- Config file saved to user config directory

> [!WARNING]
> Large files may take longer to analyze. A progress bar with ETA will be displayed during analysis.


## Building

### Prerequisites

> [!IMPORTANT]
> - Qt5
> - OpenSSL
> - CMake 3.16+

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make -j4
```

> [!TIP]
> The executable will be created at `build/pe`.

---

## Usage

1. **Launch** the application
2. **Select** a binary file using Browse or drag-and-drop
3. **Click** Analyze to start the analysis
4. **Navigate** through tabs to view different aspects of the binary
5. Use **Settings** to change theme or enable Debug Mode

> [!TIP]
> Enable **Debug Mode** in Settings to test the application with placeholder data without loading actual files!

---

## Configuration

> [!IMPORTANT]
> Settings are stored in:
> - **Linux/macOS**: `~/.config/ryzeneebinscope/config.json`
> - **Windows**: `%APPDATA%\ryzeneebinscope\config.json`

---

## License

> [!NOTE]
> MIT License

---

<p align="center">Built with Qt5 ❤️</p>
