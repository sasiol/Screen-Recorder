//#include "Gdiplus.h"
#include <windows.h>
#include <iostream>
#include <fstream>

// Function declarations
BITMAPINFOHEADER createBitmapHeader(int width, int height);
BITMAPFILEHEADER createBitmapFileHeader(int width, int height, int imageSize);
void captureScreen(HWND hWnd);
void save(HBITMAP compBitmap, HDC hMemoryDC);


//creating a bitmap
BITMAPINFOHEADER createBitmapHeader(int width, int height) {
    BITMAPINFOHEADER  bi;

    bi.biSize=sizeof(BITMAPINFOHEADER);
    bi.biWidth=width;
    bi.biHeight=-height;//+
    bi.biPlanes=1;
    bi.biBitCount=24; //32?
    bi.biCompression=BI_RGB;
    bi.biSizeImage=0;
    bi.biXPelsPerMeter=0;
    bi.biYPelsPerMeter=0; 
    bi.biClrUsed=0;
    bi.biClrImportant=0;

    return bi;
} 
//creating a file header
BITMAPFILEHEADER createBitmapFileHeader(int width, int height, int imageSize) {
    BITMAPFILEHEADER fileHeader;
    fileHeader.bfType = 0x4D42; 
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    return fileHeader;
} 
void captureScreen(){
    HDC hScreenDC = GetDC(NULL); //gets DC of whole screen
    
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC); //creates a memory DC that is compatible with the screen DC

    int width = GetDeviceCaps(hScreenDC, HORZRES);
    int height = GetDeviceCaps(hScreenDC, VERTRES);
    HBITMAP compBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    
    SelectObject(hMemoryDC, compBitmap); //"any drawing operations to memory DC should be performed on the bitmap. "

     //copies the screen content to memory dc--> bit map
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
    //test to flip  the image later

    save(compBitmap, hMemoryDC);

    // Cleanup
    //SelectObject(hMemoryDC, NULL); //needed?
    DeleteObject(compBitmap);
    DeleteDC(hMemoryDC); //try without when adding video. what happens?
    ReleaseDC(NULL, hScreenDC);

}

void save(HBITMAP compBitmap, HDC hDC){ 

    // Retrieve the bitmap data
    BITMAP bmp;
    GetObject(compBitmap, sizeof(BITMAP), &bmp);
    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

   

    int imageSize = ((width * 24 + 31) / 32) * 4 * height;
     // Allocate memory for the bitmap data
    char* bmpData = new char[imageSize]; 
     
    BITMAPINFOHEADER biHeader=createBitmapHeader(width, height);
    BITMAPFILEHEADER fileHeader=createBitmapFileHeader(width, height, imageSize);
    BITMAPINFO bmpInfo = {0};
    bmpInfo.bmiHeader = biHeader;

    // Puts the bitmap data to a buffer (bmpData) which is suitable for saving into a file
   if (!GetDIBits(hDC, compBitmap, 0, height, bmpData, &bmpInfo, DIB_RGB_COLORS)) {
        std::cerr << "Failed to get bitmap data from device context." << std::endl;
        delete[] bmpData;
        return;
    }

     // Debugging output to check the current working directory
    char cwd[1024];
    if (GetCurrentDirectoryA(1024, cwd)) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }

    // Write the bitmap to a file
    std::ofstream file("./spic.bmp", std::ios::out | std::ios::binary);
    //error handling
    if (!file) {
        std::cerr << "Failed to open file for writing." << std::endl;
        delete[] bmpData;
        return;
    }
    
    file.write((char*)&fileHeader, sizeof(fileHeader));
    file.write((char*)&bmpInfo.bmiHeader, sizeof(bmpInfo.bmiHeader));
    file.write(bmpData, imageSize);

    // Clean up
    file.close();
    delete[] bmpData;

    std::cout << "Screenshot saved as spic.bmp" << std::endl;

}
int main() {
    captureScreen();
    return 0;
}


