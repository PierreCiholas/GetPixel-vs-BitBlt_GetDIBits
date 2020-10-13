#include <Windows.h>
#include <iostream>
#include <chrono>
#include "Capture.hpp"

#define DEFAULT_WOW_WINDOW_NAME "World of Warcraft"
#define DEFAULT_WOW_WINDOW_CLASS "GxWindowClass"

using namespace std;

int main() {
	HWND wow = FindWindow(DEFAULT_WOW_WINDOW_CLASS, DEFAULT_WOW_WINDOW_NAME);
	Capture frameCapture(wow);
	frameCapture.SetForeground();
	Sleep(250);

	RECT pixelBuffer = { 1852, 1012, 1920, 1080 };

	while (true) {
		frameCapture.CaptureClientFromScreen(pixelBuffer);
		frameCapture.WriteFrameOnDisk();
		cout << "Frame saved." << endl;
		system("pause");
	}

	// Performance measurements
	std::chrono::milliseconds startMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	SHORT f12 = 0;
	int f = 0;

	while (true) {
		frameCapture.CaptureClientFromScreen(pixelBuffer);
		//frameCapture.CaptureClientFromScreen();
		//frameCapture.WriteFrameOnDisk();
		//system("pause");

		// Performance measurements
		f++;
		f12 = GetAsyncKeyState(VK_F12);
		if (f12 & 1) { // F12 pressed and released
			std::chrono::milliseconds nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
			long long diffMs = nowMs.count() - startMs.count();
			float diffSec = diffMs / 1000;
			float fps = f / diffSec;
			cout << fps << endl;
		}
	}

	//system("pause");
	return EXIT_SUCCESS;
}