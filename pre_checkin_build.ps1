#----------------------------------------------------------------------------
# Copyright (c) Microsoft Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not 
# use this file except in compliance with the License.  You may obtain a copy 
# of the License at http://www.apache.org/licenses/LICENSE-2.0  
# 
# THIS CODE IS PROVIDED #AS IS# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
# WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
# MERCHANTABLITY OR NON-INFRINGEMENT. 
#
# See the Apache Version 2.0 License for specific language governing 
# permissions and limitations under the License.
#---------------------------------------------------------------------------
# 
# C++ AMP standard algorithm library.
#
# This file contains the C++ AMP standard algorithms
#---------------------------------------------------------------------------

## Configure VS environment

function set-vsenv
{
    write-host "Setting up VS 2013 environment..."

    $script = "${env:ProgramFiles(x86)}\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
    $tempFile = [IO.Path]::GetTempFileName()
 
    write-host "Creating setup script: $tempFile."
    write-host " Running $script..."
 
    cmd /c " `"$script`" && set > `"$tempFile`" " 

    Get-Content $tempFile | Foreach-Object {   
        if($_ -match "^(.*?)=(.*)$")  
        { 
            Set-Content "env:\$($matches[1])" $matches[2]  
        } 
    }
    Remove-Item $tempFile
}

## Tools paths and options

write-output ""

if (-not (test-path "${env:ProgramFiles(x86)}\Microsoft Visual Studio 12.0\VC"))
{
    write-host "Visual Studio 2013 not found at ${env:ProgramFiles(x86)}\Microsoft Visual Studio 12.0\VC" -fore red
    exit
}
if (-not (test-path "${env:ProgramFiles(x86)}\Microsoft Visual Studio 11.0\VC"))
{
    write-host "Visual Studio 2012 not found at ${env:ProgramFiles(x86)}\Microsoft Visual Studio 11.0\VC" -fore red
    exit
}
if (-not (test-path "$env:VSINSTALLDIR"))
{
    set-vsenv
}

$stopwatch = New-Object System.Diagnostics.Stopwatch
$vstest_exe = "$env:VSINSTALLDIR\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
$msbuild_exe = "$env:FrameworkDir$env:FrameworkVersion\msbuild.exe"
$msbuild_options = "/p:WARNINGS_AS_ERRORS=true /verbosity:m /nologo /filelogger /target:build /consoleloggerparameters:verbosity=m"

## Process arguments and configure builds

if ($args -contains "/ref")
{
    $msbuild_options = $msbuild_options + " /p:USE_REF=USE_REF"
    write-host "Building with USE_REF defined, unit tests will use REF accelerator." -fore yellow
}

$build_dir = split-path -parent $MyInvocation.MyCommand.Definition
$test_dir =
$build_int = "$build_dir/Intermediate"

$vsvers = @( "11", "12")
$configs = @( "Debug", "Release" )
$platforms = @( "x64", "Win32" )

if ($args -contains "/test")
{
    $builds = @( (New-Object 'Tuple[string, string, string]'("11", "Release", "Win32")), (New-Object 'Tuple[string, string, string]'("12", "Release", "Win32")))
    write-host "Test build: Building only Win32/Release." -fore yellow
}
else
{
    $builds = @()
    foreach ($ver in $vsvers)
    {
        foreach ($conf in $configs) 
        {
            foreach ($plat in $platforms)
            {
                $builds += New-Object 'Tuple[string, string, string]'($ver, $conf, $plat)
            }
        }
    }
}

## Run build

$stopwatch.Start()

## Clean tree...

write-host "== Clean         ===============================================================" -fore yellow

foreach ($b in $builds) 
{
    $ver = $b.Item1
    $conf = $b.Item2
    $plat = $b.Item3
    $path_bin = "$build_dir/$plat/$conf/v${ver}0"
    $path_int = "$build_int/$plat/$conf/v${ver}0"
    foreach ($p in @( "$build_dir/$plat", "$build_int/$plat" ))
    {
        if (test-path $p) 
        { 
            Remove-Item -Recurse -Force $p
            write-host "  Cleaned:  $p"
        }
    }
}

