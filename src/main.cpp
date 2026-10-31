#include <thread>

  int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    return Win32App::Run(hInstance, nCmdShow);
  }

  int main() { return Win32App::Run(GetModuleHandle(nullptr), SW_SHOWDEFAULT); }
