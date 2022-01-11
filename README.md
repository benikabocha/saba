# Saba

[![Build Status](https://travis-ci.org/benikabocha/saba.svg?branch=master)](https://travis-ci.org/benikabocha/saba)
[![Build status](https://ci.appveyor.com/api/projects/status/kjk8chdx0du65m3n?svg=true)](https://ci.appveyor.com/project/benikabocha/saba)
[![Build status](https://github.com/benikabocha/saba/actions/workflows/build-linux.yml/badge.svg)](https://github.com/benikabocha/saba/actions/workflows/build-linux.yml)

[日本語(japanese)](./README.jp.md)

* [MMD](https://sites.google.com/view/vpvp/) (PMD/PMX/VMD) play and load library
* Viewer (MMD/OBJ)
    * saba_viewer
* Example
    * simple_mmd_viewer_glfw (OpenGL 4.1)
    * simple_mmd_viewer_dx11 (DirectX 11)
    * simple_mmd_viewer_vulkan (Vulkan 1.0.65)
    * Transparent Window (GLFW 3.3.2 or higher, Windows)

![saba_viewer](./images/saba_viewer_01.png)
![transparent_sample](./images/transparent_sample.gif)
© 2017 Pronama LLC

## Important Changes
[commit 7ba7020](https://github.com/benikabocha/saba/commit/7ba70208741d6cd5f0105f27a2264b555a5ce043)

UV was flipped with this commit.


## Environment

* Windows
  * Visual Studio 2019
  * Visual Studio 2017
  * Visual Studio 2015 Update 3
* Linux
* Mac

## File types

* OBJ
* PMD
* PMX
* VMD
* VPD
* x file (mmd extension)

## How to build

Please install CMake before the build.

### Required libraries

Please prepare the following libraries.

* OpenGL
* [Bullet Physics](http://bulletphysics.org/wordpress/)
* [GLFW](http://www.glfw.org/)

#### mingw

Do not use the `msys/cmake`.
Pelase use the `mingw64/mingw-w64-x86_64-cmake`.

https://gitlab.kitware.com/cmake/cmake/-/issues/21649

Prepare the mingw64 environment as follows.

```
pacman -S base-devel mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-ninja
pacman -S mingw-w64-x86_64-mesa
```

### 1. Setup Bullet Physics

#### Setup Bullet Physics (on Windows)

Build Bullet Physics as follows.

```
cmake -G "Visual Studio 14 2015 Win64" ^
    -D CMAKE_INSTALL_PREFIX=<Your Bullet Physics install directory> ^
    -D INSTALL_LIBS=ON ^
    -D USE_MSVC_RUNTIME_LIBRARY_DLL=On ^
    -D BUILD_CPU_DEMOS=Off ^
    -D BUILD_OPENGL3_DEMOS=Off ^
    -D BUILD_BULLET2_DEMOS=Off ^
    -D BUILD_UNIT_TESTS=Off ^
    ..

cmake --build . --config Debug --target ALL_BUILD
cmake --build . --config Debug --target INSTALL
cmake --build . --config Release --target ALL_BUILD
cmake --build . --config Release --target INSTALL
```

Please change `-G "Visual Studio 14 2015 Win64"` according to your environment.

#### Setup Bullet Physics (on Mac)

```
brew install bullet
```

#### Setup Bullet Physics (on Linux)

Ubuntu:

```
apt-get install libbullet-dev
```

Arch linux:

```
pacman -S bullet
```

#### Setup Bullet Physics (on mingw)

```
pacman -S mingw-w64-x86_64-bullet
```

### 2. Setup GLFW

#### Setup GLFW (on Windows)

[Download](http://www.glfw.org/download.html)

#### Setup GLFW (on Mac)

```
brew install glfw
```

#### Setup GLFW (on Linux)

Ubuntu:

```
apt-get install libglfw3-dev
```

Arch linux:

```
pacman -S glfw
```

#### Setup GLFW (on mingw)

```
pacman -S mingw-w64-x86_64-glfw
```

### 3. Clone Saba

```
git clone https://github.com/benikabocha/saba.git
cd saba
```

### 4. Run CMake and build

#### Run CMake and build (on Windows)

```
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" ^
    -D SABA_BULLET_ROOT=<Your Bullet Physics install directory> ^
    -D SABA_GLFW_ROOT=<your GLFW install directory> ^
    ..
```

Open the created sln file in Visual Studio and build it.

"saba_viewer" project is the viewer application.

#### Run CMake and build (on Mac/Linux)

```
mkdir build
cd build
cmake ..
make -j4
./saba_viewer
```

If the operation is heavy, please try the following.

```
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make -j4
```

#### Run CMake and build (on mingw)

```
mkdir build
cd build
cmake ..
ninja
./saba_viewer
```

## Initial setting

Initialize with the "init.json" or "init.lua" file placed in the current directory.

Write "init.json" or "init.lua" file in UTF-8.

### 1. Example "init.json"
```javascript
{
    "MSAAEnable":	true,
    "MSAACount":	8,
    "Commands":[
        {
            "Cmd":"open",
            "Args":["test.pmx"]
        },
        {
            "Cmd":"open",
            "Args":["test.vmd"]
        }
    ]
}
```
#### MSAAEnable

Enable MSAA.

#### MSAACount

Set the number of MSAA samples.

#### Commands

Set the commands to be executed at startup.

### 2. Example "init.lua"

```lua
MSAA = {
    Enable  = true,
    Count    = 8
}

InitCamera = {
    Center = {x = 0, y = 10, z = 0},
    Eye = {x = 0, y = 10, z = 50},
    NearClip = 1.0,
    FarClip = 2000.0,
    Radius = 100
}

InitScene = {
    UnitScale = 10
}

Commands = {
    {
        Cmd     = "open",
        Args    = {"test.pmx"},
    },
    {
        Cmd     = "open",
        Args    = {"test.vmd"},
    },
    {
        Cmd     = "play"
    },
}
```

#### MSAA.Enable

Enable MSAA.

#### MSAA.Count

Set the number of MSAA samples.

#### InitCamera.Center

Set camera center position at scene initialization.

#### InitCamera.Eye

Set camera eye position at scene initialization.

#### InitCamera.NearClip

Set camera near clip at scene initialization.

#### InitCamera.FarClip

Set camera far clip at scene initialization.

#### InitCamera.Radius

Set camera zoom radius at scene initialization.

#### InitScene.UnitScale

Set grid size at scene initialization.

#### Commands

Set the commands to be executed at startup.

#### Run Script

You can run the script.

The command line argument is the "Args" variable.

```lua
ModelIndex = 1
print(Args[1])

Commands = {
    {
        Cmd     = "open",
        Args    = {Models[ModelIndex]},
    },
    {
        Cmd     = "open",
        Args    = {"test.vmd"},
    },
    {
        Cmd     = "play"
    },
}
```

## How to use

Drag and drop files, or use the "open" command.

![saba_viewer](./images/saba_viewer_02.png)

### MMD

1. Drag and drop model(PMD/PMX) file.
2. Drag and drop motion(VMD) file.

### Camera

Drag the mouse to move the camera.

* Left Button (z + Left Button) : Rotate
* Right Button (c + Left Button) : Zoom
* Middle Button (x + Left Button) : Translate

### Commands

#### open

`open <file path>`

Open the file.

Supported file types.

* OBJ
* PMD
* PMX
* VMD

The model file will be selected when opened.
The model name will be `model_xxx`(`nnn` is ID).

#### select

`select <model name>`

Select a model.


#### clear

`clear [-all]`

Clear a model.

If invoked with no arguments, it clears the selected model.

If `-all` is specified, all models will be cleared.

#### play

`play`

Play the animation.

#### stop

`stop`

Stop the animation.

#### translate

`translate x y z`

Translate the selected model.

#### rotate

`rotate x y z`

Rotate the selected model.

#### scale

`scale x y z`

Scale the selected model.

#### refreshCustomCommand

`refreshCustomCommand`

Refresh the custom command.

#### enableUI

`enableUI [false]`

Switch the display of the UI.

`F1` key works the same way.

#### clearAnimation

`clearAnimation [-all]`

Clear animation of selected model.

#### clearSceneAnimation

`clearSceneAnimation`

Clear animation of scene(eg camera).

## Custom command

You can create custom commands using Lua.

When "command.lua" is placed in the current directory and started up, the custom command written in Lua is loaded.

Write "command.lua" with UTF - 8.

For example, you can register a model or animation load as a macro.

```lua
function OpenModel(files)
    return function ()
        ExecuteCommand("clear", "-all")
        for i, filename in ipairs(files) do
            ExecuteCommand("open", filename)
        end
    end
end

function OpenAnim(files, isPlay)
    return function ()
        ExecuteCommand("clearAnimation", "-all")
        ExecuteCommand("clearSceneAnimation")
        for i, filename in ipairs(files) do
            ExecuteCommand("open", filename)
        end
        if isPlay then
            ExecuteCommand("play")
        end
    end
end

-- Register Model Load Command
RegisterCommand("", OpenModel({"Model1_Path"}), "01_Model/Menu1")
RegisterCommand("", OpenModel({"Model2_Path"}), "01_Model/Menu2")

-- Register Animation Load Command
anims = {
    "ModelAnim_Path",
    "CameraAnim_Path",
}
RegisterCommand("", OpenAnim(anims, true), "02_Anim/Anim1")

```

Here are functions that can be used in "command.lua".

### RegisterCommand

```lua
RegisterCommand(commandName, commandFunc, menuName)
-- Register command.

-- commandName : Command name
-- If it is empty, the command name is set automatically.。

-- commandFunc : Command function

-- menuName : Menu name
-- This is the name when registering a custom command in the menu.。
-- If it is empty it will not be added to the menu.
-- '/' Separate the menu hierarchy.
```
### ExecuteCommand

```lua
ExecuteCommand(command, args)
-- Execute the command.

-- command : Execute the command.

--- args : Arguments to pass to the command.
--- It is a string or table.
```

## How to use library

[Wiki](https://github.com/benikabocha/saba/wiki/How-to-use-library)

### Use case

* sanko-shoko.net (AR Example) http://www.sanko-shoko.net/note.php?id=sbtj
