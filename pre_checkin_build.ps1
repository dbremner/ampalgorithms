# Configure VS environment

function script:append-path {
	$oldPath = get-content Env:\Path;
	$newPath = $oldPath + ";" + $args;
	set-content Env:\Path $newPath;
}

if (-not (test-path env:VSINSTALLDIR)) {
    echo "Setting up VS environment..."
    $script="${env:ProgramFiles(x86)}\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
    $tempFile = [IO.Path]::GetTempFileName()  
 
    echo "Creating setup script: $tempFile"

    ## Store the output of cmd.exe.  We also ask cmd.exe to output   
    ## the environment table after the batch file completes  
 
    cmd /c " `"$script`" $parameters && set > `"$tempFile`" " 
     
    ## Go through the environment variables in the temp file.  
    ## For each of them, set the variable in our local environment.  

    Get-Content $tempFile | Foreach-Object {   
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
$StopWatch = New-Object system.Diagnostics.Stopwatch
$StopWatch.Start()

write-host "== Clean         ===============================================================" -fore yellow

$paths = @( "$msbuild_dir\Win32", "$msbuild_dir\x64", "$msbuild_dir\Intermediate", "$msbuild_dir\TestResults" )
 
foreach ($p in $paths) {
    if (test-path $p) { 
        Remove-Item -Recurse -Force $p
        echo $p
    }
}
mkdir $msbuild_dir\Intermediate > $null

write-host "== x64/Debug     ===============================================================" -fore yellow
$log="$msbuild_int\x64_Debug.log"
Invoke-Expression "$msbuild $msbuild_sln /p:platform=x64 /p:configuration=Debug     $msbuild_options /fileloggerparameters:logfile='$log'"
write-host "== Win32/Debug   ===============================================================" -fore yellow
$log="$msbuild_int\Win32_Debug.log"
Invoke-Expression "$msbuild $msbuild_sln /p:platform=Win32 /p:configuration=Debug   $msbuild_options /fileloggerparameters:logfile='$log'"
write-host "== x64/Release   ===============================================================" -fore yellow
$log="$msbuild_int\x64_Release.log"
Invoke-Expression "$msbuild $msbuild_sln /p:platform=x64 /p:configuration=Release   $msbuild_options /fileloggerparameters:logfile='$log'"
write-host "== Win32/Release ===============================================================" -fore yellow
$log="$msbuild_int\Win32_Release.log"
Invoke-Expression "$msbuild $msbuild_sln /p:platform=Win32 /p:configuration=Release $msbuild_options /fileloggerparameters:logfile='$log'"

$StopWatch.Stop();
write-host "== Run Tests     ===============================================================" -fore yellow

$elapsed = $StopWatch.Elapsed  
$BuildElapsedTime = [system.String]::Format("{0:00}:{1:00}.{2:00}", $elapsed.Minutes, $elapsed.Seconds, $elapsed.Milliseconds / 10);

## Run tests

$StopWatch.Reset();
$StopWatch.Start()

echo "Showing output from failed tests only:"
."$env:VSINSTALLDIR\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" "$msbuild_dir\Win32\Release\amp_algorithms.dll" "$msbuild_dir\Win32\Release\amp_stl_algorithms.dll" 2>&1 |
	where { $_ -match @(' *Failed   .*') } | write-host -fore red

$StopWatch.Stop();
echo "Tests complete"
write-host "================================================================================" -fore yellow

$elapsed = $StopWatch.Elapsed  
$TestsElapsedTime = [system.String]::Format("{0:00}:{1:00}.{2:00}", $elapsed.Minutes, $elapsed.Seconds, $elapsed.Milliseconds / 10);

write-host ""
write-host "Build completed in: $BuildElapsedTime" -fore yellow
write-host "Tests completed in: $TestsElapsedTime" -fore yellow
