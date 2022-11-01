# dk WinDbg extension

# Summary

dk is the enhanced refactored version of [tokenext](https://github.com/long123king/tokenext). The goal is to improve the readability and extensibility, as well as to leverage the powerful [Debugger Data Model](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/data-model-cpp-overview) and [Time Travel Debugging](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/time-travel-debugging-overview). SVG document will be generated for an intuitive visualization in certain circumstances.

Run ***!dk help*** for supported command list.

Check page_2_svg demo [here](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000060a11ff000.svg) and [here with pointers](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052%60182ff730_1.svg) and [here with local/symbol/heap pointers](https://raw.githubusercontent.com/long123king/dk/main/demos/page_00000052%60182ff8f0_2.svg).

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

