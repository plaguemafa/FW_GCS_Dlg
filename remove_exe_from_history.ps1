# 从Git历史中移除exe文件的PowerShell脚本
# 使用方法：关闭Visual Studio后，在PowerShell中运行此脚本

Write-Host "开始从Git历史中移除exe文件..." -ForegroundColor Green

# 进入项目目录
Set-Location "c:\WorkSpace\VSworkSpace\FW_GCS_Dlg"

# 检查Git状态
Write-Host "`n检查Git状态..." -ForegroundColor Yellow
git status

# 检查是否有未提交的更改
$status = git status --porcelain
if ($status) {
    Write-Host "`n警告：检测到未提交的更改。建议先提交或暂存这些更改。" -ForegroundColor Red
    $response = Read-Host "是否继续？(y/n)"
    if ($response -ne "y") {
        Write-Host "操作已取消。" -ForegroundColor Yellow
        exit
    }
}

# 显示当前提交历史
Write-Host "`n当前提交历史：" -ForegroundColor Yellow
git log --oneline -5

# 确认操作
Write-Host "`n此操作将从提交 9f25b6a 中移除 maptiler-engine-14.1.2-app-win-x64-setup.exe 文件" -ForegroundColor Yellow
Write-Host "警告：这将重写Git历史，如果已推送到远程，需要使用 --force 推送" -ForegroundColor Red
$confirm = Read-Host "`n确认继续？(yes/no)"
if ($confirm -ne "yes") {
    Write-Host "操作已取消。" -ForegroundColor Yellow
    exit
}

# 创建备份分支
Write-Host "`n创建备份分支..." -ForegroundColor Yellow
git branch backup-before-exe-removal
Write-Host "备份分支已创建：backup-before-exe-removal" -ForegroundColor Green

# 方法：使用git filter-branch（如果可用）
Write-Host "`n尝试使用git filter-branch移除文件..." -ForegroundColor Yellow
try {
    git filter-branch --force --index-filter "git rm --cached --ignore-unmatch maptiler-engine-14.1.2-app-win-x64-setup.exe" --prune-empty --tag-name-filter cat -- XF150v2
    Write-Host "git filter-branch 执行成功！" -ForegroundColor Green
} catch {
    Write-Host "git filter-branch 执行失败，尝试使用交互式rebase方法..." -ForegroundColor Yellow
    Write-Host "`n请手动执行以下步骤：" -ForegroundColor Yellow
    Write-Host "1. git rebase -i a25bc5d" -ForegroundColor Cyan
    Write-Host "2. 将 9f25b6a 行的 'pick' 改为 'edit'" -ForegroundColor Cyan
    Write-Host "3. 保存并关闭编辑器" -ForegroundColor Cyan
    Write-Host "4. git rm --cached maptiler-engine-14.1.2-app-win-x64-setup.exe" -ForegroundColor Cyan
    Write-Host "5. git commit --amend --no-edit" -ForegroundColor Cyan
    Write-Host "6. git rebase --continue" -ForegroundColor Cyan
    exit
}

# 验证结果
Write-Host "`n验证结果..." -ForegroundColor Yellow
git log --oneline -5
Write-Host "`n检查提交 9f25b6a 中的文件：" -ForegroundColor Yellow
git show 9f25b6a --name-only | Select-String "exe"

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n✅ exe文件已成功从历史中移除！" -ForegroundColor Green
    Write-Host "`n下一步：推送到远程仓库" -ForegroundColor Yellow
    Write-Host "使用命令：git push --force-with-lease origin XF150v2" -ForegroundColor Cyan
    Write-Host "`n注意：如果其他人也在使用这个分支，请先与他们协调！" -ForegroundColor Red
} else {
    Write-Host "`n⚠️ 请手动验证结果" -ForegroundColor Yellow
}
