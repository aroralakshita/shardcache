param(
    [int]$Nodes = 3,
    [int]$BasePort = 7001,
    [int]$Capacity = 10000
)

$exe = Join-Path $PSScriptRoot "..\build\Debug\shardcache_server.exe"
if (-not (Test-Path $exe)) {
    Write-Error "Server not built yet: $exe not found. Run cmake --build build --config Debug first."
    exit 1
}

$addresses = @()
for ($i = 0; $i -lt $Nodes; $i++) {
    $port = $BasePort + $i
    Start-Process -FilePath $exe -ArgumentList $port, $Capacity
    $addresses += "127.0.0.1:$port"
}

Write-Host "Started $Nodes node(s). Connect with:"
Write-Host "  .\build\Debug\shardcache_cli.exe $($addresses -join ' ')"
