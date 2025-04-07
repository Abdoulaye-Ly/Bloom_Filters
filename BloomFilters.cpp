#include <windows.h>
#include <commctrl.h>
#include <bitset>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <cctype>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

class BloomFilter {
private:
    bitset<1000> filter;

    size_t hash1(const wstring& key) {
        size_t hash = 5381;
        for (wchar_t c : key) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash % 1000;
    }

    size_t hash2(const wstring& key) {
        size_t hash = 0;
        for (wchar_t c : key) {
            hash = c + (hash << 6) + (hash << 16) - hash;
        }
        return hash % 1000;
    }

public:
    void add(const wstring& key) {
        filter.set(hash1(key));
        filter.set(hash2(key));
    }

    bool contains(const wstring& key) {
        return filter.test(hash1(key)) && filter.test(hash2(key));
    }
};

class UserDatabase {
private:
    vector<wstring> usernames;
    BloomFilter bloomFilter;

    bool isValidUsername(const wstring& username) {
        if (username.empty()) {
            return false;
        }
        if (username.length() == 1) {
            return false;
        }
        if (username.length() > 50) {
            return false;
        }
        if (username.find(L' ') != wstring::npos) {
            return false;
        }
        for (wchar_t c : username) {
            if (!iswalnum(c) && c != L'_') {
                return false;
            }
        }
        return true;
    }

    void generateSampleData() {
        for (int i = 0; i < 10000; i++) {
            wstring username = L"user" + to_wstring(i);
            usernames.push_back(username);
            bloomFilter.add(username);
        }
        saveToCSV(L"usernames.csv");
    }

    void saveToCSV(const wstring& filename) {
        ofstream file(filename);
        for (const auto& username : usernames) {
            string narrowStr(username.begin(), username.end());
            file << narrowStr << endl;
        }
    }

public:
    void loadFromCSV(const wstring& filename) {
        ifstream file(filename);
        if (!file.good()) {
            generateSampleData();
            return;
        }

        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                wstring wline(line.begin(), line.end());
                usernames.push_back(wline);
                bloomFilter.add(wline);
            }
        }

        if (usernames.size() < 10000) {
            generateSampleData();
        }
    }

    pair<bool, wstring> checkUsername(const wstring& username) {
        if (!isValidUsername(username)) {
            if (username.empty()) {
                return make_pair(false, L"Username cannot be empty");
            }
            if (username.length() == 1) {
                return make_pair(false, L"Username must be at least 2 characters");
            }
            if (username.length() > 50) {
                return make_pair(false, L"Username cannot exceed 50 characters");
            }
            if (username.find(L' ') != wstring::npos) {
                return make_pair(false, L"Username cannot contain spaces");
            }
            for (wchar_t c : username) {
                if (!iswalnum(c) && c != L'_') {
                    return make_pair(false, L"Username can only contain letters, numbers and underscores");
                }
            }
        }

        if (bloomFilter.contains(username)) {
            if (find(usernames.begin(), usernames.end(), username) != usernames.end()) {
                return make_pair(false, L"Username already taken");
            }
        }
        return make_pair(true, L"Username available");
    }

    pair<bool, wstring> addUser(const wstring& username) {
        pair<bool, wstring> checkResult = checkUsername(username);
        if (!checkResult.first) {
            return checkResult;
        }

        usernames.push_back(username);
        bloomFilter.add(username);

        ofstream file("usernames.csv", ios::app);
        string narrowStr(username.begin(), username.end());
        file << narrowStr << endl;

        return make_pair(true, L"Registration successful!");
    }

    const vector<wstring>& getAllUsernames() const {
        return usernames;
    }

    wstring comparePerformance() {
        const auto& allUsernames = getAllUsernames();
        vector<wstring> testUsernames;
        srand(static_cast<unsigned int>(time(nullptr)));

        // Create test cases (50 existing, 50 new)
        for (int i = 0; i < 50; i++) {
            testUsernames.push_back(allUsernames[rand() % allUsernames.size()]);
        }
        for (int i = 0; i < 50; i++) {
            testUsernames.push_back(L"testuser" + to_wstring(rand()));
        }

        // Test Bloom filter
        LARGE_INTEGER freq, bloomStart, bloomEnd;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&bloomStart);

        for (const auto& username : testUsernames) {
            checkUsername(username);
        }

        QueryPerformanceCounter(&bloomEnd);
        double bloomTime = (bloomEnd.QuadPart - bloomStart.QuadPart) * 1000.0 / freq.QuadPart;

        // Test linear search
        LARGE_INTEGER linearStart, linearEnd;
        QueryPerformanceCounter(&linearStart);

        for (const auto& username : testUsernames) {
            find(allUsernames.begin(), allUsernames.end(), username) != allUsernames.end();
        }

        QueryPerformanceCounter(&linearEnd);
        double linearTime = (linearEnd.QuadPart - linearStart.QuadPart) * 1000.0 / freq.QuadPart;

        // Calculate speedup
        double speedup = linearTime / bloomTime;

        // Format results
        wstringstream results;
        results << L"Performance Comparison (100 searches):\n";
        results << L"Bloom Filter: " << fixed << setprecision(3) << bloomTime << L" ms\n";
        results << L"Linear Search: " << fixed << setprecision(3) << linearTime << L" ms\n";
        results << L"Speedup: " << fixed << setprecision(1) << speedup << L"x faster\n";

        return results.str();
    }
};

