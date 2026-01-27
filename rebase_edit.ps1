# 自动修改rebase序列文件
$rebaseDir = ".git/rebase-merge"
if (Test-Path $rebaseDir) {
    $gitSequenceFile = Join-Path $rebaseDir "git-rebase-todo"
    if (Test-Path $gitSequenceFile) {
        $content = Get-Content $gitSequenceFile
        $newContent = $content -replace '^pick 9f25b6a', 'edit 9f25b6a'
        $newContent | Set-Content $gitSequenceFile
        Write-Host "已修改rebase序列文件"
    }
}
