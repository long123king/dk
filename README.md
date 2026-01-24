# dk WinDbg extension

![Build Status](https://github.com/long123king/dk/workflows/Build/badge.svg)

# Summary

dk is the enhanced refactored version of [tokenext](https://github.com/long123king/tokenext). The goal is to improve the readability and extensibility, as well as to leverage the powerful [Debugger Data Model](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/data-model-cpp-overview) and [Time Travel Debugging](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/time-travel-debugging-overview). SVG document will be generated for an intuitive visualization in certain circumstances.

Run ***!dk help*** for supported command list.

Check page_2_svg demos:
1. [Initial version](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000060a11ff000.svg)
2. [Add pointers to local buffer(in green), pointers to symbols(in red)](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052%60182ff730_1.svg)
3. [Add pointers to heap allocations(in blue)](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052%60182ff8f0_2.svg)
4. [Add heap allocation changing history, blue rects are clickable](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052182ff8f0_link.svg)

Run following commands to generate callstack forest visualization in svg format(small projects only!), demos for helloworld project can be found  [here](https://raw.githubusercontent.com/long123king/dk/main/demos/helloworld_viz_forest.svg). (Click interaction on svg is blocked by browsers, try it locally)

```
0:001> !dk ldttd
0:001> !dk dump_ttd_events d:\helloworld_viz
```



# Reference

1. [TTD.hpp](https://github.com/commial/ttd-bindings/blob/master/TTD/TTD.hpp) from [Bindings for Microsoft WinDBG TTD](https://github.com/commial/ttd-bindings)

# How to start a new WinDbg C++ extension?

## 1. Add Windows Kits related folder to Visual Studio project setting:

``` xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
    ......
<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <IncludePath>C:\Program Files (x86)\Windows Kits\10\Debuggers\inc;$(IncludePath)</IncludePath>
    <LibraryPath>C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x64;$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <IncludePath>C:\Program Files (x86)\Windows Kits\10\Debuggers\inc;$(IncludePath)</IncludePath>
    <LibraryPath>C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x64;$(LibraryPath)</LibraryPath>
  </PropertyGroup>
    ......
</Project>
```

## 2. Include engextcpp.cpp from C:\Program Files (x86)\Windows Kits\10\Debuggers\inc to Visual Studio project, and make the following changes:

```diff
diff -r C:\Program Files (x86)\Windows Kits\10\Debuggers\inc\engextcpp.cpp C:\Users\dk\source\repos\dk\engextcpp.cpp
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

## 3. Define exported symbols in dk.def file, and don't forget the 4 default exports:

```configuration
DebugExtensionInitialize
DebugExtensionUninitialize
DebugExtensionNotify
help
```

# CI/CD

## Build Status

The project uses GitHub Actions for continuous integration. The build workflow automatically:
- Installs Windows SDK with Debugging Tools
- Builds both Debug and Release configurations for x64 platform
- Runs verification checks
- Uploads build artifacts

You can download the latest build artifacts (DLL files) from the [Actions tab](https://github.com/long123king/dk/actions) on GitHub.

## Building Locally

### Prerequisites

1. **Visual Studio 2022** with C++ desktop development workload
2. **Windows SDK 10** with Debugging Tools for Windows

### Installing Debugging Tools for Windows

The build requires Windows Debugging Tools, which provides the necessary headers and libraries (`engextcpp.hpp`, `dbgeng.lib`).

#### Option 1: Using Chocolatey (Recommended for CI/CD)
```powershell
choco install windows-sdk-10.1 --params="/features:OptionId.WindowsDesktopDebuggers" -y
```

#### Option 2: Using Windows SDK Installer
1. Download the [Windows SDK Installer](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/)
2. During installation, ensure you select **"Debugging Tools for Windows"**
3. The tools will be installed to:
   - `C:\Program Files (x86)\Windows Kits\10\Debuggers\`

#### Option 3: Standalone Debuggers Installation
1. Download from the [Microsoft Download Center](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/debugger-download-tools)
2. Install and note the installation path

### Build Steps

1. Open `dk.sln` in Visual Studio 2022
2. Select the desired configuration (Debug or Release) and platform (x64)
3. Build the solution (Ctrl+Shift+B)
4. The output DLL will be in `x64\Debug\dk.dll` or `x64\Release\dk.dll`

#### Command Line Build
```powershell
# Build Debug configuration
msbuild dk.sln /p:Configuration=Debug /p:Platform=x64

# Build Release configuration  
msbuild dk.sln /p:Configuration=Release /p:Platform=x64
```

## Troubleshooting

### Error: Cannot find engextcpp.hpp

**Symptom:** Build fails with error about missing `engextcpp.hpp`

**Solution:**
1. Ensure Windows SDK with Debugging Tools is installed
2. Verify the file exists at: `C:\Program Files (x86)\Windows Kits\10\Debuggers\inc\engextcpp.hpp`
3. If installed to a different location, update the paths in `dk.vcxproj`:
   ```xml
   <IncludePath>YOUR_PATH\Debuggers\inc;$(IncludePath)</IncludePath>
   <LibraryPath>YOUR_PATH\Debuggers\lib\x64;$(LibraryPath)</LibraryPath>
   ```

### Error: Cannot find dbgeng.lib

**Symptom:** Linker error about missing `dbgeng.lib`

**Solution:**
1. Verify the file exists at: `C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x64\dbgeng.lib`
2. Ensure the `LibraryPath` in `dk.vcxproj` points to the correct location

### Error: 'std::byte' conflicts

**Symptom:** Compilation errors related to `std::byte`

**Solution:**
This is already handled in the project with `/D _HAS_STD_BYTE=0` compiler flag. If you still encounter issues, ensure you're using C++20 standard as configured in the project.

### Platform Toolset Issues

**Symptom:** Error about missing platform toolset v143

**Solution:**
1. Install Visual Studio 2022 with C++ workload
2. Alternatively, modify the project to use your installed toolset (v142 for VS2019, v141 for VS2017)

## Project Configuration

The project is configured for x64 builds with the following settings:
- **Platform Toolset:** v143 (Visual Studio 2022)
- **C++ Standard:** C++20
- **Character Set:** Unicode
- **Output Type:** DynamicLibrary (.dll)
- **Include Paths:** 
  - `C:\Program Files (x86)\Windows Kits\10\Debuggers\inc`
- **Library Paths:**
  - `C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\x64`
- **Additional Dependencies:** dbgeng.lib

