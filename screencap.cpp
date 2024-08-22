//#include "Gdiplus.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <conio.h> 

#include <cstdlib> 
#include <vfw.h> 
#include <string.h>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h> 
}

// Global flag to indicate whether to exit the loop
bool exitFlag = false;

void initializeInput(AVFormatContext** inputFormatContext,  AVCodecContext** inputCodecContext ){
     avdevice_register_all();

    const AVInputFormat* inputFormat = av_find_input_format("gdigrab");
    
    
    *inputFormatContext= avformat_alloc_context();
   if (avformat_open_input(inputFormatContext, "desktop", inputFormat, nullptr) != 0) {
        std::cerr << "Could not open input" << std::endl;
        exit(1);
    }


    //const AVCodec* codec = nullptr;
    AVStream* stream = (*inputFormatContext)->streams[0];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    *inputCodecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(*inputCodecContext, stream->codecpar);

    if (avcodec_open2(*inputCodecContext, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return;
    }

    // Manually set dimensions and frame rate 
    (*inputCodecContext)->bit_rate = 400000;
   (*inputCodecContext)->width = 1280;
    (*inputCodecContext)->height = 720;
    (*inputCodecContext)->time_base = AVRational{1, 25};
    (*inputCodecContext)->framerate = AVRational{25, 1};
    (*inputCodecContext)->gop_size = 10;
    (*inputCodecContext)->max_b_frames = 1;
    (*inputCodecContext)->pix_fmt = AV_PIX_FMT_YUV420P;
    
    std::cerr << "Input dimensions: " << (*inputCodecContext)->width << "x" << (*inputCodecContext)->height << std::endl;
    std::cerr << "Input pixel format: " << (*inputCodecContext)->pix_fmt << std::endl;
    std::cerr << "Input frame rate is " << av_q2d((*inputCodecContext)->time_base) << " (time_base: " << (*inputCodecContext)->time_base.num << "/" << (*inputCodecContext)->time_base.den << ")" << std::endl;
    std::cerr << "Input frame rate (stream) is " << av_q2d(stream->r_frame_rate) << " (r_frame_rate: " << stream->r_frame_rate.num << "/" << stream->r_frame_rate.den << ")" << std::endl;

}

void initializeOutput(AVFormatContext** outputFormatContext, AVCodecContext** outputCodecContext, AVCodecContext* inputCodecContext) {
    AVStream* outStream = nullptr;

    avformat_alloc_output_context2(outputFormatContext, nullptr, nullptr, "output_file.mp4");
    if (!*outputFormatContext) {
        std::cerr << "Could not create output context" << std::endl;
        return;
    }

    // Add a video stream to the output format context
    outStream = avformat_new_stream(*outputFormatContext, nullptr);
   
    const AVCodec* outputCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    
    *outputCodecContext = avcodec_alloc_context3(outputCodec);

    // Set  parameters for the output codec context
    (*outputCodecContext)->height = inputCodecContext->height;
    (*outputCodecContext)->width = inputCodecContext->width;
    (*outputCodecContext)->sample_aspect_ratio = inputCodecContext->sample_aspect_ratio;
    (*outputCodecContext)->pix_fmt =AV_PIX_FMT_YUV420P;
    (*outputCodecContext)->time_base = (AVRational){1, 25}; 

     std::cerr << "Output frame rate is " << av_q2d((*outputCodecContext)->time_base) << " (time_base: " << (*outputCodecContext)->time_base.num << "/" << (*outputCodecContext)->time_base.den << ")" << std::endl;

    outStream->time_base = (*outputCodecContext)->time_base; // Set the time base for the stream

    if ((*outputFormatContext)->oformat->flags & AVFMT_GLOBALHEADER)
        (*outputCodecContext)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(*outputCodecContext, outputCodec, nullptr) < 0) {
        std::cerr << "Could not open output codec" << std::endl;
        exit(1);
    }

    // Copy the codec parameters from the output codec context to the output stream
    if (avcodec_parameters_from_context(outStream->codecpar, *outputCodecContext) < 0) {
        std::cerr << "Could not copy codec parameters to output stream" << std::endl;
        return;
    }

    // Open output file
    if (!((*outputFormatContext)->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&(*outputFormatContext)->pb, "svid_file.mp4", AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file" << std::endl;
            return;
        }
    }

    if (avformat_write_header(*outputFormatContext, nullptr) < 0) {
        std::cerr << "Error occurred when opening output file" << std::endl;
        return;
    }
}


