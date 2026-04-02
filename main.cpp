#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>

// --- 定数 ---
const int CELL_SIZE = 60;
const int BOARD_MARGIN = 50;
const int BOARD_SIZE = 8;
const wchar_t* DATA_PATH = L"brain.dat";

enum class Space { EMPTY, BLACK, WHITE };
enum class Scene { TITLE, GAME, RESULT };

// --- グローバル変数 ---
Space board[BOARD_SIZE][BOARD_SIZE];
Space current_turn = Space::BLACK;
Scene current_scene = Scene::TITLE;
int cpu_level = 2; // 1:Easy, 2:Normal, 3:Hard
bool vs_cpu = true;
std::map<int, int> player_habits;
POINT last_move = {-1, -1};

// 評価テーブル（Normal/Hard用）
const int eval_table[8][8] = {
    {100, -20, 10,  5,  5, 10, -20, 100},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    { 10,  -2,  5,  1,  1,  5,  -2,  10},
    {  5,  -2,  1,  0,  0,  1,  -2,   5},
    {  5,  -2,  1,  0,  0,  1,  -2,   5},
    { 10,  -2,  5,  1,  1,  5,  -2,  10},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    {100, -20, 10,  5,  5, 10, -20, 100}
};

// --- ロジック ---
void LoadHabit() {
    std::ifstream ifs(DATA_PATH, std::ios::binary);
    int pos, count;
    while (ifs.read((char*)&pos, sizeof(pos)) && ifs.read((char*)&count, sizeof(count))) {
        player_habits[pos] = count;
    }
}

void SaveHabit(int r, int c) {
    player_habits[r * 10 + c]++;
    std::ofstream ofs(DATA_PATH, std::ios::binary);
    for (auto const& [pos, count] : player_habits) {
        ofs.write((char*)&pos, sizeof(pos));
        ofs.write((char*)&count, sizeof(count));
    }
}

bool IsValid(int r, int c, Space s) {
    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != Space::EMPTY) return false;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            bool found_opp = false;
            while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == (s == Space::BLACK ? Space::WHITE : Space::BLACK)) {
                nr += dr; nc += dc; found_opp = true;
            }
            if (found_opp && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == s) return true;
        }
    }
    return false;
}

void Flip(int r, int c, Space s) {
    board[r][c] = s;
    last_move = {c, r};
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            std::vector<std::pair<int, int>> targets;
            while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == (s == Space::BLACK ? Space::WHITE : Space::BLACK)) {
                targets.push_back({nr, nc});
                nr += dr; nc += dc;
            }
            if (!targets.empty() && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == s) {
                for (auto& p : targets) board[p.first][p.second] = s;
            }
        }
    }
}

// CPU思考ロジック
void CpuMove() {
    auto moves = std::vector<std::pair<int, int>>();
    for(int r=0; r<BOARD_SIZE; ++r) for(int c=0; c<BOARD_SIZE; ++c) {
        if(IsValid(r, c, Space::WHITE)) moves.push_back({r, c});
    }
    if (moves.empty()) { current_turn = Space::BLACK; return; }

    std::sort(moves.begin(), moves.end(), [&](const auto& a, const auto& b){
        int sa = eval_table[a.first][a.second];
        int sb = eval_table[b.first][b.second];
        if (cpu_level == 3) {
            sa += player_habits[a.first * 10 + a.second] * 10;
            sb += player_habits[b.first * 10 + b.second] * 10;
        }
        return (cpu_level == 1) ? (rand() % 2 == 0) : (sa > sb);
    });

    Flip(moves[0].first, moves[0].second, Space::WHITE);
    current_turn = Space::BLACK;
}

