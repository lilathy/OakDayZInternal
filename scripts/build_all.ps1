


param(
    [string]$Configuration = "Release",
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"
if ($PSScriptRoot) {

    $ProjectRoot = (Split-Path $PSScriptRoot -Parent)
} else {
    $ProjectRoot = Get-Location
}
$BinDir = Join-Path $ProjectRoot "bin"

Write-Host "oak build" -ForegroundColor Cyan
Write-Host ""
Write-Host "checking stuff..." -ForegroundColor Yellow


$msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
if (-not $msbuild) {

    $vsPath = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (Test-Path $vsPath) {
        $msbuild = $vsPath
    } else {
        $vsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path $vsPath) {
            $msbuild = $vsPath
        }
    }
}

if (-not $msbuild -or -not (Test-Path $msbuild)) {
    Write-Host "msbuild not found, need vs2022" -ForegroundColor Red
    exit 1
}
Write-Host "  msbuild ok" -ForegroundColor Green


$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {

    $cmakePaths = @(
        "${env:ProgramFiles}\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe",
        "${env:LOCALAPPDATA}\Microsoft\WindowsApps\cmake.exe"
    )
    
    foreach ($path in $cmakePaths) {
        if (Test-Path $path) {
            $cmake = $path
            break
        }
    }
}
if ($cmake -and $cmake -isnot [string]) {
    $cmake = $cmake.Source
}
if (-not $cmake -or -not (Test-Path $cmake)) {
    Write-Host "cmake not found" -ForegroundColor Red
    Write-Host "  get it from cmake.org" -ForegroundColor Yellow
    exit 1
}
Write-Host "  cmake ok" -ForegroundColor Green


$dotnet = Get-Command dotnet -ErrorAction SilentlyContinue
if (-not $dotnet) {
    Write-Host "dotnet not found, need .net 8" -ForegroundColor Red
    exit 1
}
Write-Host "  dotnet ok" -ForegroundColor Green


$wdkPath = "${env:WindowsKitsRoot}DDK"
if (-not (Test-Path $wdkPath)) {
    Write-Host "wdk not found, driver might not build" -ForegroundColor Yellow
} else {
    Write-Host "  wdk ok" -ForegroundColor Green
}

Write-Host ""


if ($Clean) {
    Write-Host "cleaning..." -ForegroundColor Yellow
    if (Test-Path $BinDir) {
        Remove-Item -Path $BinDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path (Join-Path $ProjectRoot "build")) {
        Remove-Item -Path (Join-Path $ProjectRoot "build") -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path (Join-Path $ProjectRoot "injector\build")) {
        Remove-Item -Path (Join-Path $ProjectRoot "injector\build") -Recurse -Force -ErrorAction SilentlyContinue
    }
    Write-Host "  cleaned" -ForegroundColor Green
    Write-Host ""
}


if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Path $BinDir | Out-Null
}


