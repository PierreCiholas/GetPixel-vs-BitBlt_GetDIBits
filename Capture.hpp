#pragma once
#include <Windows.h>
#include <string>
// For screen capture
#include <ole2.h>
#include <olectl.h>

#define BUFFER_PIXEL_BYTES_SIZE 16 * 1024 * 1024 // 16MB should be more than enough (especially for 1920x1080)
#define DEFAULT_CAPTURE_FILENAME "capture.bmp"
#define DEFAULT_WNDCLASSNAME "Control"
#define DEFAULT_WNDTITLE ""

enum CAPTURE_MODE { CAPTURE_BITBLT, CAPTURE_GETPIXEL, CAPTURE_MODE_MAX };

class Capture {
public:
	// Constructors & destructors
	Capture();
	Capture(HWND hwndTarget);
	Capture(std::string strWindowName, std::string strWindowClassName = "");
	~Capture();

	// (Re)initialisation
	void Attach(HWND hwndTarget);
	void Attach(std::string strWindowName, std::string strWindowClassName = "");

	// Frame capture & processing
	DWORD CaptureClientFromScreen(RECT areaInClient = { -1, -1, -1, -1 });

	// TODO? Function that memcpy pixel values in a RECT?
	POINT GetFrameSize() { return { m_bitmapSize.x, m_bitmapSize.y }; };
	//PBYTE GetFullBitmap() { return m_pixelBuffer; }; // TODO?
	DWORD GetFullBitmapSize() { return m_dwSizeOfBmpWithHeaders; };
	PBYTE GetPixels() { return m_pixelBuffer; };
	DWORD GetPixelsSize() { return m_dwBmpSize; };
	BOOL BringToFront(); // Bring target window to front (asynchronous, won't be on top if frame captured right after)

	// For debugging
	DWORD WriteFrameOnDisk(std::string filename = DEFAULT_CAPTURE_FILENAME);

	HWND GetTargetHwnd() { return m_hwndTarget; };
	bool IsForeground() { return m_hwndTarget == GetForegroundWindow(); }
	bool SetForeground() { if (!m_hwndTarget) { return false; } else { return SetForegroundWindow(m_hwndTarget); } }
	int GetCaptureMode() { return m_captureMode; }
	bool SetCaptureMode(int mode) { if (mode >= CAPTURE_MODE_MAX) { return false; } m_captureMode = mode; return true; }

private:
	void Initialise();

protected:
	int m_captureMode = CAPTURE_GETPIXEL;

	// These member variables need to be cleaned in the destructor with OS API functions
	PBYTE m_pixelBuffer = nullptr;
	HBITMAP m_hbmTarget = nullptr;
	HDC m_hdcScreen = nullptr;
	HDC m_hdcMem = nullptr;

	// Keeping bitmap variables as member for additional feature (e.g. write last frame on disk, display in dedicated window, ...)
	POINT m_bitmapSize = { -1, -1 };
	BITMAPFILEHEADER m_bmfHeader;
	BITMAPINFOHEADER m_bi;
	DWORD m_dwPixelsNbr = m_bitmapSize.x * m_bitmapSize.y;

	HWND m_hwndTarget = nullptr;
	DWORD m_dwSizeOfBmpWithHeaders = 0;
	DWORD m_dwBmpSize = 0;
	HWND m_hwndDesktop = nullptr;
	RECT m_rcScreen = {};
};

Capture::Capture() {
	Initialise();
}


Capture::Capture(std::string strWindowName, std::string strWindowClassName) {
	Attach(strWindowName, strWindowClassName);
	Initialise();
}

Capture::Capture(HWND hwndTarget) {
	Attach(hwndTarget);
	Initialise();
}

void Capture::Initialise() {
	m_pixelBuffer = (LPBYTE)VirtualAlloc(NULL, BUFFER_PIXEL_BYTES_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!m_hwndTarget) // Couldn't find target, using desktop instead.
		m_hwndTarget = GetDesktopWindow();
	m_hwndDesktop = GetDesktopWindow();
	m_hdcScreen = GetDC(m_hwndDesktop);
	m_hdcMem = CreateCompatibleDC(m_hdcScreen);
	GetClientRect(m_hwndDesktop, &m_rcScreen);
}

void Capture::Attach(HWND hwndTarget) { m_hwndTarget = hwndTarget; }
void Capture::Attach(std::string strWindowName, std::string strWindowClassName) {
	if (strWindowClassName == "")
		m_hwndTarget = FindWindow(NULL, strWindowName.c_str());
	else
		m_hwndTarget = FindWindow(strWindowClassName.c_str(), strWindowName.c_str());
}

Capture::~Capture() {
	DeleteObject(m_hbmTarget);
	DeleteObject(m_hdcMem);
	ReleaseDC(NULL, m_hdcScreen);
	ReleaseDC(m_hwndDesktop, m_hdcScreen);
	VirtualFree(m_pixelBuffer, 0, MEM_RELEASE);
}

BOOL Capture::BringToFront() { return BringWindowToTop(m_hwndTarget) & SetForegroundWindow(m_hwndTarget); }

