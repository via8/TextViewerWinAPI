#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include "TextModel.h"
#include "Menu.h"
#include "Error.h"

// declare Windows procedure
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nCmdShow) {
    static char szApplicationName[] = "TextViewer";
    HWND hWindow;                       // handle for our window
    MSG message;                        // here message to the application is saved
    WNDCLASSEX windowClassExtended;     // data structure for the window class

    // REMOVED 30/11/2019:
    // if (lpszArgument == NULL || strcmp(lpszArgument, "") == 0) {
    //     PrintError(NULL, ERR_ARGC, __FILE__, __LINE__);
    //     return ERR_ARGC;
    // }

    // the window structure
    windowClassExtended.hInstance     = hThisInstance;
    windowClassExtended.lpszClassName = szApplicationName;
    windowClassExtended.lpfnWndProc   = WindowProcedure;
    windowClassExtended.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClassExtended.cbSize        = sizeof(WNDCLASSEX);

    // use default icon and mouse-pointer
    windowClassExtended.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    windowClassExtended.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClassExtended.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    windowClassExtended.hIcon   = LoadIcon(NULL, IDI_APPLICATION);
    windowClassExtended.lpszMenuName = "Menu";
    windowClassExtended.cbClsExtra = 0;         // no extra bytes after the window class structure
    windowClassExtended.cbWndExtra = 0;         // or the window instance

    // register the window class, and if it fails quit the program
    if (!RegisterClassEx(&windowClassExtended)) {
        PrintError(NULL, ERR_UNKNOWN, __FILE__, __LINE__);
        return ERR_UNKNOWN;
    }

    // the class is registered, let's create the program
    hWindow = CreateWindowEx(
           0,                   // extended possibilities for variation
           windowClassExtended.lpszClassName,
           windowClassExtended.lpszClassName,
           WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
           CW_USEDEFAULT,       // windows decides the position
           CW_USEDEFAULT,       // where the window ends up on the screen
           CW_USEDEFAULT,       // the program's width
           CW_USEDEFAULT,       // and height in pixels
           HWND_DESKTOP,        // the window is a child-window to desktop
           NULL,
           hThisInstance,       // program Instance handler
           lpszArgument         // window Creation data (command line argument passed)
           );

    // make the window visible on the screen
    ShowWindow (hWindow, nCmdShow);

    // run the message loop. It will run until GetMessage() returns 0
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);    // Translate virtual-key message into character message
        DispatchMessage(&message);     // Send message to WindowProcedure
    }

    // the program return-value is the value that PostQuitMessage() gave
    return message.wParam;
}

/**
 * Initializes OPENFILENAME structure with information for Open dialog box.
 * IN:
 * @param hWindow - handler of window
 *
 * OUT:
 * @param openFilename - pointer to OPENFILENAME structure, initialized with default values
 */
void InitOpenFilename(HWND hWindow, OPENFILENAME * openFilename) {
    char * szFilter = "Text Files(*.TXT)\0*.txt\0";

    openFilename->lStructSize       = sizeof(OPENFILENAME);
    openFilename->hwndOwner         = hWindow;
    openFilename->hInstance         = NULL;
    openFilename->lpstrFilter       = szFilter;
    openFilename->lpstrCustomFilter = NULL;
    openFilename->nMaxCustFilter    = 0;
    openFilename->nFilterIndex      = 0;
    openFilename->lpstrFile         = NULL;
    openFilename->nMaxFile          = _MAX_PATH;
    openFilename->lpstrFileTitle    = NULL;
    openFilename->nMaxFileTitle     = _MAX_FNAME + _MAX_EXT;
    openFilename->lpstrInitialDir   = NULL;
    openFilename->lpstrTitle        = NULL;
    openFilename->Flags             = 0;
    openFilename->nFileOffset       = 0;
    openFilename->nFileExtension    = 0;
    openFilename->lpstrDefExt       = "txt";
    openFilename->lCustData         = 0L;
    openFilename->lpfnHook          = NULL;
    openFilename->lpTemplateName    = NULL;
}

