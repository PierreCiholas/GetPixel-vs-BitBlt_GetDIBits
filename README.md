# GetPixel-vs-BitBlt_GetDIBits
Context: I was trying to get RGB value of pixels on screen or from a window client area.

Is doing a GetPixel(hdc, x, y) for each pixels faster than a BitBlt &amp; GetDIBits?

The question might be stupid if you know more about the internals of these functions, but I didn't know much about what's under the hood.
After checking out the documentation about GetPixel I saw that it takes only an HDC, and position X and Y of the pixel.
I couldn't see it sub-calling BitBlt or any other "heavy" function after having a very brief look (few seconds) in IDA.
So I thought, "Hey, maybe it could be more performant than using BitBlt and GetDIBits?"

I have coded a C++ class that can do both.
Didn't even have to make them compete or do advanced benchmarking:
BitBlt & GetDIBits run at about 60-75 captures per second on my machine, whereas it took about a minute for GetPixels to get 68x68 pixels.

So, the answer: absolutely not.

Also, even if it worked, you'd have to deal with the frame refreshing mid capture.