DWORD Capture::CaptureClientFromScreen(RECT areaInClient) {
	// Calculating position  and size to capture
	RECT rcTarget = {};
	POINT topLeftPixel = {};
	POINT targetClientAreaSize = {};

	// Getting coordinates of target window client area on screen
	ClientToScreen(m_hwndTarget, &topLeftPixel);
	// Get either specified area or full client area
	if (areaInClient.left != -1 && areaInClient.top != -1 && areaInClient.right != -1 && areaInClient.bottom != -1) {
		rcTarget = areaInClient;
		topLeftPixel.x = areaInClient.left;
		topLeftPixel.y = areaInClient.top;
	} else {
		GetClientRect(m_hwndTarget, &rcTarget);
	}

	targetClientAreaSize = { rcTarget.right - rcTarget.left, rcTarget.bottom - rcTarget.top };

	bool sizeHasChanged = false;
	if (m_bitmapSize.x != targetClientAreaSize.x || m_bitmapSize.y != targetClientAreaSize.y)
		sizeHasChanged = true;

	if (sizeHasChanged) {
		// Save new size
		m_bitmapSize.x = targetClientAreaSize.x;
		m_bitmapSize.y = targetClientAreaSize.y;
		m_dwPixelsNbr = m_bitmapSize.x * m_bitmapSize.y;
		
		// Update info for headers (We don't use GetObject since we know the size already)
		m_bi.biSize = sizeof(BITMAPINFOHEADER);
		m_bi.biWidth = targetClientAreaSize.x;
		m_bi.biHeight = targetClientAreaSize.y;
		m_bi.biHeight *= -1; // This lines is used to store the bitmap pixels from top to bottom, rather than bottom to top (mindfuck otherwise) (could perhaps use DSTINVERT, NOMIRRORBITMAP, NOTSRCCOPY with BitBlt for that too)
		m_bi.biPlanes = 1;
		m_bi.biBitCount = 32;
		m_bi.biCompression = BI_RGB;
		m_bi.biSizeImage = 0;
		m_bi.biXPelsPerMeter = 0;
		m_bi.biYPelsPerMeter = 0;
		m_bi.biClrUsed = 0;
		m_bi.biClrImportant = 0;
		m_dwBmpSize = ((targetClientAreaSize.x * m_bi.biBitCount + 31) / 32) * 4 * targetClientAreaSize.y; // Calculating size

		// Sets header information for proper bitmap, writeable to file on disk or whatever else
		m_dwSizeOfBmpWithHeaders = m_dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Add the size of the headers to the size of the bitmap to get the total file size
		m_bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER); // Offset to where the actual bitmap bits start.
		m_bmfHeader.bfSize = m_dwSizeOfBmpWithHeaders; // Size of the file
		m_bmfHeader.bfType = 0x4D42; // bfType must always be BM for Bitmaps
	}

	// BitBlt & GetDIBits capture method
	if (m_captureMode == CAPTURE_BITBLT) {
		// Bitmap management when size changes
		if (sizeHasChanged || !m_hbmTarget) {
			if (m_hbmTarget)
				DeleteObject(m_hbmTarget);
			m_hbmTarget = CreateCompatibleBitmap(m_hdcScreen, targetClientAreaSize.x, targetClientAreaSize.y);
			if (!m_hbmTarget) return 0;
			SelectObject(m_hdcMem, m_hbmTarget);
		}

		// BitBlt & GetDIBBits capture
		if (!m_hbmTarget) return 0;
		// Copies pixel to HDC memory
		if (!BitBlt(m_hdcMem, 0, 0, targetClientAreaSize.x, targetClientAreaSize.y, m_hdcScreen, topLeftPixel.x, topLeftPixel.y, SRCCOPY)) return 0;
		// Gets the "bits" from the bitmap and copies them into a buffer which is pointed to by pixelBuffer.
		if (!GetDIBits(m_hdcScreen, m_hbmTarget, 0, (UINT)targetClientAreaSize.y, m_pixelBuffer, (BITMAPINFO *)&m_bi, DIB_RGB_COLORS)) return 0;
		return m_dwBmpSize;
	}

	// GetPixel capture
	if (m_captureMode == CAPTURE_GETPIXEL) {
		DWORD byteNbr = 0;
		for (DWORD y = 0; y < m_bitmapSize.y; y++) {
			for (DWORD x = 0; x < m_bitmapSize.x; x++) {
				// TODO: X and Y can be negative (if on left screen)
				// TODO: flip RGB values
				*(COLORREF*)(m_pixelBuffer + byteNbr) = GetPixel(m_hdcScreen, x, y);
				byteNbr += 4;
			}
		}
		return m_dwBmpSize;
	}

	return 0;
}

DWORD Capture::WriteFrameOnDisk(std::string filename) {
	// Write to file (for debug)
	HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&m_bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&m_bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)m_pixelBuffer, m_dwBmpSize, &dwBytesWritten, NULL);
	CloseHandle(hFile);
	return dwBytesWritten;
}