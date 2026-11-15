param([Parameter(Mandatory=$true)][string]$GameRoot)
$ErrorActionPreference = 'Stop'
$game = (Resolve-Path -LiteralPath $GameRoot).Path
# Only generated packaged games: never delete the vendor model from a source checkout.
if ((Test-Path -LiteralPath (Join-Path $game 'DirectorsCut.uproject')) -or
    (!(Test-Path -LiteralPath (Join-Path $game 'Binaries/Win64/DirectorsCut.exe')) -and
     !(Test-Path -LiteralPath (Join-Path $game 'Binaries/Win64/Atlantean Odyssey.exe'))) -or
    !(Test-Path -LiteralPath (Join-Path $game 'Content/Paks'))) {
    throw 'Expected a packaged DirectorsCut game directory.'
}
$relative = 'Plugins/NV_ACE_Reference/ThirdParty/Nvigi/Models/nvaim.plugin.gpt.ggml/{8E31808B-C182-4016-9ED8-64804FF5B40D}/nemotron-4-minitron-1-4b-instruct-dpo2_q4_0.gguf'
$unused = [IO.Path]::GetFullPath((Join-Path $game $relative))
if (!$unused.StartsWith($game + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Unused-model path escaped the selected packaged game.'
}
if (Test-Path -LiteralPath $unused -PathType Leaf) {
    $bytes = (Get-Item -LiteralPath $unused).Length
    Remove-Item -LiteralPath $unused -Force
    Write-Output "Removed unused legacy GPT model from generated package ($bytes bytes)."
}
