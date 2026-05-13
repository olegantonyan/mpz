param([string]$Target = ".")

# Upload debug symbols to Sentry via sentry-cli (Windows VM release helper).
# Silently skips when SENTRY_AUTH_TOKEN is unset.

if (-not $env:SENTRY_AUTH_TOKEN) {
  Write-Host "upload-symbols: SENTRY_AUTH_TOKEN unset, skipping"
  exit 0
}
if (-not $env:SENTRY_ORG)     { throw "SENTRY_ORG must be set" }
if (-not $env:SENTRY_PROJECT) { throw "SENTRY_PROJECT must be set" }

if (-not (Get-Command sentry-cli -ErrorAction SilentlyContinue)) {
  $cli = Join-Path $env:TEMP "sentry-cli.exe"
  Invoke-WebRequest -UseBasicParsing `
    -Uri "https://release-registry.services.sentry.io/apps/sentry-cli/latest?response=download&arch=x86_64&platform=Windows&package=sentry-cli" `
    -OutFile $cli
  $env:PATH = "$($env:TEMP);$env:PATH"
}

try {
  sentry-cli debug-files upload --include-sources --wait $Target
} catch {
  Write-Host "upload-symbols: upload failed (non-fatal): $_"
}