if ($args -contains "/clean") { exit }

## Build all targets...

write-host "`n== Build         ===============================================================" -fore yellow
write-host "Running the following builds:" -fore yellow
foreach ($b in $builds)
{
    $ver = $b.Item1
    $conf = $b.Item2
    $plat = $b.Item3
    $sln = "amp_algorithms${ver}0.sln"

    write-host "  $sln ( $conf | $plat )"
}
$builds_ok = 0
$builds_run = 0
foreach ($b in $builds)
{
    $ver = $b.Item1
    $conf = $b.Item2
    $plat = $b.Item3
    $path_bin = "$build_dir/$plat/$conf/v${ver}0"
    $path_int = "$build_int/$plat/$conf/v${ver}0"
    $sln = "amp_algorithms${ver}0.sln"
    $log = "$build_int/$plat/$conf/v${ver}0/build.log"

    foreach ($p in @( $path_bin, $path_int ))
    {
        if (-not (test-path $p)) { mkdir $p >> $null }
    }

    $build = "$sln $plat/$conf".PadRight(36)
    write-host "`n== Build $build ==================================" -fore yellow
    $build_cmd = "$msbuild_exe $sln /p:platformtoolset=v${ver}0 /p:VisualStudioVersion=${ver}.0 /p:platform=$plat /p:configuration=$conf /p:intdir='$path_int/' /p:outdir='$path_bin/' $msbuild_options /fileloggerparameters:logfile='$log'"
    Invoke-Expression $build_cmd |
        foreach-object { if ( $_ -match "BUILD SUCCEEDED" ) { $builds_ok++ } write-host $_ } | write-host 
    $builds_run += 2
}

$stopwatch.Stop();
$elapsed = $stopwatch.Elapsed  
$BuildElapsedTime = [system.String]::Format("{0:00}m {1:00}s", $elapsed.Minutes, $elapsed.Seconds);

$builds_failed = ($builds_run - $builds_ok)
if ( $builds_failed -gt 0 ) 
{
    write-host "`n$builds_failed/$builds_run Builds FAILED!"-fore red
    $TestsElapsedTime = ""
}
else
{
    write-host "`n$builds_ok/$builds_run Builds completed." -fore green

    ## Run tests...

    write-host "`n== Run Tests     ===============================================================" -fore yellow

    $stopwatch.Reset();
    $stopwatch.Start();

    if ($args -contains "/ref")
    {
        write-host "Running tests with REF accelerator, this may take several minutes..." -fore yellow
    }

    foreach ($ver in $vsvers)
    {
        $vstest_dlls = @( "$build_dir/Win32/Release/v${ver}0/amp_algorithms.dll", "$build_dir/Win32/Release/v${ver}0/amp_stl_algorithms.dll" )
       
        $tests_failed = 0
        $tests_passed = 0
        write-host "Running tests for Visual Studio ${ver}.0 ( Win32 | Release) build. Showing failed tests:" -fore yellow
        ."$vstest_exe" $vstest_dlls /logger:trx 2>&1 | 
            %{ if ( $_ -match @('^Passed +')) { $tests_passed++ } ; $_ } | 
            %{ if ( $_ -match @('^Failed +')) { $tests_failed++ } ; $_ } | 
            where { $_ -match @('^(Failed +| +Assert failed\.)') } | 
            %{ write-host "  $_" -fore red }

        $tests_count = $tests_passed + $tests_failed
        if ($tests_failed -gt 0) 
        {
            write-host "`n$tests_failed / $tests_count Tests FAILED!`n" -fore red
        }
        else
        {
            write-host "`nAll $tests_count tests complete, no failures.`n" -fore green
        }
    }
    $stopwatch.Stop();
    $elapsed = $stopwatch.Elapsed  
    $TestsElapsedTime = [System.String]::Format("{0:00}m {1:00}s", $elapsed.Minutes, $elapsed.Seconds);
}

write-host "`n================================================================================" -fore yellow
write-host "Build completed in: $BuildElapsedTime." -fore yellow
if ( $TestsElapsedTime -ne "" )
{
    write-host "Tests completed in: $TestsElapsedTime." -fore yellow
}
write-host