Write-Host "building driver..." -ForegroundColor Yellow
$driverProj = Join-Path $ProjectRoot "driver\oak.vcxproj"
if (Test-Path $driverProj) {
    try {
        & $msbuild $driverProj `
            /p:Configuration=$Configuration `
            /p:Platform=x64 `
            /m `
            /nologo `
            /v:minimal
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  driver ok" -ForegroundColor Green
            

            $driverSys = Join-Path $ProjectRoot "driver\bin\oak.sys"
            if (Test-Path $driverSys) {
                Copy-Item $driverSys $BinDir -Force
                Write-Host "  driver copied" -ForegroundColor Green
            }
        } else {
            Write-Host "  driver failed" -ForegroundColor Red
            exit 1
        }
    } catch {
        Write-Host "  driver error: $_" -ForegroundColor Red
        Write-Host "  need wdk for driver" -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "  driver project not found, skip" -ForegroundColor Yellow
}
Write-Host ""


Write-Host "building injector..." -ForegroundColor Yellow
$injectorBuildDir = Join-Path $ProjectRoot "injector\build"
if (-not (Test-Path $injectorBuildDir)) {
    New-Item -ItemType Directory -Path $injectorBuildDir | Out-Null
}

Push-Location $injectorBuildDir
try {

    Write-Host "  cmake config..." -ForegroundColor Gray
    & $cmake .. -DCMAKE_BUILD_TYPE=$Configuration
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  cmake failed" -ForegroundColor Red
        Write-Host "  trying vs generator..." -ForegroundColor Yellow

        & $cmake .. -G "Visual Studio 17 2022" -A x64
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  cmake failed" -ForegroundColor Red
            exit 1
        }
    }
    

    Write-Host "  building..." -ForegroundColor Gray
    & $cmake --build . --config $Configuration
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  injector ok" -ForegroundColor Green
        

        $possiblePaths = @(
            "$Configuration\oak_loader.exe",
            "Release\oak_loader.exe",
            "Debug\oak_loader.exe",
            "oak_loader.exe"
        )
        
        foreach ($relPath in $possiblePaths) {
            $oakLoaderExe = Join-Path $injectorBuildDir $relPath
            if (Test-Path $oakLoaderExe) {
                Copy-Item $oakLoaderExe $BinDir -Force
                Write-Host "  oak_loader copied" -ForegroundColor Green
                break
            }
        }
        
        foreach ($relPath in $possiblePaths) {
            $injectorExe = Join-Path $injectorBuildDir ($relPath -replace "oak_loader", "injector")
            if (Test-Path $injectorExe) {
                Copy-Item $injectorExe $BinDir -Force
                Write-Host "  injector copied" -ForegroundColor Green
                break
            }
        }
    } else {
        Write-Host "  injector failed" -ForegroundColor Red
        exit 1
    }
} finally {
    Pop-Location
}
Write-Host ""


Write-Host "building dll..." -ForegroundColor Yellow
$buildDir = Join-Path $ProjectRoot "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Push-Location $buildDir
try {

    Write-Host "  cmake config..." -ForegroundColor Gray
    & $cmake .. -DCMAKE_BUILD_TYPE=$Configuration
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  trying vs generator..." -ForegroundColor Yellow

        & $cmake .. -G "Visual Studio 17 2022" -A x64
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  cmake failed" -ForegroundColor Red
            exit 1
        }
    }
    

    Write-Host "  building..." -ForegroundColor Gray
    & $cmake --build . --config $Configuration
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  dll ok" -ForegroundColor Green
        

        $possiblePaths = @(
            "$Configuration\dayz_internal.dll",
            "Release\dayz_internal.dll",
            "Debug\dayz_internal.dll",
            "dayz_internal.dll"
        )
        
        foreach ($relPath in $possiblePaths) {
            $dllPath = Join-Path $buildDir $relPath
            if (Test-Path $dllPath) {
                Copy-Item $dllPath $BinDir -Force
                Write-Host "  dll copied" -ForegroundColor Green
                break
            }
        }
    } else {
        Write-Host "  dll failed" -ForegroundColor Red
        exit 1
    }
} finally {
    Pop-Location
}
Write-Host ""


Write-Host "building panel..." -ForegroundColor Yellow
$panelDir = Join-Path $ProjectRoot "panel-csharp"
Push-Location $panelDir
try {
    & $dotnet build -c $Configuration
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  panel ok" -ForegroundColor Green
        

        $panelExe = Join-Path $panelDir "bin\$Configuration\net8.0-windows10.0.19041.0\OakPanel.exe"
        if (Test-Path $panelExe) {
            Copy-Item "$panelDir\bin\$Configuration\net8.0-windows10.0.19041.0\*" $BinDir -Recurse -Force
        }
    } else {
        Write-Host "  panel failed" -ForegroundColor Red
        exit 1
    }
} finally {
    Pop-Location
}
Write-Host ""


Write-Host "done" -ForegroundColor Green
Write-Host "output: $BinDir" -ForegroundColor Cyan
Get-ChildItem $BinDir -File | ForEach-Object {
    Write-Host "  $($_.Name)" -ForegroundColor White
}
Write-Host ""
