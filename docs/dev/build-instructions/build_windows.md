# Building on Windows 

On Windows we only support [static builds](build_static.md).

## Prerequisites

First, [install Visual Studio](setup_visual_studio.md) in order to be able to compile the necessary code.

You'll also need Python (`winget install --id=Python.Python.3.13 -e`).

## Inner loop

You'll need to set up a window with Visual Studio's variables. You can open a Visual Studio CMD window, but you can set up an existing PowerShell window:

```pwsh
# Load vcvars64.bat into your PowerShell window.
# You can find this path with the `vswhere.exe` tool (C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe)
# that ships with Visual Studio:
# 
# vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find "VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" && set" |
    ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") { [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2]) }
    }
```

Then, in the `mumble/` directory, set up CMake. For example:

```pwsh
# Set this to wherever you set up vcpkg.
$env_root = "C:\Users\me\Projects\mumble-env\mumble_env.x64-windows-static-md.b1fe4a4257"

# Configure cmake, e.g.
cmake -G Ninja `
    -S "C:\Users\me\Projects\mumble" `
    -B "C:\Users\me\Projects\mumble\build" `
    -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
    -Dstatic=ON `
    "-DCMAKE_TOOLCHAIN_FILE=$env_root\scripts\buildsystems\vcpkg.cmake" `
    "-DIce_HOME=$env_root\installed\x64-windows-static-md" `
    -DCMAKE_BUILD_TYPE=Release
```

Then build:

```pwsh
cmake --build "C:\Users\me\Projects\mumble\build" --parallel
```

For full guidance, see [static builds](build_static.md).