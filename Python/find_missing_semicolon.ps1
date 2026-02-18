# ============================================================================
# find_missing_semicolon.ps1 - Diagnostic Script
# ============================================================================
# Run this in PowerShell from Z:\EmulatRAppUni\
# ============================================================================

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Searching for missing semicolon issue..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Find all files that include cpuCore_core.h
$searchPattern = '#include.*cpuCore_core\.h'
$files = Get-ChildItem -Path . -Recurse -Include *.h,*.cpp | 
         Select-String -Pattern $searchPattern | 
         Select-Object -ExpandProperty Path -Unique

Write-Host "Files that include cpuCore_core.h:" -ForegroundColor Yellow
foreach ($file in $files) {
    Write-Host "  $file" -ForegroundColor White
}
Write-Host ""

if ($files.Count -eq 0) {
    Write-Host "No files directly include cpuCore_core.h" -ForegroundColor Red
    Write-Host "Searching for files that might include it indirectly..." -ForegroundColor Yellow
    
    # Search for files that might include AlphaPipeline.h or similar
    $indirectFiles = Get-ChildItem -Path . -Recurse -Include *.h,*.cpp |
                     Select-String -Pattern '#include.*AlphaPipeline\.h' |
                     Select-Object -ExpandProperty Path -Unique
    
    Write-Host "Files that include AlphaPipeline.h (which might include cpuCore_core.h):" -ForegroundColor Yellow
    foreach ($file in $indirectFiles) {
        Write-Host "  $file" -ForegroundColor White
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Checking for common syntax errors..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Check each file for missing semicolons before includes
foreach ($file in $files) {
    Write-Host ""
    Write-Host "Checking: $file" -ForegroundColor Yellow
    
    $content = Get-Content $file -Raw
    $lines = Get-Content $file
    
    # Find the line number where cpuCore_core.h is included
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match 'cpuCore_core\.h') {
            $includeLine = $i + 1  # Convert to 1-based line number
            
            Write-Host "  Found include at line $includeLine" -ForegroundColor Green
            
            # Check previous 10 lines for missing semicolons
            $startLine = [Math]::Max(0, $i - 10)
            Write-Host "  Context (10 lines before):" -ForegroundColor Cyan
            
            for ($j = $startLine; $j -le $i; $j++) {
                $lineNum = $j + 1
                $line = $lines[$j].TrimEnd()
                
                # Highlight suspicious lines
                if ($line -match '^\s*class\s+\w+' -or 
                    $line -match '^\s*struct\s+\w+' -or
                    $line -match '^\s*\}\s*$' -and $line -notmatch ';') {
                    Write-Host "  $lineNum : $line" -ForegroundColor Red
                    Write-Host "         ^^^ SUSPICIOUS - might need semicolon!" -ForegroundColor Red
                } else {
                    Write-Host "  $lineNum : $line" -ForegroundColor Gray
                }
            }
            
            break
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Checking AlphaPipeline.h specifically..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$pipelineFile = Get-ChildItem -Path . -Recurse -Filter "AlphaPipeline.h" | Select-Object -First 1

if ($pipelineFile) {
    Write-Host "Found: $($pipelineFile.FullName)" -ForegroundColor Green
    
    $lines = Get-Content $pipelineFile.FullName
    
    # Check if it includes cpuCore_core.h
    $includesCpuCore = $false
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match 'cpuCore_core\.h') {
            $includesCpuCore = $true
            $includeLine = $i + 1
            
            Write-Host ""
            Write-Host "AlphaPipeline.h includes cpuCore_core.h at line $includeLine!" -ForegroundColor Yellow
            
            # Show context
            $startLine = [Math]::Max(0, $i - 15)
            Write-Host "Context:" -ForegroundColor Cyan
            
            for ($j = $startLine; $j -le $i; $j++) {
                $lineNum = $j + 1
                $line = $lines[$j].TrimEnd()
                
                if ($line -match '^\s*\}\s*$' -and $line -notmatch ';') {
                    Write-Host "  $lineNum : $line" -ForegroundColor Red
                    Write-Host "         ^^^ MISSING SEMICOLON HERE!" -ForegroundColor Red
                } elseif ($line -match 'cpuCore_core') {
                    Write-Host "  $lineNum : $line" -ForegroundColor Yellow
                } else {
                    Write-Host "  $lineNum : $line" -ForegroundColor Gray
                }
            }
            
            break
        }
    }
    
    if (-not $includesCpuCore) {
        Write-Host "AlphaPipeline.h does NOT include cpuCore_core.h directly" -ForegroundColor Green
    }
} else {
    Write-Host "AlphaPipeline.h not found!" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "RECOMMENDATIONS:" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "1. Look for lines marked 'SUSPICIOUS' or 'MISSING SEMICOLON' above" -ForegroundColor Yellow
Write-Host "2. If you see '}' without ';' before an #include, that's your bug!" -ForegroundColor Yellow
Write-Host "3. If no issues found, try a clean rebuild (see REBUILD_INSTRUCTIONS.txt)" -ForegroundColor Yellow
Write-Host ""
