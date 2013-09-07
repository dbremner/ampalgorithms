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

# Configure VS environment

function script:append-path 
{
    $oldPath = get-content Env:\Path;
    $newPath = $oldPath + ";" + $args;
    set-content Env:\Path $newPath;
}

if (-not (test-path env:VSINSTALLDIR)) 
{
    write-host "Setting up VS environment..."
    $script="${env:ProgramFiles(x86)}\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
    $tempFile = [IO.Path]::GetTempFileName()  
 
    write-host "Creating setup script: $tempFile."

    ## Store the output of cmd.exe.  We also ask cmd.exe to output   
    ## the environment table after the batch file completes  
 
    cmd /c " `"$script`" $parameters && set > `"$tempFile`" " 
     
    ## Go through the environment variables in the temp file.  
    ## For each of them, set the variable in our local environment.  

    Get-Content $tempFile | Foreach-Object 
    {   
        if ($_ -match "^(.*?)=(.*)$")  
        { 
            set-content "env:\$($matches[1])" $matches[2]  
        } 
    }  
    Remove-Item $tempFile
}

## Build targets and options

$vstest = "$env:VSINSTALLDIR\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
$msbuild = "$env:FrameworkDir$env:FrameworkVersion\msbuild.exe"

$msbuild_options = "/p:WARNINGS_AS_ERRORS=true /verbosity:m /nologo /filelogger /target:rebuild /consoleloggerparameters:verbosity=m"
$StopWatch = New-Object System.Diagnostics.Stopwatch

$msbuild_dir = split-path -parent $MyInvocation.MyCommand.Definition
$msbuild_sln = "amp_algorithms.sln"
$msbuild_int = "$msbuild_dir\Intermediate"
$vstest_dlls = @( "$msbuild_dir\Win32\Release\amp_algorithms.dll", "$msbuild_dir\Win32\Release\amp_stl_algorithms.dll" )

## Process arguments

write-output ""

if ($args -contains "/ref")
{
    $msbuild_options = $msbuild_options + " /p:USE_REF=USE_REF"
    write-host "Building with USE_REF defined, unit tests will use REF accelerator." -fore yellow
}

$configs = @( "Debug", "Release" )
$platforms = @( "x64", "Win32" )
$output_paths = @( "$msbuild_dir\Win32", "$msbuild_dir\x64", "$msbuild_dir\Intermediate", "$msbuild_dir\TestResults" )

if ($args -contains "/test")
{
    write-host "Building only Win32/Release." -fore yellow
    $configs = @( "Release" )
    $platforms = @( "Win32" )
    $output_paths = @( "$msbuild_dir\Win32", "$msbuild_dir\Intermediate", "$msbuild_dir\TestResults" )
}

## Run build

$StopWatch.Start()

## Clean tree...

write-host "== Clean         ===============================================================" -fore yellow

foreach ($p in $output_paths) 
{
    if (test-path $p) 
    { 
        Remove-Item -Recurse -Force $p
        write-host "  $p"
    }
}
mkdir $msbuild_dir\Intermediate > $null

## Build all targets...

$builds_ok = 0
$builds_run = 0
foreach ($c in $configs) 
{
    foreach ($p in $platforms) 
    {
    	$build = "$p/$c".PadRight(14)
        write-host "`n== Build $build ========================================================" -fore yellow
        $log="$msbuild_int\$p" + "_$c.log"
        Invoke-Expression "$msbuild $msbuild_sln /p:platform=$p /p:configuration=$c $msbuild_options /fileloggerparameters:logfile='$log'" |
            foreach-object { if ( $_ -match "BUILD SUCCEEDED" ) { $builds_ok++ } write-host $_ } | write-host 
        $builds_run += 2
    }
}

$StopWatch.Stop();
$elapsed = $StopWatch.Elapsed  
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

    $StopWatch.Reset();
    $StopWatch.Start();

    if ($args -contains "/ref")
    {
        write-host "Running tests with REF accelerator, this may take several minutes..." -fore yellow
    }
    $tests_failed = 0
    write-host "Showing output from failed tests only:" -fore yellow
    ."$vstest" $vstest_dlls /logger:trx 2>&1 | 
        %{ if ( $_ -match @('^Failed +')) { $tests_failed++ } ; $_ } | 
        where { $_ -match @('^(Failed +| +Assert failed\.)') } | 
        %{ write-host "  $_" -fore red }

    $StopWatch.Stop();
    if ($tests_failed -gt 0) 
    {
        write-host "`n$tests_failed Tests FAILED!" -fore red
    }
    else
    {
        write-host "`nTests complete, no failures." -fore green
    }

    $elapsed = $StopWatch.Elapsed  
    $TestsElapsedTime = [System.String]::Format("{0:00}m {1:00}s", $elapsed.Minutes, $elapsed.Seconds);
}

write-host "`n================================================================================" -fore yellow
write-host "Build completed in: $BuildElapsedTime." -fore yellow
if ( $TestsElapsedTime -ne "" )
{
    write-host "Tests completed in: $TestsElapsedTime." -fore yellow
}
write-host