UserDatabase g_userDB;
HWND g_hEditUsername, g_hStatusLabel, g_hMainWindow;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Enter desired username:",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 20, 200, 25, hwnd, NULL, NULL, NULL);

        g_hEditUsername = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20, 50, 300, 25, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Check Availability",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 90, 150, 30, hwnd, (HMENU)1, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Register",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            180, 90, 150, 30, hwnd, (HMENU)2, NULL, NULL);

        g_hStatusLabel = CreateWindowW(L"STATIC", L"Status: Ready",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 140, 300, 40, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Test Edge Cases",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 190, 200, 30, hwnd, (HMENU)3, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Compare Performance",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 230, 200, 30, hwnd, (HMENU)4, NULL, NULL);

        return 0;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // Check Availability
            wchar_t username[256];
            GetWindowTextW(g_hEditUsername, username, 256);

            pair<bool, wstring> result = g_userDB.checkUsername(username);
            SetWindowTextW(g_hStatusLabel, (L"Status: " + result.second).c_str());
        }
        else if (LOWORD(wParam) == 2) { // Register
            wchar_t username[256];
            GetWindowTextW(g_hEditUsername, username, 256);

            pair<bool, wstring> result = g_userDB.addUser(username);
            SetWindowTextW(g_hStatusLabel, (L"Status: " + result.second).c_str());
        }
        else if (LOWORD(wParam) == 3) { // Test Edge Cases
            vector<pair<wstring, wstring>> testCases = {
                make_pair(L"", L"Empty string"),
                make_pair(L"a", L"Single character"),
                make_pair(L"username with spaces", L"Contains spaces"),
                make_pair(L"verylongusernameverylongusernameverylongusernameverylongusernameverylongusername", L"Too long (51 chars)"),
                make_pair(L"user@name", L"Contains special char (@)"),
                make_pair(L"user_name", L"Valid with underscore"),
                make_pair(L"username123", L"Valid with numbers"),
                make_pair(L"USERNAME", L"Valid uppercase"),
                make_pair(L"username", L"Valid lowercase")
            };

            wstring results = L"Edge Case Test Results:\n\n";
            for (const auto& testPair : testCases) {
                pair<bool, wstring> testResult = g_userDB.checkUsername(testPair.first);
                results += testPair.second + L": " + testResult.second + L"\n";
            }

            MessageBoxW(hwnd, results.c_str(), L"Edge Case Test Results", MB_OK);
        }
        else if (LOWORD(wParam) == 4) { // Compare Performance
            wstring results = g_userDB.comparePerformance();
            MessageBoxW(hwnd, results.c_str(), L"Performance Comparison", MB_OK);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    g_userDB.loadFromCSV(L"usernames.csv");

    const wchar_t CLASS_NAME[] = L"BloomFilterWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    g_hMainWindow = CreateWindowExW(0, CLASS_NAME, L"Username Validator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL);

    if (g_hMainWindow == NULL) return 0;

    ShowWindow(g_hMainWindow, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}