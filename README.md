# ForzaTech CLI
Toolkit to translate ForzaTech 3D assets (.zip, .modelbin) without any additional software.

> [!IMPORTANT]
> Full game installation, decrypted database and untouched assets are required to take full advantage of the features of this tool.

## Features
* Directly .zip reading support
* Textures extraction/conversion
* Material extraction/conversion
* Manufacturer colors extraction/conversion
* Full animation support when available
* Full upgrade support
* Fast data translation
* Quad and tri mesh support

### Supported games
* Forza Motorsport 5
* Forza Motorsport 6 / Apex
* Forza Motorsport 7
* Forza Motorsport 2023
* Forza Horizon 2
* Forza Horizon 3
* Forza Horizon 4
* Forza Horizon 5
* Forza Horizon 6

## Project setup and build

This code is designed to build with Visual Studio 2022, Visual Studio 2026 or later. Use of the Windows 11 April 2025 Update SDK (22621) or later is required for Visual Studio.

Necessary workloads
* Desktop development with C++
* Game development with C++

## Requirements
1. [Git](https://git-scm.com/)
2. [Visual Studio 2022](https://aka.ms/vs/17/release/vs_enterprise.exe)
3. [Autodesk FBX SDK 2020.3.9 VS2022](https://aps.autodesk.com/developer/overview/fbx-sdk)
4. [Boost 1.91.0](https://archives.boost.io/release/1.91.0/binaries/)

## Clone
```
git clone https://github.com/fmnext/cli.git --recursive
```

## Build
* Install requirements.
* Open main solution `cli.sln`.
* On the toolbar, choose Release.
* Right sidebar, RMB the solution > Build Solution.

## Credits
* All research archived on this project is the solely result of [Doliman100's](https://github.com/Doliman100) work.


*********


### DISCLAIMER

The inclusion of Forza's logo, products, or services in any materials or content does not imply endorsement or affiliation with Microsoft Corporation, unless specifically stated.

### Notices
All content and source code for this project are subject to the terms of the [GPL License](https://).

### Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.

© 2026, Microsoft Corporation. Forza and Forza Logo are trademarks or registered trademarks of Microsoft Corporation.

© 2026, Autodesk, Inc. FBX is a trademark or registered trademark of Autodesk, Inc. in the USA and elsewhere.