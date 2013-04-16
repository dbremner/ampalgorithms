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
    echo "Setting up VS environment..."
    $script="${env:ProgramFiles(x86)}\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
    $tempFile = [IO.Path]::GetTempFileName()  
 
    echo "Creating setup script: $tempFile"

    ## Store the output of cmd.exe.  We also ask cmd.exe to output   
    ## the environment table after the batch file completes  
 
    cmd /c " `"$script`" $parameters && set > `"$tempFile`" " 
     
    ## Go through the environment variables in the temp file.  
    ## For each of them, set the variable in our local environment.  

    Get-Content $tempFile | Foreach-Object 
    {   
        if ($_ -match "^(.*?)=(.*)$")  
        { 
            Set-Content "env:\$($matches[1])" $matches[2]  
        } 
    }  
    Remove-Item $tempFile
}

## Run build

$msbuild="$env:FrameworkDir$env:FrameworkVersion\msbuild.exe"
# TODO: Make warnings get treated like errors in these builds.
$msbuild_options="/verbosity:m /nologo /filelogger /target:rebuild /consoleloggerparameters:verbosity=m"
$msbuild_sln="amp_algorithms.sln"
$msbuild_dir = split-path -parent $MyInvocation.MyCommand.Definition
$msbuild_int ="$msbuild_dir\Intermediate"
$StopWatch = New-Object System.Diagnostics.Stopwatch
$StopWatch.Start()

## Clean tree...

write-host "== Clean         ===============================================================" -fore yellow

$paths = @( "$msbuild_dir\Win32", "$msbuild_dir\x64", "$msbuild_dir\Intermediate", "$msbuild_dir\TestResults" )
 
foreach ($p in $paths) 
{
    if (test-path $p) 
    { 
        Remove-Item -Recurse -Force $p
        echo $p
    }
}
mkdir $msbuild_dir\Intermediate > $null

## Build all targets...

if ($args -contains "/ref")
{
    $msbuild_options=$msbuild_options + " /p:USE_REF=USE_REF"
    write-host "Building with USE_REF defined, unit tests will use REF accelerator." -fore yellow
}

$configs = @( "Debug", "Release" )
$platforms = @( "x64", "Win32" )

$builds_ok = 0
$builds_run = 0
foreach ($c in $configs) 
{
    foreach ($p in $platforms) 
    {
        write-host "== Build $p/$c     =========================================================" -fore yellow
        $log="$msbuild_int\$p" + "_$c.log"
        Invoke-Expression "$msbuild $msbuild_sln /p:platform=$p /p:configuration=$c $msbuild_options /fileloggerparameters:logfile='$log'" |
            foreach-object { if ( $_ -match "BUILD SUCCEEDED" ) { $builds_ok++ } echo $_ } | write-host 
        $builds_run += 2
    }
}

$StopWatch.Stop();
$elapsed = $StopWatch.Elapsed  
$BuildElapsedTime = [system.String]::Format("{0:00}:{1:00}.{2:00}", $elapsed.Minutes, $elapsed.Seconds, $elapsed.Milliseconds / 10);

$builds_failed = ($builds_run - $builds_ok)
if ( $builds_failed -gt 0 ) 
{
    write-host "$builds_failed/$builds_run Builds FAILED!"-fore red
    $TestsElapsedTime = ""
}
else
{
    write-host "$builds_ok/$builds_run Builds completed." -fore green

    write-host "== Run Tests     ===============================================================" -fore yellow

    ## Run tests...

    $StopWatch.Reset();
    $StopWatch.Start();

    if ($arg1 -eq "/ref")
    {
        write-host "Running tests with REF accelerator, this may take several minutes." -fore yellow
    }
    $tests_failed = 0
    write-host "Showing output from failed tests only:" -fore yellow
    ."$env:VSINSTALLDIR\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" "$msbuild_dir\Win32\Release\amp_algorithms.dll" "$msbuild_dir\Win32\Release\amp_stl_algorithms.dll" /logger:trx 2>&1 |
        where { $_ -match @(' *Failed   .*') } | foreach-object { $tests_failed++; echo $_ } | write-host -fore red 

    $StopWatch.Stop();
    if ($tests_failed -gt 0) 
    {
        write-host "$tests_failed Tests FAILED!"-fore red
    }
    else
    {
        write-host "Tests complete." -fore green
    }

    $elapsed = $StopWatch.Elapsed  
    $TestsElapsedTime = [System.String]::Format("{0:00}:{1:00}.{2:00}", $elapsed.Minutes, $elapsed.Seconds, $elapsed.Milliseconds / 10);
}

write-host "================================================================================" -fore yellow
write-host ""
write-host "Build completed in: $BuildElapsedTime" -fore yellow
if ( $TestsElapsedTime -ne "" )
{
    write-host "Tests completed in: $TestsElapsedTime" -fore yellow
}