void encodeFrames(AVFormatContext* inputFormatContext, AVCodecContext* inputCodecContext, AVFormatContext* outputFormatContext,  AVCodecContext* outputCodecContext){
    
    AVFrame* frame=av_frame_alloc();
    AVPacket* packet=av_packet_alloc();
     if (!frame || !packet) {
        std::cerr << "Could not allocate frame or packet" << std::endl;
        exit(1);
    }

     frame->format = AV_PIX_FMT_YUV420P;
    frame->width = 1280;  
    frame->height = 720;  

    // Allocate buffer for the frame
    if (av_frame_get_buffer(frame, 32) < 0) {
        std::cerr << "Could not allocate the frame buffer" << std::endl;
        av_frame_free(&frame);
        av_packet_free(&packet);
        exit(1);
    }

     // Allocate the output frame (the frame that will be encoded)
    AVFrame* outputFrame = av_frame_alloc();
     if (!outputFrame) {
        std::cerr << "Could not allocate output frame" << std::endl;
        exit(1);
    }
    outputFrame->width = outputCodecContext->width;
    outputFrame->height = outputCodecContext->height;
    outputFrame->format = outputCodecContext->pix_fmt;

    // Allocate buffer for the output frame
    int ret = av_frame_get_buffer(outputFrame, 32);  
    if (ret < 0) {
        std::cerr << "Could not allocate output frame buffer" << std::endl;
        av_frame_free(&frame);
        av_frame_free(&outputFrame);
        av_packet_free(&packet);
        exit(1);
    }

    SwsContext* swsContext = nullptr;

    int64_t frameIndex = 0; //for frame calculation

    while (!exitFlag && av_read_frame(inputFormatContext, packet) >= 0) {

        if (packet->stream_index == 0) {
            int ret = avcodec_send_packet(inputCodecContext, packet);
            std::cerr << " send packet ret: " << ret << std::endl;

            if (ret < 0) {
               std::cerr << "Error sending packet to decoder: "  << std::endl;
                av_packet_unref(packet);
                continue;
            }

            ret = avcodec_receive_frame(inputCodecContext, frame);
            //std::cerr << "receive frame ret: " << ret << std::endl;

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(packet);
                continue;
            } else if (ret < 0) {
                std::cerr << "Error receiving frame from decoder" << std::endl;
                break;
            }

            if (!swsContext) {
                // Initialize SwsContext after receiving the first frame
                swsContext = sws_getContext(
                    frame->width, frame->height, (AVPixelFormat)frame->format,
                    outputFrame->width, outputFrame->height, (AVPixelFormat)outputFrame->format,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);

                if (!swsContext) {
                    std::cerr << "Could not initialize the conversion context" << std::endl;
                    exit(1);
                }
            }
           
            // Scale/convert the frame to the output frame
            sws_scale(swsContext, frame->data, frame->linesize, 0, inputCodecContext->height, 
                 outputFrame->data, outputFrame->linesize);

            outputFrame->pts = frameIndex++;

            // Send the scaled frame to the encoder
            ret = avcodec_send_frame(outputCodecContext, outputFrame);
           

            if (ret < 0) {
                 std::cerr << "Error sending frame for encoding: " << std::endl;
                break;
            }

            while (ret >= 0) {
                AVPacket* outPacket=av_packet_alloc();
              
                ret = avcodec_receive_packet(outputCodecContext, outPacket);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error receiving packet from encoder" << std::endl;
                    break;
                }

                  // Check if the packet contains data
                if (outPacket->size <= 0) {
                    std::cerr << "Received empty packet" << std::endl;
                    av_packet_free(&outPacket);
                    continue;
                } 

                av_packet_rescale_ts(outPacket, outputCodecContext->time_base, outputFormatContext->streams[0]->time_base);

                ret = av_interleaved_write_frame(outputFormatContext, outPacket);
                av_packet_free(&outPacket);
                if (ret < 0) {
                    std::cerr << "Error writing frame to output" << std::endl;
                    break;
                }
            }
        }

        av_packet_unref(packet);
         // Check if Enter key has been pressed
        if (_kbhit() && _getch() == 13) { // 13 is the Enter key
            exitFlag = true;
        }
    }
    avcodec_send_frame(outputCodecContext, nullptr);
    av_frame_free(&frame);
    sws_freeContext(swsContext);
}




int main() {
   
    AVFormatContext* inputFormatContext=nullptr;
    AVCodecContext* inputCodecContext=nullptr;

    AVFormatContext* outputFormatContext=nullptr;
    AVCodecContext* outputCodecContext=nullptr;

    avdevice_register_all();
    

    initializeInput(&inputFormatContext, &inputCodecContext);
    initializeOutput(&outputFormatContext, &outputCodecContext, inputCodecContext);
    std::cerr << "Initialization done, starting encoding" << std::endl;

    encodeFrames(inputFormatContext, inputCodecContext,  outputFormatContext,  outputCodecContext);
     av_write_trailer(outputFormatContext);
    //cleanup
     //av_frame_free(&frame);
    avcodec_free_context(&inputCodecContext);
    avcodec_free_context(&outputCodecContext);
    avformat_close_input(&inputFormatContext);
    avformat_free_context(outputFormatContext);
    return 0;
}


