#include <windows.h>
#include <d2d1.h>
#include <vector>
#include <string>

#pragma comment(lib, "d2d1.lib")

// --- 定数・グローバル ---
const int BOARD_SIZE = 8;
const float CELL_SIZE = 60.0f;
const float MARGIN = 40.0f;

enum class Space { EMPTY, BLACK, WHITE };
Space board[BOARD_SIZE][BOARD_SIZE];
bool is_title = true;

// Direct2D インターフェース
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRT = NULL;
ID2D1SolidColorBrush* pBrushBrush = NULL;

// --- 初期化 ---
HRESULT InitD2D(HWND hwnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
    if (SUCCEEDED(hr)) {
        RECT rc; GetClientRect(hwnd, &rc);
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)),
            &pRT);
    }
    return hr;
}

void CleanD2D() {
    if (pRT) pRT->Release();
    if (pFactory) pFactory->Release();
}

// --- 描画ロジック ---
void Render(HWND hwnd) {
    if (!pRT) return;

    pRT->BeginDraw();
    pRT->Clear(D2D1::ColorF(0.1f, 0.1f, 0.12f)); // ダーク背景

    if (is_title) {
        // タイトル描画（簡略化）
        pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrushBrush);
        pRT->DrawRectangle(D2D1::RectF(100, 100, 500, 200), pBrushBrush);
        pBrushBrush->Release();
    } else {
        // 盤面描画
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                D2D1_RECT_F rect = D2D1::RectF(
                    MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE,
                    MARGIN + (c + 1) * CELL_SIZE, MARGIN + (r + 1) * CELL_SIZE
                );

                pRT->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.4f, 0.2f), &pBrushBrush);
                pRT->FillRectangle(rect, pBrushBrush);
                pBrushBrush->Release();

                pRT->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &pBrushBrush);
                pRT->DrawRectangle(rect, pBrushBrush);
                pBrushBrush->Release();

                if (board[r][c] != Space::EMPTY) {
                    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                        D2D1::Point2F(rect.left + CELL_SIZE / 2, rect.top + CELL_SIZE / 2),
                        CELL_SIZE / 2 - 5, CELL_SIZE / 2 - 5
                    );
                    pRT->CreateSolidColorBrush(
                        board[r][c] == Space::BLACK ? D2D1::ColorF(0, 0, 0) : D2D1::ColorF(1, 1, 1),
                        &pBrushBrush
                    );
                    pRT->FillEllipse(ellipse, pBrushBrush);
                    pBrushBrush->Release();
                }
            }
        }
    }

    pRT->EndDraw();
}

// --- ウィンドウプロシージャ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        InitD2D(hwnd);
        for(int i=0; i<4; i++) board[3+i/2][3+i%2] = (i==1||i==2)?Space::BLACK:Space::WHITE;
        return 0;
    case WM_LBUTTONDOWN:
        is_title = false;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_PAINT:
        Render(hwnd);
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_SIZE:
        if (pRT) {
            RECT rc; GetClientRect(hwnd, &rc);
            pRT->Resize(D2D1::SizeU(rc.right, rc.bottom));
        }
        return 0;
    case WM_DESTROY:
        CleanD2D();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE, PWSTR, int nS) {
    WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"D2DOthello";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, L"D2DOthello", L"Direct2D Elite Othello", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 650, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nS);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
