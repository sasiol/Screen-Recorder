//#include "Gdiplus.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vfw.h> 
#include <string.h>
#include <string>

using std::string;
// Function declarations
BITMAPINFOHEADER createBitmapHeader(int width, int height);
BITMAPFILEHEADER createBitmapFileHeader(int width, int height, int imageSize);
HBITMAP captureScreen(HWND hWnd);
char* createBuffer(HBITMAP compBitmap);
void saveScreenShot(HBITMAP compBitmap, HDC hMemoryDC);
void appendNewFrame(std::ofstream& frameOut, HBITMAP compBitmap, long frameNum);



//creating a bitmap
BITMAPINFOHEADER createBitmapHeader(int width, int height) {
    BITMAPINFOHEADER  bi;

    bi.biSize=sizeof(BITMAPINFOHEADER);
    bi.biWidth=width;
    bi.biHeight=-height;
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
HBITMAP captureScreen(){
     HDC hScreenDC = GetDC(NULL); //gets DC of whole screen
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC); //creates a memory DC that is compatible with the screen DC

    int width = GetDeviceCaps(hScreenDC, HORZRES);
    int height = GetDeviceCaps(hScreenDC, VERTRES);
    HBITMAP compBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    
    SelectObject(hMemoryDC, compBitmap); //"any drawing operations to memory DC should be performed on the bitmap. "

     //copies the screen content to memory dc--> bit map
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
    //test to flip  the image later

    return compBitmap;

     DeleteDC(hMemoryDC);
}
char* createBuffer(HBITMAP compBitmap){

    HDC hScreenDC = GetDC(NULL);
     // Retrieve the bitmap data
    BITMAP bmp;
    GetObject(compBitmap, sizeof(BITMAP), &bmp);
    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

    int imageSize = ((width * 24 + 31) / 32) * 4 * height;
     // Allocate memory for the bitmap data
    char* bmpData = new char[imageSize]; 
  //  saveScreenShot(compBitmap, hMemoryDC); //for screenshot
   
    BITMAPINFOHEADER biHeader=createBitmapHeader(width, height);
    BITMAPFILEHEADER fileHeader=createBitmapFileHeader(width, height, imageSize);
    BITMAPINFO bmpInfo = {0};
    bmpInfo.bmiHeader = biHeader;

    // Puts the bitmap data to a buffer (bmpData) which is suitable for saving into a file
   if (!GetDIBits(hScreenDC, compBitmap, 0, height, bmpData, &bmpInfo, DIB_RGB_COLORS)) {
        std::cerr << "Failed to get bitmap data from device context." << std::endl;
        delete[] bmpData;
        
    }

    // Cleanup
    //SelectObject(hMemoryDC, NULL); //needed?
   // DeleteObject(compBitmap);
   // DeleteDC(hMemoryDC); //try without when adding video. what happens?
    ReleaseDC(NULL, hScreenDC);

    return bmpData;

}
//not needed atm
/*
void saveScreenShot(HBITMAP compBitmap, HDC hDC){ 

    // Retrieve the bitmap data
   
    BITMAP bmp;
    GetObject(compBitmap, sizeof(BITMAP), &bmp);
    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    


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
*/
void appendNewFrame(PAVISTREAM aviStream, HBITMAP compBitmap, LONG& frameNum){

     // Retrieve the bitmap data
    BITMAP bmp;
    GetObject(compBitmap, sizeof(BITMAP), &bmp);
    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

    int frameSize = ((width * 24 + 31) / 32) * 4 * height;

    char* bmBuffer=createBuffer(compBitmap);

  // Write the frame to the AVI stream
     // Write the frame to the AVI stream
    HRESULT hr = AVIStreamWrite(aviStream, frameNum, 1, bmBuffer, frameSize, AVIIF_KEYFRAME, NULL, NULL);
    if (hr != 0) {
        std::cerr << "Failed to write to AVI stream. Error: " << hr << std::endl;
    } else {
        std::cerr << "Frame " << frameNum << " written to AVI." << std::endl;
    }

    //delete[] bmpData;
   
   // AVIStreamRelease(aviStream);

}

PAVIFILE createAviFile(PAVISTREAM& aviStream, int width, int height){
    AVIFileInit();
    PAVIFILE aviFile;
    
    if (AVIFileOpen(&aviFile, TEXT("svid.avi"), OF_WRITE | OF_CREATE, NULL) != AVIERR_OK) {
        std::cerr << "Failed to open AVI file." << std::endl;
        return nullptr;
    }
    //avi stream info
    AVISTREAMINFO streamInfo = { 0 };
    streamInfo.fccType = streamtypeVIDEO;
    streamInfo.dwScale = 1;
    streamInfo.dwRate = 25; // 25 frames per second
    streamInfo.dwSuggestedBufferSize = width * height * 3;
    SetRect(&streamInfo.rcFrame, 0, 0, width, height);
   
    if (AVIFileCreateStream(aviFile, &aviStream, &streamInfo) != AVIERR_OK) {
        std::cerr << "Failed to create AVI stream." << std::endl;
        AVIFileRelease(aviFile);
        return nullptr;
    }
     BITMAPINFOHEADER biHeader = createBitmapHeader(width, height);
    if (AVIStreamSetFormat(aviStream, 0, &biHeader, sizeof(BITMAPINFOHEADER)) != AVIERR_OK) {
        std::cerr << "Failed to set AVI stream format." << std::endl;
        AVIStreamRelease(aviStream);
        AVIFileRelease(aviFile);
        return nullptr;
    }
  //  AVIStreamRelease(aviStream); // Release stream if it's not needed immediately
    //std::cerr << "wavi file and stream created" << std::endl;
    return aviFile;
    
}

int main() {
    HDC hScreenDC= GetDC(NULL);
    // Capture one frame to get dimensions
    int width = GetDeviceCaps(hScreenDC, HORZRES);
    int height = GetDeviceCaps(hScreenDC, VERTRES); 

    PAVISTREAM aviStream;
    PAVIFILE aviFile = createAviFile(aviStream, width, height);
    
    long i=1;
    while(i<40){
        HBITMAP compBitmap= captureScreen();
        appendNewFrame(aviStream, compBitmap, i );
        DeleteObject(compBitmap);
        i++;
    }

 AVIStreamRelease(aviStream);
AVIFileRelease(aviFile); // Release AVI file handle
AVIFileExit(); // Cleanup AVI file subsystem
std::cout << "Screenvideo saved as svid.avi" << std::endl;
 return 0;
}


