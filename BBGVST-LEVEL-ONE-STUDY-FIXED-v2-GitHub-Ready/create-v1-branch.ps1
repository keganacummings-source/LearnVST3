$ErrorActionPreference = 'Stop'
Write-Host 'BBGVST V1 branch helper' -ForegroundColor Cyan
if (-not (Test-Path '.git')) { throw 'Run this from the root of the BBGVST Git repository.' }
git fetch origin
git switch -C v1-identity-engine
git add .
git commit -m 'BBGVST V1 identity engine: distinct FX/instrument personalities' --allow-empty
git push -u origin v1-identity-engine
Write-Host 'V1 branch pushed: v1-identity-engine' -ForegroundColor Green
