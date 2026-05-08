![git](https://github.com/linuxtopG/html_url_to_deb/blob/main/htod.jpg)


# WebApp Converter

Convert any URL or HTML/CSS/JS directory into a **standalone Linux desktop application** with embedded Chromium (Qt 6 WebEngine). The generated app is a real native binary — not a wrapper script — and can be packaged as a `.deb` for distribution.

---

## Requirements (for building the converter tool)

| Tool / Library | Package (Debian) | Purpose |
|---|---|---|
| **CMake** ≥ 3.21 | `cmake` | Build system |
| **C++17 compiler** | `build-essential` (g++) | Compilation |
| **Qt 6 Core** | `qt6-base-dev` | CLI & file I/O |

## Requirements (for building generated apps)

| Tool / Library | Package (Debian) | Purpose |
|---|---|---|
| **CMake** ≥ 3.21 | `cmake` | Build system |
| **C++17 compiler** | `build-essential` (g++) | Compilation |
| **Qt 6 Widgets** | `qt6-base-dev` | Window, menus, dialogs |
| **Qt 6 WebEngine** | `qt6-webengine-dev` | Embedded Chromium browser |
| **Qt 6 PrintSupport** | `libqt6printsupport6` | PDF printing via `printToPdf()` |
| **dpkg-deb** | `dpkg-dev` | .deb packaging (optional) |

---

## Debian / Ubuntu — تثبيت جميع المكتبات المطلوبة

### لتشغيل الأداة (webapp-converter)

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev
```

### لبناء التطبيقات المولّدة (generated apps)

```bash
sudo apt install -y qt6-webengine-dev libqt6printsupport6
```

### كامل الأمر مرة واحدة (كل ما تحتاجه)

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev qt6-webengine-dev libqt6printsupport6
```

---

## Quick Start

### 1. Build the converter

```bash
git clone <repo-url> webapp-converter
cd webapp-converter
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
sudo cp webapp-converter /usr/local/bin/
```

### 2. Generate a desktop app — from a URL

```bash
webapp-converter --url https://github.com --name githubt --build
```

This creates a `githubt/` folder with a complete C++/Qt project, then compiles it.  
The resulting binary is at `githubt/build/githubt`.

### 3. Generate a desktop app — from local HTML/CSS/JS

```bash
webapp-converter --dir ./my-web-app/ --name MyApp --icon icon.png --build
```

All files in `./my-web-app/` are embedded into the binary via Qt resources.  
The app loads `index.html` at startup.

### 4. Build a .deb package

```bash
cd MyApp && ./build-deb.sh
```

Produces `MyApp-1.0.0-linux-amd64.deb` in `build-deb/`.

---

## CLI Reference

```
Usage: webapp-converter [options]

Options:
  --url <url>          URL of the web application to convert
  --dir <dir>          Local directory with HTML/CSS/JS files
  --name <name>        Application name (required)
  --icon <path>        Path to application icon (PNG/SVG)
  --output <dir>       Output directory (default: ./<name>)
  --app-version <ver>  Application version (default: 1.0.0)
  --build              Build the generated project after creation
  -h, --help           Show help
  -v, --version        Show version
```

---

## How It Works

```
┌─────────────────────────────────────────────────────────┐
│                   webapp-converter CLI                   │
│  (Qt6::Core – no WebEngine needed at build time)         │
└──────────┬──────────────────────────────────────┬────────┘
           │ --url                                │ --dir
           ▼                                      ▼
┌──────────────────────┐            ┌──────────────────────────┐
│  Generated C++/Qt    │            │  Generated C++/Qt        │
│  Project (URL mode)  │            │  Project (local mode)    │
│  - loads remote URL  │            │  - bundles files in .qrc │
│  - QWebEngineProfile │            │  - loads via qrc:///web/ │
│    with persistent    │            │  - same engine features  │
│    cookies + disk     │            │                          │
│    cache              │            │                          │
└──────────────────────┘            └──────────────────────────┘
         │                                       │
         └───────────────┬───────────────────────┘
                         ▼
            ┌─────────────────────────┐
            │  CMake + CPack          │
            │  ──── build ────        │
            │  Native binary          │
            │  (Qt6::Widgets          │
            │   Qt6::WebEngineWidgets │
            │   Qt6::PrintSupport)    │
            │  ──── cpack ────        │
            │  .deb package           │
            └─────────────────────────┘
```

### Key features of generated apps

| Feature | Implementation |
|---|---|
| **Browser engine** | Qt WebEngine (Chromium) — full JS, CSS3, WebGL |
| **Internet access** | Enabled (`LocalContentCanAccessRemoteUrls`) |
| **Login persistence** | Named `QWebEngineProfile` with `ForcePersistentCookies` + disk HTTP cache (100 MB max) |
| **Session save** | Cookies & cache stored in `~/.local/share/<AppName>/storage/` |
| **Print to PDF** | `printToPdf()` callback → saves PDF → opens with system viewer |
| **Icon** | Custom PNG/SVG or embedded default SVG |
| **Kiosk-style** | No address bar, no navigation UI — pure web content |

---

## Generated Project Structure

```
MyApp/
├── CMakeLists.txt           # CMake build file
├── main.cpp                 # Entry point, profile setup, page creation
├── MainWindow.h             # Window header (Qt6 WebEngine)
├── MainWindow.cpp           # Window implementation (print, zoom, nav)
├── app.qrc                  # Qt resource file (local mode only)
├── MyApp.desktop            # Linux .desktop entry
├── build.sh                 # Quick build script
├── build-deb.sh             # .deb package script
└── resources/               # Icon, web files, etc.
```

---

## Architecture

- **Converter tool** (`webapp-converter`): C++ CLI that generates C++/Qt projects from templates embedded as raw string literals in `ProjectGenerator.cpp`. Only depends on `Qt6::Core`.
- **Generated app**: A real Qt 6 application with `QWebEngineView`, `QWebEngineProfile`, `QMainWindow`. It opens the target URL or loads local files from Qt resources.
- **No scripts or wrappers** — the generated app is a compiled native executable.

---

## License

MIT