/**
 * Pops standard Windows file open dialog to get filename to process
 * and saves this information in OPENFILENAME structure.
 * IN:
 * @param hWindow - handler of window
 * @param openFilename - pointer to structure for saving info
 *
 * OUT:
 * @param pstrFilename - buffer, saves name of file to open
 * @return result of comdlg32.lib function GetOpenFileName()
 * (0 if failed or dialog box was closed or Cancel button was pressed)
 */
BOOL PopFileOpenDialog(HWND hWindow, OPENFILENAME * openFilename, PSTR pstrFilename) {
    openFilename->hwndOwner = hWindow;
    openFilename->lpstrFile = pstrFilename;
    openFilename->Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT;

    return GetOpenFileName((LPOPENFILENAME)openFilename);
}

// this function is called by the Windows function DispatchMessage()
LRESULT CALLBACK WindowProcedure (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
    static TextModel model = { NULL, NULL };
    static ErrorType errorType = ERR_NO;
    HDC hDeviceContext;
    PAINTSTRUCT paintStruct;
    TEXTMETRIC textMetric;
    RECT invalidRectangle;
    RECT invalidChars;
    char const * line = NULL;
    long capacityCharsX;
    long prevFirstSymbol;
    long prevFirstLine;
    long lineLength;
    long incrementX;
    long incrementY;
    int scrollPosition;
    OPENFILENAME openFilename;
    PSTR pstrFilename;

    // ADDED 30/11/2019:
    if (errorType != ERR_NO) {
        return DefWindowProc (hWindow, message, wParam, lParam);
    }

    // handle message
    switch (message) {
    case WM_CREATE:
        // processing input file to build stored and displayed models
        errorType = BuildTextModel(&model, *(char**)lParam);
        if (errorType != ERR_NO) {
            SendMessage(hWindow, WM_DESTROY, 0, 0);
            break;
        }

        // device context initialization
        hDeviceContext = GetDC(hWindow);
        SetMapMode(hDeviceContext, MM_TEXT);
        SelectObject(hDeviceContext, GetStockObject(SYSTEM_FIXED_FONT));

        // set text metrics
        GetTextMetrics(hDeviceContext, &textMetric);
        model.displayed->charPixelsX = textMetric.tmAveCharWidth;
        model.displayed->charPixelsY = textMetric.tmHeight;

        ReleaseDC(hWindow, hDeviceContext);

        break;
    // WM_CREATE

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_OPEN:
            InitOpenFilename(hWindow, &openFilename);
            pstrFilename = (PSTR)calloc(_MAX_PATH, sizeof(char));
            if (PopFileOpenDialog(hWindow, &openFilename, pstrFilename)) {
                errorType = RebuildTextModel(&model, openFilename.lpstrFile);
                if (errorType != ERR_NO) {
                    free(pstrFilename);
                    SendMessage(hWindow, WM_DESTROY, 0, 0);
                }
            }
            free(pstrFilename);
            break;

        case IDM_FILE_EXIT:
            DestroyTextModel(&model);
            PostMessage(hWindow, WM_CLOSE, 0, 0);
            break;

        case IDM_VIEW_STANDARD:
            if (model.displayed->viewMode != VIEW_MODE_STANDARD)
                SwitchMode(model.stored, model.displayed, VIEW_MODE_STANDARD);
            break;

        case IDM_VIEW_WRAP:
            if (model.displayed->viewMode != VIEW_MODE_WRAP)
                SwitchMode(model.stored, model.displayed, VIEW_MODE_WRAP);
            break;

        default:
            PrintError(NULL, ERR_UNKNOWN, __FILE__, __LINE__);
            break;
        }

        // common actions for listed commands
        if (LOWORD(wParam) == IDM_FILE_OPEN ||
            LOWORD(wParam) == IDM_VIEW_STANDARD ||
            LOWORD(wParam) == IDM_VIEW_WRAP) {
                // update metrics binded with window size
                // 0 passed as a parameter to force recount of linesNumberWrap
                UpdateModelMetrics(hWindow, model.stored, model.displayed, 0);

                // force repaint
                InvalidateRect(hWindow, NULL, TRUE);
                UpdateWindow(hWindow);
            }

        break;
    // WM_COMMAND

    case WM_SIZE:
        // if error has alreary occured during opening file
        if (errorType != ERR_NO)
            break;

        // set new metrics of displayed model
        capacityCharsX = model.displayed->capacityCharsX;   // temp value with previous capacity
        model.displayed->clientAreaX    = LOWORD(lParam);
        model.displayed->clientAreaY    = HIWORD(lParam);
        model.displayed->capacityCharsX = LOWORD(lParam) / model.displayed->charPixelsX;
        model.displayed->capacityCharsY = HIWORD(lParam) / model.displayed->charPixelsY;

        UpdateModelMetrics(hWindow, model.stored, model.displayed, capacityCharsX);
        break;
    // WM_SIZE

    case WM_MOVE:
        InvalidateRect(hWindow, NULL, TRUE);
        break;
    // WM_MOVE

    case WM_HSCROLL:
        if (model.displayed->viewMode != VIEW_MODE_STANDARD)
            break;
        incrementX = 0;

        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            incrementX = -1;
            break;
        case SB_LINEDOWN:
            incrementX = 1;
            break;
        case SB_PAGEUP:
            incrementX = -model.displayed->capacityCharsX;
            break;
        case SB_PAGEDOWN:
            incrementX = model.displayed->capacityCharsX;
            break;
        case SB_THUMBTRACK:
            incrementX = scrollToIncrementX(model.stored, model.displayed, HIWORD(wParam));
            break;
        default:
            break;
        }

        if (incrementX != 0) {
            // update model and set valid increment
            incrementX = UpdateModelStandardX(model.stored, model.displayed, incrementX);

            // set window changes
            ScrollWindow(hWindow, -incrementX * model.displayed->charPixelsX, 0, NULL, NULL);
            SetInvalidRectagleX(model.displayed, incrementX, &invalidRectangle);
            InvalidateRect(hWindow, &invalidRectangle, TRUE);

            // process scrollbars changes
            if (LOWORD(wParam) == SB_THUMBTRACK)
                scrollPosition = HIWORD(wParam);
            else
                scrollPosition = countScrollPositionX(model.stored, model.displayed);

            SetScrollPos(hWindow, SB_HORZ, scrollPosition, TRUE);
        }

        break;
    // WM_HSCROLL

    case WM_VSCROLL:
        incrementY = 0;

        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            incrementY = -1;
            break;
        case SB_LINEDOWN:
            incrementY = 1;
            break;
        case SB_PAGEUP:
            incrementY = -model.displayed->capacityCharsY;
            break;
        case SB_PAGEDOWN:
            incrementY = model.displayed->capacityCharsY;
            break;
        case SB_THUMBTRACK:
            incrementY = HIWORD(wParam);
            incrementY = scrollToIncrementY(model.stored, model.displayed, HIWORD(wParam));
            break;
        default:
            break;
        }

        if (incrementY != 0) {
            // update model and set valid increment
            if (model.displayed->viewMode == VIEW_MODE_STANDARD)
                incrementY = UpdateModelStandardY(model.stored, model.displayed, incrementY);
            if (model.displayed->viewMode == VIEW_MODE_WRAP)
                incrementY = UpdateModelWrapY(model.stored, model.displayed, incrementY);

            // set window changes
            ScrollWindow(hWindow, 0, -incrementY * model.displayed->charPixelsY, NULL, NULL);
            SetInvalidRectagleY(model.displayed, incrementY, &invalidRectangle);
            InvalidateRect(hWindow, &invalidRectangle, TRUE);

            // process scrollbars changes
            if (LOWORD(wParam) == SB_THUMBTRACK)
                scrollPosition = HIWORD(wParam);
            else
                scrollPosition = countScrollPositionY(model.stored, model.displayed);

            SetScrollPos(hWindow, SB_VERT, scrollPosition, TRUE);
        }

        break;
    // WM_VSCROLL

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:
            PostMessage(hWindow, WM_VSCROLL, SB_LINEUP, (LPARAM)0);
            break;
        case VK_DOWN:
            PostMessage(hWindow, WM_VSCROLL, SB_LINEDOWN, (LPARAM)0);
            break;
        case VK_LEFT:
            PostMessage(hWindow, WM_HSCROLL, SB_LINEUP, (LPARAM)0);
            break;
        case VK_RIGHT:
            PostMessage(hWindow, WM_HSCROLL, SB_LINEDOWN, (LPARAM)0);
            break;
        case VK_PRIOR:
            PostMessage(hWindow, WM_VSCROLL, SB_PAGEUP, (LPARAM)0);
            break;
        case VK_NEXT:
            PostMessage(hWindow, WM_VSCROLL, SB_PAGEDOWN, (LPARAM)0);
            break;
        case VK_HOME:
            PostMessage(hWindow, WM_HSCROLL, SB_PAGEUP, (LPARAM)0);
            break;
        case VK_END:
            PostMessage(hWindow, WM_HSCROLL, SB_PAGEDOWN, (LPARAM)0);
            break;
        default:
            break;
        }

        break;
    // WM_KEYDOWN

    case WM_PAINT:
        hDeviceContext = BeginPaint(hWindow, &paintStruct);
        switch (model.displayed->viewMode) {
        case VIEW_MODE_STANDARD:
            invalidChars.top    = paintStruct.rcPaint.top    / model.displayed->charPixelsY;
            invalidChars.bottom = paintStruct.rcPaint.bottom / model.displayed->charPixelsY;
            invalidChars.left   = paintStruct.rcPaint.left   / model.displayed->charPixelsX;
            invalidChars.right  = paintStruct.rcPaint.right  / model.displayed->charPixelsX;

            capacityCharsX = invalidChars.right - invalidChars.left;
            for (incrementY = invalidChars.top; incrementY < invalidChars.bottom; ++incrementY) {
                line = GetLineStandard(model.stored,
                                       model.displayed->firstLine + incrementY,
                                       model.displayed->firstSymbol + invalidChars.left,
                                       capacityCharsX,
                                       &lineLength);
                if (line == NULL) break;
                TextOut(hDeviceContext,
                        paintStruct.rcPaint.left,
                        incrementY * model.displayed->charPixelsY,
                        line,
                        lineLength);
            }
            break;

        case  VIEW_MODE_WRAP:
            invalidChars.top    = paintStruct.rcPaint.top    / model.displayed->charPixelsY;
            invalidChars.bottom = paintStruct.rcPaint.bottom / model.displayed->charPixelsY;

            // process first line
            prevFirstLine   = model.displayed->firstLine;
            prevFirstSymbol = model.displayed->firstSymbol;
            line = GetLineWrap(model.stored, model.displayed, invalidChars.top, &lineLength, &prevFirstSymbol, &prevFirstLine);
            if (line == NULL) break;
            TextOut(hDeviceContext, 0, paintStruct.rcPaint.top, line, lineLength);

            // process the remaining lines
            for (incrementY = invalidChars.top + 1; incrementY < invalidChars.bottom; ++incrementY) {
                line = GetLineWrap(model.stored, model.displayed, 1, &lineLength, &prevFirstSymbol, &prevFirstLine);
                if (line == NULL) break;
                TextOut(hDeviceContext,
                        0,
                        incrementY * model.displayed->charPixelsY,
                        line,
                        lineLength);
            }
            break;

        default:
            break;
        }

        EndPaint(hWindow, &paintStruct);
        break;
    // WM_PAINT

    case WM_DESTROY:
        DestroyTextModel(&model);
        PostQuitMessage(errorType);
        break;
    // WM_DESTROY

    default:
        return DefWindowProc (hWindow, message, wParam, lParam);
    }

    return errorType;
}
