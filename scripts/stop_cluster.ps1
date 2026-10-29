$servers = Get-Process -Name shardcache_server -ErrorAction SilentlyContinue
if ($null -eq $servers) {
    Write-Host "No ShardCache servers running."
} else {
    $servers | Stop-Process -Confirm:$false
    Write-Host "Stopped $(@($servers).Count) server(s)."
}
