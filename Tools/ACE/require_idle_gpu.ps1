# Do not overlap bounded ACE tests with another editor or game rendering workload.
# This detects Unreal processes, not every possible GPU consumer on the workstation.
$busy = @(Get-CimInstance Win32_Process -ErrorAction Stop | Where-Object {
    ($_.Name -in @('UnrealEditor.exe','UnrealEditor-Cmd.exe','DirectorsCut.exe','Atlantean Odyssey.exe') -or
        $_.Name -like 'DirectorsCut-Win64-*.exe') -and
    $_.CommandLine -notmatch '(?i)(?:^|\s)-NullRHI(?:\s|$)'
})
if ($busy.Count) {
    throw ('Another Unreal GPU job is running (PID ' + (($busy | ForEach-Object ProcessId) -join ', ') +
        '). Finish that job before starting an ACE test; this helper will not stop it.')
}
