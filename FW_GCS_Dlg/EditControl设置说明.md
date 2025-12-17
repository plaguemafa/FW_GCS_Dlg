# Edit Control 显示控件设置说明

## 一、属性设置步骤

1. **在资源编辑器中选中 Edit Control**
2. **右键 → Properties（属性）**
3. **设置以下属性：**

### 必须设置的属性：

| 属性名 | 设置值 | 说明 |
|--------|--------|------|
| **Read Only** | `TRUE` | 只读，用户无法编辑 |
| **Multiline** | `TRUE` | 多行显示 |
| **Vertical Scroll** | `TRUE` | 垂直滚动条 |
| **Auto VScroll** | `TRUE` | 自动垂直滚动 |
| **Want Return** | `FALSE` | 不需要回车键（可选） |

### 可选设置：

| 属性名 | 设置值 | 说明 |
|--------|--------|------|
| **Number** | `FALSE` | 不限制为数字（默认） |
| **Password** | `FALSE` | 不是密码框（默认） |
| **Border** | `TRUE` | 显示边框（默认） |

## 二、关于事件处理程序

**重要：对于只读显示控件，不需要创建事件处理程序！**

原因：
- 只读控件用户无法交互
- 我们只需要更新显示内容（SetWindowText）
- 不需要响应任何用户事件

## 三、控件ID

假设您创建的Edit Control的ID是：`IDC_EDIT_UDP_RECEIVE`

（如果您的ID不同，请告诉我，我会相应修改代码）

