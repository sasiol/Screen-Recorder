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
void captureScreen(HWND hWnd);
void save(HBITMAP compBitmap, HDC hMemoryDC);
void appendNewFrame(std::ofstream& frameOut, HBITMAP compBitmap, long frameNum);



//creating a bitmap
BITMAPINFOHEADER createBitmapHeader(int width, int height) {
    BITMAPINFOHEADER  bi;

    bi.biSize=sizeof(BITMAPINFOHEADER);
    bi.biWidth=width;
    bi.biHeight=height;//-
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

    //save(compBitmap, hMemoryDC); //for screenshot
   

    // Cleanup
    //SelectObject(hMemoryDC, NULL); //needed?
   // DeleteObject(compBitmap);
    DeleteDC(hMemoryDC); //try without when adding video. what happens?
    ReleaseDC(NULL, hScreenDC);

    return compBitmap;

}
//not needed atm
void saveScreenShot(HBITMAP compBitmap, HDC hDC){ 

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
void appendNewFrame(PAVIFILE aviFile, HBITMAP compBitmap, LONG& frameNum){

     // Retrieve the bitmap data
    BITMAP bmp;
    GetObject(compBitmap, sizeof(BITMAP), &bmp);
    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    //getting avistream
    PAVISTREAM aviStream;
    if (AVIFileGetStream(aviFile, &aviStream, streamtypeVIDEO, 0) != AVIERR_OK) {
        std::cerr << "Failed to get AVI stream." << std::endl;
        return;
    }
      int frameSize = ((width * 24 + 31) / 32) * 4 * height;
    std::cerr << "avi stream gottem" << std::endl;

    BITMAPINFOHEADER biHeader=createBitmapHeader(width, height);
    BITMAPFILEHEADER fileHeader=createBitmapFileHeader(width, height, frameSize);
    BITMAPINFO bmpInfo = {0};
    bmpInfo.bmiHeader = biHeader;
  
    std::cerr << "bitmap headers readu for avistreamsetformat" << std::endl;
    if (AVIStreamSetFormat(aviStream, 0, &bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER)) != AVIERR_OK) {
        std::cerr << "Failed to set AVI stream format." << std::endl;
        AVIStreamRelease(aviStream);
        return;
    }
    std::cerr << "avistream format made" << std::endl;
    HRESULT hr = AVIStreamWrite(aviStream, frameNum, 1, compBitmap, sizeof(BITMAP), AVIIF_KEYFRAME, NULL, NULL);
    if (hr != 0) {
        std::cerr << "Failed to write to AVI stream. Error: " << hr << std::endl;
    } else {
        std::cerr << "bitmap copied to avi" << std::endl;
    }
    
    AVIStreamRelease(aviStream);

}

PAVIFILE createAviFile(){
    AVIFileInit();
    PAVIFILE aviFile;
    //wide-character string??
    if (AVIFileOpen(&aviFile, TEXT("svid.avi"), OF_WRITE | OF_CREATE, NULL) != AVIERR_OK) {
        std::cerr << "Failed to open AVI file." << std::endl;
        return nullptr;
    }
    //avi stream
    PAVISTREAM aviStream;
    AVISTREAMINFO streamInfo={0};
    ZeroMemory(&streamInfo, sizeof(streamInfo));

    streamInfo.fccType = streamtypeVIDEO;
    streamInfo.fccHandler = 0;
    streamInfo.dwScale = 1;
    streamInfo.dwRate = 25; // 25 frames per second
    streamInfo.dwSuggestedBufferSize = 0;
   
    if (AVIFileCreateStream(aviFile, &aviStream, &streamInfo) != AVIERR_OK) {
        std::cerr << "Failed to create AVI stream." << std::endl;
        AVIFileRelease(aviFile);
        return nullptr;
    }
    
  //  AVIStreamRelease(aviStream); // Release stream if it's not needed immediately
    std::cerr << "wavi file and stream created" << std::endl;
    return aviFile;
    
}

int main() {
    PAVIFILE aviFile= createAviFile();
    long i=0;
    while(i<40){
        HBITMAP compBitmap= captureScreen();
        appendNewFrame(aviFile, compBitmap, i );
        i++;
    }
AVIFileRelease(aviFile); // Release AVI file handle
AVIFileExit(); // Cleanup AVI file subsystem
std::cout << "Screenvideo saved as svid.avi" << std::endl;
 return 0;
}


