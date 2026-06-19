<div align="center">

# 🔍 dk — Advanced WinDbg Extension

**A powerful WinDbg extension for deep Windows kernel and user-mode debugging, with rich visualizations and Time Travel Debugging support.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Release](https://img.shields.io/badge/Release-v1.0.5-brightgreen.svg)](https://github.com/long123king/dk/releases/tag/v1.0.5)
[![Platform: Windows x64 | x86](https://img.shields.io/badge/Platform-Windows%20x64%20%7C%20x86-0078d7.svg)](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/)
[![WinDbg Extension](https://img.shields.io/badge/WinDbg-Extension-orange.svg)](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blueviolet.svg)](https://en.cppreference.com/w/cpp/17)

</div>

---

## ✨ What is dk?

`dk` is an enhanced, extensible WinDbg extension originally derived from [tokenext](https://github.com/long123king/tokenext). It goes far beyond a token inspector — `dk` is a comprehensive toolkit for Windows kernel and user-mode debugging, offering:

- 🧠 **Deep kernel introspection** — processes, threads, tokens, objects, modules, handles, sessions, pools
- 📄 **Memory analysis** — pages, VADs, heap, virtual address regions, pool tracking
- 🎨 **SVG visualizations** — export memory pages, call-stack forests, and address reference trees as interactive SVGs
- ⏱️ **Time Travel Debugging (TTD)** — replay and analyze recorded execution, memory access history, and call events
- 🌐 **Embedded HTTP server** — serve debug data over a local REST API for external tooling
- 🤖 **Debugger Data Model** — leverage WinDbg's modern structured object model
- 🖥️ **x64 and x86 (Win32)** — supports both 64-bit and 32-bit WinDbg sessions

---

## 🖼️ Demos

### Memory Page Visualization (`page_2_svg`)

| Demo | Description |
|------|-------------|
| [Initial version](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000060a11ff000.svg) | Basic page content layout |
| [Buffer & symbol pointers](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052%60182ff730_1.svg) | Local buffer pointers (green), symbol pointers (red) |
| [Heap pointers](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052%60182ff8f0_2.svg) | Heap allocation pointers (blue) |
| [Heap history (clickable)](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052182ff8f0_link.svg) | Heap allocation change history with clickable links |

### TTD Call-Stack Forest Visualization (`dump_ttd_events`)

Generate an interactive SVG call-stack forest from a Time Travel Debugging session (best for small projects):

```windbg
0:001> !dk ldttd
0:001> !dk dump_ttd_events d:\helloworld_viz
```

[📊 View helloworld demo](https://raw.githubusercontent.com/long123king/dk/main/demos/helloworld_viz_forest.svg) *(open locally for click interaction — browsers block SVG scripts)*

---

## 📦 Download

Pre-built binaries for **v1.0.5** are available on the [Releases page](https://github.com/long123king/dk/releases/tag/v1.0.5). Pick the build that matches your WinDbg architecture:

| Build | Architecture | Download |
|-------|-------------|----------|
| **Release x64** *(recommended)* | 64-bit WinDbg | [dk-v1.0.5-Release-x64.zip](https://github.com/long123king/dk/releases/download/v1.0.5/dk-v1.0.5-Release-x64.zip) |
| **Release x86 (Win32)** | 32-bit WinDbg | [dk-v1.0.5-Release-Win32.zip](https://github.com/long123king/dk/releases/download/v1.0.5/dk-v1.0.5-Release-Win32.zip) |
| Debug x64 | 64-bit WinDbg | [dk-v1.0.5-Debug-x64.zip](https://github.com/long123king/dk/releases/download/v1.0.5/dk-v1.0.5-Debug-x64.zip) |
| Debug x86 (Win32) | 32-bit WinDbg | [dk-v1.0.5-Debug-Win32.zip](https://github.com/long123king/dk/releases/download/v1.0.5/dk-v1.0.5-Debug-Win32.zip) |

> **Tip:** Use the Release build for everyday debugging. Use Debug builds only when diagnosing issues with the extension itself.

---

## 🚀 Quick Start

### Option A — Use a pre-built release *(recommended)*

1. Download the ZIP for your architecture from the [📦 Download](#-download) section above.
2. Extract it to a folder, e.g. `C:\tools\dk\`.
3. Load the extension in WinDbg:
   ```windbg
   .load C:\tools\dk\dk.dll
   ```
4. Verify:
   ```windbg
   !dk help
   ```

### Option B — Build from source

**Prerequisites**

- Windows 10/11
- [WinDbg Preview](https://apps.microsoft.com/detail/windbg-preview/9PGJGD53TN86) or classic WinDbg
- [Windows SDK / WDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/) with Debugging Tools
- Visual Studio 2019 or later

**Steps**

1. Clone the repository:
   ```sh
   git clone https://github.com/long123king/dk.git
   ```
2. Open `dk.sln` in Visual Studio.
3. Build in **Release | x64** (or **Release | Win32** for 32-bit) configuration.
4. Load the resulting `dk.dll` in WinDbg as shown above.

---

## 📖 Command Reference

> Run `!dk help` for the full list, or `!dk help <command>` for detailed usage of a specific command.

### Process & Thread

| Command | Description |
|---------|-------------|
| `!dk pses` | List all processes |
| `!dk process [EPROCESS_Addr]` | Dump detailed process information |
| `!dk ps_flags <EPROCESS_Addr>` | Analyze process flags |
| `!dk kill <EPROCESS_Addr>` | Terminate a process |
| `!dk threads` | List threads for the current process |
| `!dk kall` | Dump all stack backtraces (single process) |
| `!dk pkall` | Dump all stack backtraces (all processes) |

### Security (Token / SID / Privileges)

| Command | Description |
|---------|-------------|
| `!dk token [Token_Addr]` | Dump a security token object |
| `!dk sid <SID_Addr>` | Dump a Security Identifier (SID) |
| `!dk add_privilege <Priv_Value>` | Add a privilege to the current token |

### Memory & Pages

| Command | Description |
|---------|-------------|
| `!dk va_regions` | Analyze virtual address regions |
| `!dk page <Virtual_Addr>` | Dump page table translation |
| `!dk pages <Virtual_Addr>` | Dump page table entries for a region |
| `!dk vad <MMVAD_Addr>` | Display VAD information |
| `!dk as_qword <Address> [Count]` | Display memory as QWORDs with type enrichment |
| `!dk as_mem <Address>` | Display memory content with type enrichment |
| `!dk page_2_svg <Addr> <Svg_File>` | Export a page to SVG |
| `!dk pages_2_svg <Addr> <Count> <Svg_File>` | Export continuous pages to SVG |

### Kernel Pool

| Command | Description |
|---------|-------------|
| `!dk poolhdr <Address>` | Decode pool header |
| `!dk bigpool` | Dump kernel Big Pool allocations |
| `!dk poolrange` | Dump pool allocation range |
| `!dk pooltrack` | Analyze pool tracking info |
| `!dk poolmetrics` | Display pool metrics |
| `!dk free_pool <Size_Hex>` | Search for free pool chunks of a given size |

### Heap (User Mode)

| Command | Description |
|---------|-------------|
| `!dk heap_ssum` | Display heap size summary |
| `!dk heap_bysize <Size_Hex>` | List heap allocations of a specific size |
| `!dk heap_oversize <Size_Hex>` | List heap allocations larger than a given size |

### Objects & Handles

| Command | Description |
|---------|-------------|
| `!dk obj <Object_Addr>` | Display Object Header and type info |
| `!dk gobj` | Dump global Root Object Directory |
| `!dk obj_dir <Directory_Addr>` | List Object Directory contents |
| `!dk handle_table <TableAddr>` | Display a handle table |
| `!dk khandles` | Dump kernel handles |

### Modules & PE

| Command | Description |
|---------|-------------|
| `!dk lm` | List all modules |
| `!dk lmu` | List user-mode modules |
| `!dk lmk` | List kernel modules |
| `!dk pe_hdr <Addr>` | Dump PE header |
| `!dk peguid <Address>` | Extract PE GUID |
| `!dk unloaded_pe` | Search for potential unloaded PE modules |

### User-Mode Search & Analysis

| Command | Description |
|---------|-------------|
| `!dk usearch_astr <String>` | Search user memory for an ASCII string |
| `!dk usearch_ustr <String>` | Search user memory for a Unicode string |
| `!dk usearch_bytes <Bytes>` | Search user memory for bytes |
| `!dk usearch_addr <Address>` | Search user memory for an address |
| `!dk uaddr_analyze <Address>` | Analyze a user-mode address |
| `!dk uaddr_ref_tree <Address>` | Display address reference tree |
| `!dk uaddr_analyze_svg <Address>` | Analyze user-mode address, output as SVG |
| `!dk ustacks` | Summarize user-mode stacks |

### Time Travel Debugging (TTD)

| Command | Description |
|---------|-------------|
| `!dk ldttd` | Load/initialize TTD support |
| `!dk ttd_calls` | List TTD calls |
| `!dk ttd_mem_access` | TTD memory access analysis |
| `!dk ttd_mem_use` | TTD memory usage info |
| `!dk ttd_vis_info` | Generate TTD visualization information |
| `!dk dump_ttd_events <Output_Dir>` | Dump TTD events as SVG visualizations |
| `!dk dump_ttd_events_to_csv <Filename>` | Dump TTD events to a CSV file |

### Debugger Data Model

| Command | Description |
|---------|-------------|
| `!dk ls_model <root>` | List model objects (`Sessions`, `Settings`, `State`, `Utility`, `LastEvent`) |
| `!dk ls_sessions` | List model sessions |
| `!dk ls_processes <session_id>` | List model processes |
| `!dk ls_threads <session_id> <pid>` | List model threads |
| `!dk ls_modules <session_id> <pid>` | List model modules |
| `!dk ls_handles <session_id> <pid>` | List model handles |
| `!dk mobj <path>` | Dump a model object at a path |
| `!dk call <path>` | Call a model function |

### Embedded HTTP Server

Start a local REST API server to expose debug data to external tools:

```windbg
0:001> !dk serve_start [host] [port]
```

Default listens on `http://127.0.0.1:8080`. Stop with `GET /api/server/stop`.

---

## 🏗️ Building a New WinDbg C++ Extension

### 1. Add Windows Kits paths to Visual Studio project settings

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <!-- ... -->
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <IncludePath>C:\Program Files (x86)\Windows Kits\10\Debuggers\inc;$(IncludePath)</IncludePath>
    <LibraryPath>C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x64;$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <IncludePath>C:\Program Files (x86)\Windows Kits\10\Debuggers\inc;$(IncludePath)</IncludePath>
    <LibraryPath>C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x64;$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <IncludePath>C:\Program Files (x86)\Windows Kits\10\Debuggers\inc;$(IncludePath)</IncludePath>
    <LibraryPath>C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x86;$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <IncludePath>C:\Program Files (x86)\Windows Kits\10\Debuggers\inc;$(IncludePath)</IncludePath>
    <LibraryPath>C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x86;$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <!-- ... -->
</Project>
```

### 2. Patch `engextcpp.cpp`

Copy `engextcpp.cpp` from `C:\Program Files (x86)\Windows Kits\10\Debuggers\inc` into your project and apply the following fixes:

```diff
248c248
<     m_OptionChars = "/-";
---
>     m_OptionChars = const_cast<PSTR>("/-");
286c286
<     PSTR Value = "";
---
>     PSTR Value = const_cast<PSTR>("");
2673c2673
<              BufferChars > 0)
---
>              *BufferChars > 0)
```

### 3. Define exported symbols in `dk.def`

Every WinDbg extension DLL must export at minimum these four symbols:

```
DebugExtensionInitialize
DebugExtensionUninitialize
DebugExtensionNotify
help
```

---

## 📚 References

- [TTD.hpp](https://github.com/commial/ttd-bindings/blob/master/TTD/TTD.hpp) — from [Bindings for Microsoft WinDBG TTD](https://github.com/commial/ttd-bindings)
- [Debugger Data Model C++ Overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/data-model-cpp-overview)
- [Time Travel Debugging Overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/time-travel-debugging-overview)
- [tokenext](https://github.com/long123king/tokenext) — the original project that dk evolved from

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

