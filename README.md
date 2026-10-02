# 📦 glTF 2.0 Importer/Exporter for Autodesk 3ds Max

This project adds glTF™ (2.0) file access capabilities to Autodesk 3ds Max®, providing a seamless round-trip workflow for importing, editing, and exporting 3D content in glTF format.

## 📋 Table of Contents

1. [Introduction](#-introduction)
2. [User Documentation](#-user-documentation)
3. [Build Requirements](#-build-requirements)
4. [Installation](#-installation)
5. [Trademarks](#-trademarks)

---

## 🚀 Introduction

This project consists of two main plug-in build projects:

- <b>`KHRglTFImporter`</b>: A plug-in to import glTF files into 3ds Max scenes.
- <b>`KHRglTFExporter`</b>: A plug-in to export 3ds Max scene data to glTF files.

---

## 📖 User Documentation

<b>[End-User Documentation](./User_Documentation/README.md)</b> is divided into sections:

- **glTF Importer** options for importing and formatting the scene for use in 3ds Max.
- **glTF Editing** tools within 3ds Max for editing and preparing glTF content.
- **glTF Exporter** options for exporting the scene out from 3ds Max into glTF format.

---

## ⚙️ Build Requirements

To build or use this project, the following environment is required:

### 💻 Build Environment (Compiler and Platform Toolsets)

- **Microsoft Visual Studio 2022**
- **MSVC v143 - VS 2022 C++ x64/x86 build tools**: For 3ds Max 2026, and 2027
- **MSVC v142 - VS 2019 C++ x64/x86 build tools**: For 3ds Max 2023, 2024 and 2025
- **MSVC v141 - VS 2017 C++ x64/x86 build tools**: For 3ds Max 2020, 2021 and 2022
- **Git** and **CMake** executables in your PATH to build the 3rd-party dependencies

### 🧰 Required SDK

- **Autodesk 3ds Max SDK**: Provided by Autodesk.

### 🌐 Environment Variables

The project requires the 3ds Max SDK environment variables to be configured correctly. These environment variables are automatically set while installing the 3ds Max SDKs.

If environment variables are missing, you must manually set them to point to the correct installation path of the SDKs. Use the variable name corresponding to your specific 3ds Max version:

| 3ds Max Version | Environment Variable Name |
| :-------------- | :------------------------ |
| 2020            | `ADSK_3DSMAX_SDK_2020`    |
| 2021            | `ADSK_3DSMAX_SDK_2021`    |
| 2022            | `ADSK_3DSMAX_SDK_2022`    |
| 2023            | `ADSK_3DSMAX_SDK_2023`    |
| 2024            | `ADSK_3DSMAX_SDK_2024`    |
| 2025            | `ADSK_3DSMAX_SDK_2025`    |
| 2026            | `ADSK_3DSMAX_SDK_2026`    |
| 2027            | `ADSK_3DSMAX_SDK_2027`    |

### 🛠️ Initial Building of 3rd-Party Dependencies

The project depends on several prebuilt 3rd-party libraries, which must be available in the <b>`3rdParty\_dist\_v141\Debug`</b> and <b>`3rdParty\_dist\_v141\Release`</b> directories.

- A batch script is provided to build these libraries from source: <b>`3rdParty\build_dependencies.cmd`</b>
- The script should be run once from a **Visual Studio x64 Developer Command Prompt**.
- The script will fetch, build, and install all required dependencies into <b>`3rdParty\_dist_v141`</b>.
- The script relies on **MSVC v141 - VS 2017 C++ x64/x86 build tools** to be installed.
- The <b>`BuildAll.cmd`</b> and <b>`BuildForAvailableSDKs.cmd`</b> described below perform this step automatically.

### 🛠️ Building the Project

The solution includes two primary build configurations for each 3ds Max version:

- <b>`Release-{Max Version}`</b>: Production-ready build.
- <b>`Hybrid-{Max Version}`</b>: Optimized build with debug symbols (for development).

To build all release configurations at once, either use Visual Studio's "Batch Build..." functionality or run one of the accompanying batch scripts from inside the **Visual Studio x64 Developer Command Prompt**:

- <b>`BuildAll.cmd`</b>: Build for all 3ds Max Versions
- <b>`BuildForAvailableSDKs.cmd`</b>: Build for all 3ds Max Versions with matching SDK available

### 🔧 Adding New 3ds Max Versions

To keep project files clean, the recommended way to add new 3ds Max build targets is by manually editing the files below. Find the blocks related to the latest version and duplicate them, updating the year accordingly.

The example below adds the build configuration for 3ds Max 2027 by cloning and editing the entries for the 3ds Max 2026 configuration.

---

- **Solution File (`KHRglTF.sln`)**
  Duplicate all lines containing `Max2026` and change the target to `Max2027` in the new lines.
- **Project Files (`KHRglTFImporter.vcxproj`, `KHRglTFExporter.vcxproj`)**
  Duplicate the XML `<ProjectConfiguration>` elements containing `Max2026`. Update the year to `Max2027` for both the **Release** and **Hybrid** configurations.
- **Property Sheets (`MaxSDKSetup.props`)**
  Duplicate the XML lines/property groups containing `Max2026` and update them to `Max2027`.

---

## 💾 Plugin Installation

Once built, the following folders are generated in the `Packages` folder for each supported 3ds Max version:

```text
Packages/
├─ 3ds Max 2020/
│  └─ Plugins/
│     ├─ KHRglTFExporter_2020.dle
│     └─ KHRglTFImporter_2020.dli
│
⋮  [Intermediate 3ds Max versions]
│
└─ 3ds Max 2027/
   └─ Plugins/
      ├─ KHRglTFExporter_2027.dle
      └─ KHRglTFImporter_2027.dli
```

Copy the files matching your 3ds Max version into the `Plugins` folder of your 3ds Max installation directory.

---

### 🎨 Khronos PBR Neutral Tone Mapping

This importer includes a feature to switch the Color Management to use Khronos PBR Neutral Tone when loading glTF files.

To enable this feature (3ds Max 2024 and later): Copy the tone map file located in the `Packages` folder into your 3ds Max installation directory.

---

## ⚖️ Trademarks

- **3ds Max®** is a registered trademark of Autodesk, Inc.
- **glTF™** is a registered trademark of Khronos Group Inc.
- **V-Ray®** and **Chaos Corona®** are registered trademarks of Chaos Software EOOD.
- **Pencil+®** is a registered trademark of P SOFTHOUSE CO., LTD.

### 📚 3rd Party Libraries

This project incorporates the following open-source libraries. We acknowledge and appreciate the contributions of these projects:

- [KTX-Software](https://github.com/KhronosGroup/KTX-Software): Licensed under the Apache License 2.0. Copyright © 2013-2020 Mark Callow, The Khronos Group Inc.
- [Draco](https://github.com/google/draco): Licensed under the Apache License 2.0. Copyright © 2016 Google Inc.
- [libwebp](https://chromium.googlesource.com/webm/libwebp) / libsharpyuv: Licensed under the BSD 3-Clause License. Copyright (c) 2010, Google Inc. All rights reserved.
- [JsonCpp](https://github.com/open-source-parsers/jsoncpp): Licensed under the MIT License or Public Domain. Copyright © 2007-2010 Baptiste Lepilleur and The JsonCpp Authors.
- [TinyGLTF](https://github.com/syoyo/tinygltf): Licensed under the MIT License. Copyright © 2015-Present Syoyo Fujita.
- [cgltf](https://github.com/jkuhlmann/cgltf): Licensed under the MIT License. Copyright © 2018 Johannes Kuhlmann.

_Note: TinyGLTF and cgltf may require modifications for specific glTF extension support and are located in the corresponding 3ds Max plugin project folder._

For more information regarding these licenses, please refer to the documentation provided within the source repository.

---

### 📄 License Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

```

```
