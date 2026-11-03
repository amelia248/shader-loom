// #include <windows.h>
// #include <stdio.h>

// #pragma comment(lib, "comdlg32.lib")  /* MSVC only; with MinGW use -lcomdlg32 */

// int main(void)
// {
// 	static COLORREF customColors[16] = {0};  /* the dialog's custom swatches */
// 	CHOOSECOLOR cc = {0};

// 	cc.lStructSize  = sizeof(cc);
// 	cc.hwndOwner    = NULL;                  /* or your window handle in a GUI app */
// 	cc.rgbResult    = RGB(60, 196, 196);        /* initial colour shown in the dialog */
// 	cc.lpCustColors = customColors;
// 	cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
// 	cc

// 	if (ChooseColor(&cc)) {
// 			COLORREF c = cc.rgbResult;
// 			printf(
// 				"Picked: RGB(%d, %d, %d)  hex #%02X%02X%02X\n",
// 				GetRValue(c), GetGValue(c), GetBValue(c),
// 				GetRValue(c), GetGValue(c), GetBValue(c)
// 			);
// 	} else {
// 		DWORD err = CommDlgExtendedError();
// 		if (err)
// 			printf("ChooseColor failed, error %lu\n", err);
// 		else
// 			printf("Cancelled.\n");
// 	}
// 	return 0;
// }

#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "user32.lib")

static void CentreOver(HWND hwnd, HWND ref)
{
    RECT rc, rcRef;
    int x, y;

    GetWindowRect(hwnd, &rc);
    if (ref) {
        GetWindowRect(ref, &rcRef);
    } else {
        /* No owner: centre on the work area of the primary monitor */
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcRef, 0);
    }

    x = rcRef.left + ((rcRef.right  - rcRef.left) - (rc.right  - rc.left)) / 2;
    y = rcRef.top  + ((rcRef.bottom - rcRef.top)  - (rc.bottom - rc.top))  / 2;

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static UINT_PTR CALLBACK ColorHook(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)wParam;
    if (msg == WM_INITDIALOG) {
        /* lParam points to the CHOOSECOLOR we passed in */
        CHOOSECOLOR *cc = (CHOOSECOLOR *)lParam;
        CentreOver(hdlg, cc->hwndOwner);
        return TRUE;  /* let the dialog finish its own init */
    }
    return 0;
}

bool colourPicker(int* red, int* green, int* blue) {
    static COLORREF customColors[16] = {0};
    CHOOSECOLOR cc = {0};

    cc.lStructSize    = sizeof(cc);
    cc.hwndOwner      = NULL;               /* set to your window for a GUI app */
    cc.rgbResult      = RGB(*red, *green, *blue);
    cc.lpCustColors   = customColors;
    cc.Flags          = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
    cc.lpfnHook       = ColorHook;

    if (ChooseColor(&cc)) {
        COLORREF c = cc.rgbResult;
        printf("Picked: RGB(%d, %d, %d)  hex #%02X%02X%02X\n",
               GetRValue(c), GetGValue(c), GetBValue(c),
               GetRValue(c), GetGValue(c), GetBValue(c));
				
				*red   = GetRValue(c);
				*green = GetGValue(c);
				*blue  = GetBValue(c);
				return true;
    } else {
        DWORD err = CommDlgExtendedError();
        if (err) printf("ChooseColor failed, error %lu\n", err);
        else     printf("Cancelled.\n");
    }
    return false;
}