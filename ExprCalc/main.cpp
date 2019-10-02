#include <windows.h>
#include <windowsx.h>
#include <richedit.h>
#include <sstream>
#include "resource.h"
#include "PowerExpression/PowerExpression.h"

using namespace std;

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

static HINSTANCE g_hInst;
static HHOOK g_hhook;
static HWND g_hDlg;
static HWND g_hEdit;
static HFONT g_hFont;
static bool g_bRunning;
static WNDPROC g_oldProc;
static HMENU g_hMenu;
//static HCURSOR g_hArrow, g_hInsert;

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK keyBoardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditTextProc(HWND hEdit, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	g_hInst = hInstance;

	//加载RichEdit2.0库文件
	HMODULE richEditModule = LoadLibrary(TEXT("riched20.dll"));

	//安装键盘钩子
	g_hhook = SetWindowsHookEx(WH_KEYBOARD, keyBoardProc, g_hInst, GetCurrentThreadId());

	//获取右键菜单
	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDM_RIGHTBUTTON));
	g_hMenu = GetSubMenu(g_hMenu, 0);

	//加载光标
	//g_hArrow = LoadCursor(g_hInst, IDC_ARROW);
	//g_hInsert = LoadCursor(g_hInst, IDC_IBEAM);

	//创建字体
	g_hFont = CreateFont(
		20, 0, //高度、宽度
		0, 0,  //文本倾斜、字体倾斜
		FW_NORMAL, //粗体
		FALSE, FALSE, FALSE, //斜体、下划线、中划线
		ANSI_CHARSET,
		OUT_CHARACTER_PRECIS,
		CLIP_CHARACTER_PRECIS,
		DEFAULT_QUALITY,
		FF_MODERN,
		TEXT("consolas")); //字体名字

	//打开对话框
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlgProc);

	//卸载键盘钩子
	if (g_hhook != NULL)
	{
		UnhookWindowsHookEx(g_hhook);
	}

	//释放RichEdit2.0库文件
	FreeLibrary(richEditModule);

	return 0;
}

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD len;
	LPWCH buf;
	char* expr;
	double val;

	switch (message)
	{
	case WM_INITDIALOG:
		g_hDlg = hDlg;
		//获取编辑控件
		g_hEdit = GetDlgItem(hDlg, IDE_EXPR);
		//获取编辑控件窗口过程
		g_oldProc = (WNDPROC)GetWindowLong(g_hEdit, GWL_WNDPROC);
		//替换编辑控件窗口过程
		SetWindowLong(g_hEdit, GWL_WNDPROC, (LONG)EditTextProc);
		//设置编辑框字体
		SendMessage(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, 0);
		//SetFocus(g_hEdit);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_RUN:
			g_bRunning = true;

			//读取编辑框中的内容
			len = Edit_GetTextLength(g_hEdit);
			buf = (LPWCH)malloc(sizeof(TCHAR) * (len + 1));
			Edit_GetText(g_hEdit, buf, len + 1);
			
			//将宽字节字符串转换成多字节字符串
			len = WideCharToMultiByte(CP_ACP, 0, buf, -1, NULL, 0, NULL, NULL);
			expr = (char*)malloc(sizeof(char) * (len + 1));
			WideCharToMultiByte(CP_ACP, 0, buf, -1, expr, len, NULL, NULL);
			free(buf);

			try
			{
				//运行代码
				val = PowerExpression(expr).eval();

				//显示结果
				MessageBoxA(hDlg, (stringstream() << val).str().c_str(), "运行结果", MB_OK);
			}
			//异常处理
			catch (ParseError e)
			{
				MessageBoxA(hDlg, e.info().c_str(), "语法错误", MB_OK | MB_ICONERROR);
			}
			catch (RuntimeError e)
			{
				MessageBoxA(hDlg, e.info().c_str(), "运行时错误", MB_OK | MB_ICONERROR);
			}

			free(expr);
			g_bRunning = false;
			SetFocus(g_hEdit);
			break;
		default:
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

LRESULT CALLBACK keyBoardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	DWORD keyDownMask = 0x80000000;

	//处理Tab键消息 向编辑框中插入制表符
	if ((wParam == VK_TAB) && !(lParam & keyDownMask))
	{
		if (GetFocus() == g_hEdit)
		{
			Edit_ReplaceSel(g_hEdit, TEXT("    "));
		}
		return 1;
	}
	//按F5运行
	else if (wParam == VK_F5 && !(lParam & keyDownMask) && !g_bRunning)
	{
		SendMessage(g_hDlg, WM_COMMAND, (WPARAM)IDM_RUN, 0);
	}
	
	return CallNextHookEx(g_hhook, nCode, wParam, lParam);
}

LRESULT CALLBACK EditTextProc(HWND hEdit, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT p;

	switch (message)
	{
	//编辑框内右键被按下
	case WM_RBUTTONUP:
		//弹出右键菜单
		p.x = LOWORD(lParam);
		p.y = HIWORD(lParam);
		ClientToScreen(hEdit, &p);
		TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, p.x, p.y, 0, hEdit, NULL);
		break;
	//右键菜单事件
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_COPY:
			//复制选中
			SendMessage(hEdit, WM_COPY, 0, 0);
			break;
		case IDM_PASTE:
			//粘贴
			SendMessage(hEdit, WM_PASTE, 0, 0);
			break;
		case IDM_CUT:
			//剪切
			SendMessage(hEdit, WM_CUT, 0, 0);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return CallWindowProc(g_oldProc, hEdit, message, wParam, lParam);
}