# GetPixel-vs-BitBlt_GetDIBits
To get RGB value of pixels: Is doing a GetPixel(hdc, x, y) for each pixels faster than a BitBlt &amp; GetDIBits? The answer: absolutely not. GetPixel took about a minute to capture 68x68 pixels.
