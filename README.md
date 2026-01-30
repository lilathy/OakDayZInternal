# Oak

DayZ internal - driver, injector, game DLL, and a small external ClickGUI panel to configure modules. This is a quick and buggy version;

## Building

From the repo root, run:

```powershell
.\scripts\build_all.ps1
```

## Screenshots

![ESP & Skeleton](1.png)

![ClickGUI](2.png)


You’ll need Visual Studio (with C++ and WDK for the driver), CMake, and .NET 8. The script builds the driver, injector, oak_loader, the game DLL, and the panel, then drops everything into `oak\bin`.

## Running

Copy the contents of `bin` to `C:\oak`. The driver expects the DLL at `C:\oak\dayz_internal.dll`.

**Before launching DayZ**, open PowerShell as Administrator, then run:

```powershell
cd C:\oak
.\oak_loader.exe
```

That loads the driver. Then start DayZ. To open the ClickGUI, run **OakPanel.exe** from `C:\oak`.

**Driver warning:** If you use the default Intel driver (the one that ships with kdmapper/oak_loader), you will get banned within a day. Use a private/custom driver if you care about not getting banned.

## Layout

- **driver** — kernel driver (mapper, etc.)
- **injector** — injector + oak_loader (driver loader)
- **src** — game DLL (DayZ internal)
- **panel-csharp** — WPF control panel
- **scripts** — `build_all.ps1`, etc.

Credits: 
- **[kdmapper](https://github.com/TheCruZ/kdmapper)**
Used in the loader to map unsigned kernel drivers into memory via the vulnerable intel driver.

(Prebuilt custom driver, .dll, loader, injector, panel, panel .dll, panel dependencies, .net runtime config, windows sdk, and WinRT runtime are all in release.) Make sure to run powershell as admin or Windows will refuse to run the driver.
