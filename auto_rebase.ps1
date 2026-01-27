# 自动化rebase脚本
$rebaseDir = ".git/rebase-merge"
if (-not (Test-Path $rebaseDir)) {
    New-Item -ItemType Directory -Path $rebaseDir -Force | Out-Null
}

$gitSequenceFile = Join-Path $rebaseDir "git-rebase-todo"
$content = @"
edit 9f25b6a 增加WebView2（NuGet）环境，集成MebTile瓦片地图解包代码并置于主窗口底层
pick 42332d0 增加地图操作控制，并修复部分操作逻辑问题，mtb更换为高分辨率文件测试
"@
$content | Set-Content $gitSequenceFile -Encoding UTF8
Write-Host "Rebase sequence file created"