// --- 描画 ---
void DrawUI(HDC hdc, HWND hwnd) {
    RECT rect; GetClientRect(hwnd, &rect);
    if (current_scene == Scene::TITLE) {
        SetTextColor(hdc, RGB(0, 0, 0));
        TextOut(hdc, 200, 100, L"OTHELLO ELITE", 13);
        TextOut(hdc, 150, 200, L"Press '1' for Easy, '2' for Normal, '3' for Hard", 48);
        TextOut(hdc, 150, 250, L"Press 'P' for 2-Player Mode", 27);
    } else {
        // 盤面描画
        HBRUSH hBg = CreateSolidBrush(RGB(34, 139, 34));
        SelectObject(hdc, hBg);
        Rectangle(hdc, BOARD_MARGIN, BOARD_MARGIN, BOARD_MARGIN + BOARD_SIZE * CELL_SIZE, BOARD_MARGIN + BOARD_SIZE * CELL_SIZE);
        DeleteObject(hBg);

        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                int x1 = BOARD_MARGIN + c * CELL_SIZE, y1 = BOARD_MARGIN + r * CELL_SIZE;
                Rectangle(hdc, x1, y1, x1 + CELL_SIZE, y1 + CELL_SIZE);

                // 石の描画
                if (board[r][c] != Space::EMPTY) {
                    HBRUSH sBrush = CreateSolidBrush(board[r][c] == Space::BLACK ? RGB(0, 0, 0) : RGB(255, 255, 255));
                    SelectObject(hdc, sBrush);
                    Ellipse(hdc, x1 + 5, y1 + 5, x1 + CELL_SIZE - 5, y1 + CELL_SIZE - 5);
                    DeleteObject(sBrush);
                    // ラストムーブのハイライト
                    if (last_move.x == c && last_move.y == r) {
                        HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
                        SelectObject(hdc, hPen);
                        SelectObject(hdc, GetStockObject(NULL_BRUSH));
                        Ellipse(hdc, x1 + 2, y1 + 2, x1 + CELL_SIZE - 2, y1 + CELL_SIZE - 2);
                        DeleteObject(hPen);
                    }
                } else if (IsValid(r, c, current_turn)) {
                    // ガイド表示
                    HBRUSH gBrush = CreateSolidBrush(RGB(60, 179, 113));
                    SelectObject(hdc, gBrush);
                    Ellipse(hdc, x1 + 20, y1 + 20, x1 + CELL_SIZE - 20, y1 + CELL_SIZE - 20);
                    DeleteObject(gBrush);
                }
            }
        }
        std::wstring status = L"Turn: " + std::wstring(current_turn == Space::BLACK ? L"Black" : L"White");
        if(vs_cpu) status += L" | Level: " + std::to_wstring(cpu_level);
        TextOut(hdc, BOARD_MARGIN, 10, status.c_str(), (int)status.length());
    }
}

// --- ウィンドウプロシージャ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        LoadHabit();
        return 0;
    case WM_CHAR:
        if (current_scene == Scene::TITLE) {
            if (wParam == '1' || wParam == '2' || wParam == '3') {
                cpu_level = (int)(wParam - '0');
                vs_cpu = true;
                current_scene = Scene::GAME;
            } else if (wParam == 'p' || wParam == 'P') {
                vs_cpu = false;
                current_scene = Scene::GAME;
            }
            if (current_scene == Scene::GAME) {
                for(int i=0; i<BOARD_SIZE; i++) for(int j=0; j<BOARD_SIZE; j++) board[i][j] = Space::EMPTY;
                board[3][3] = Space::WHITE; board[4][4] = Space::WHITE;
                board[3][4] = Space::BLACK; board[4][3] = Space::BLACK;
                current_turn = Space::BLACK;
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (current_scene == Scene::GAME) {
            int c = (LOWORD(lParam) - BOARD_MARGIN) / CELL_SIZE;
            int r = (HIWORD(lParam) - BOARD_MARGIN) / CELL_SIZE;
            if (IsValid(r, c, current_turn)) {
                Flip(r, c, current_turn);
                if (current_turn == Space::BLACK) SaveHabit(r, c);
                current_turn = (current_turn == Space::BLACK ? Space::WHITE : Space::BLACK);
                InvalidateRect(hwnd, NULL, TRUE);
                if (vs_cpu && current_turn == Space::WHITE) PostMessage(hwnd, WM_USER + 1, 0, 0);
            }
        }
        return 0;
    case WM_USER + 1:
        Sleep(500); CpuMove();
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawUI(hdc, hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE, PWSTR, int nS) {
    const wchar_t CN[] = L"OthelloElite";
    WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = CN;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CN, L"Othello Elite", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 650, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nS);
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
