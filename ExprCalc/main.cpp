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

	//����RichEdit2.0���ļ�
	HMODULE richEditModule = LoadLibrary(TEXT("riched20.dll"));

	//��װ���̹���
	g_hhook = SetWindowsHookEx(WH_KEYBOARD, keyBoardProc, g_hInst, GetCurrentThreadId());

	//��ȡ�Ҽ��˵�
	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDM_RIGHTBUTTON));
	g_hMenu = GetSubMenu(g_hMenu, 0);

	//���ع��
	//g_hArrow = LoadCursor(g_hInst, IDC_ARROW);
	//g_hInsert = LoadCursor(g_hInst, IDC_IBEAM);

	//��������
	g_hFont = CreateFont(
		20, 0, //�߶ȡ����
		0, 0,  //�ı���б��������б
		FW_NORMAL, //����
		FALSE, FALSE, FALSE, //б�塢�»��ߡ��л���
		ANSI_CHARSET,
		OUT_CHARACTER_PRECIS,
		CLIP_CHARACTER_PRECIS,
		DEFAULT_QUALITY,
		FF_MODERN,
		TEXT("consolas")); //��������

	//�򿪶Ի���
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlgProc);

	//ж�ؼ��̹���
	if (g_hhook != NULL)
	{
		UnhookWindowsHookEx(g_hhook);
	}

	//�ͷ�RichEdit2.0���ļ�
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
		//��ȡ�༭�ؼ�
		g_hEdit = GetDlgItem(hDlg, IDE_EXPR);
		//��ȡ�༭�ؼ����ڹ���
		g_oldProc = (WNDPROC)GetWindowLong(g_hEdit, GWL_WNDPROC);
		//�滻�༭�ؼ����ڹ���
		SetWindowLong(g_hEdit, GWL_WNDPROC, (LONG)EditTextProc);
		//���ñ༭������
		SendMessage(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, 0);
		//SetFocus(g_hEdit);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_RUN:
			g_bRunning = true;

			//��ȡ�༭���е�����
			len = Edit_GetTextLength(g_hEdit);
			buf = (LPWCH)malloc(sizeof(TCHAR) * (len + 1));
			Edit_GetText(g_hEdit, buf, len + 1);
			
			//�����ֽ��ַ���ת���ɶ��ֽ��ַ���
			len = WideCharToMultiByte(CP_ACP, 0, buf, -1, NULL, 0, NULL, NULL);
			expr = (char*)malloc(sizeof(char) * (len + 1));
			WideCharToMultiByte(CP_ACP, 0, buf, -1, expr, len, NULL, NULL);
			free(buf);

			try
			{
				//���д���
				val = PowerExpression(expr).eval();

				//��ʾ���
				MessageBoxA(hDlg, (stringstream() << val).str().c_str(), "���н��", MB_OK);
			}
			//�쳣����
			catch (ParseError e)
			{
				MessageBoxA(hDlg, e.info().c_str(), "�﷨����", MB_OK | MB_ICONERROR);
			}
			catch (RuntimeError e)
			{
				MessageBoxA(hDlg, e.info().c_str(), "����ʱ����", MB_OK | MB_ICONERROR);
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

	//����Tab����Ϣ ��༭���в����Ʊ��
	if ((wParam == VK_TAB) && !(lParam & keyDownMask))
	{
		if (GetFocus() == g_hEdit)
		{
			Edit_ReplaceSel(g_hEdit, TEXT("    "));
		}
		return 1;
	}
	//��F5����
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
	//�༭�����Ҽ�������
	case WM_RBUTTONUP:
		//�����Ҽ��˵�
		p.x = LOWORD(lParam);
		p.y = HIWORD(lParam);
		ClientToScreen(hEdit, &p);
		TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, p.x, p.y, 0, hEdit, NULL);
		break;
	//�Ҽ��˵��¼�
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_COPY:
			//����ѡ��
			SendMessage(hEdit, WM_COPY, 0, 0);
			break;
		case IDM_PASTE:
			//ճ��
			SendMessage(hEdit, WM_PASTE, 0, 0);
			break;
		case IDM_CUT:
			//����
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