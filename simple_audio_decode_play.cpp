/** 
 * 最简单的基于FFmpeg的音频播放器 2  
 * Simplest FFmpeg Audio Player 2  
 * 
 * 雷霄骅 Lei Xiaohua 
 * leixiaohua1020@126.com 
 * 中国传媒大学/数字电视技术 
 * Communication University of China / Digital TV Technology 
 * http://blog.csdn.net/leixiaohua1020 
 * 
 * 本程序实现了音频的解码和播放。 
 * 是最简单的FFmpeg音频解码方面的教程。 
 * 通过学习本例子可以了解FFmpeg的解码流程。 
 * 
 * 该版本使用SDL 2.0替换了第一个版本中的SDL 1.0。 
 * 注意：SDL 2.0中音频解码的API并无变化。唯一变化的地方在于 
 * 其回调函数的中的Audio Buffer并没有完全初始化，需要手动初始化。 
 * 本例子中即SDL_memset(stream, 0, len); 
 * 
 * This software decode and play audio streams. 
 * Suitable for beginner of FFmpeg. 
 * 
 * This version use SDL 2.0 instead of SDL 1.2 in version 1 
 * Note:The good news for audio is that, with one exception,  
 * it's entirely backwards compatible with 1.2. 
 * That one really important exception: The audio callback  
 * does NOT start with a fully initialized buffer anymore.  
 * You must fully write to the buffer in all cases. In this  
 * example it is SDL_memset(stream, 0, len); 
 * 
 * Version 2.0 
 */  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
  
#define __STDC_CONSTANT_MACROS  
  
#ifdef _WIN32  
//Windows  
extern "C"  
{  
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libswresample/swresample.h"  
#include "SDL2/SDL.h"  
};  
#else  
//Linux...  
#ifdef __cplusplus  
extern "C"  
{  
#endif  
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#include <libswresample/swresample.h>  
#include "SDL2/SDL.h"
#ifdef __cplusplus  
};  
#endif  
#endif  
  
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  
  
//Output PCM  
#define OUTPUT_PCM 1 
//Use SDL  
#define USE_SDL 0  
  

#define ARR_LEGNTH(array,length){length=sizeof(array)/sizeof(array[0]);}

//Buffer:  
//|-----------|-------------|  
//chunk-------pos---len-----|  
static  Uint8  *audio_chunk;   
static  Uint32  audio_len;   
static  Uint8  *audio_pos;   
  
/* The audio function callback takes the following parameters:  
 * stream: A pointer to the audio buffer to be filled  
 * len: The length (in bytes) of the audio buffer  
*/   
void  fill_audio(void *udata,Uint8 *stream,int len){   
    //SDL 2.0  
    SDL_memset(stream, 0, len);  
    if(audio_len==0)  
        return;   
  
    len=(len>audio_len?audio_len:len);   /*  Mix  as  much  data  as  possible  */   
  
    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);  
    audio_pos += len;   
    audio_len -= len;   
}   
//-----------------  
  
  
int main(int argc, char* argv[])  
{  

    if(argc != 2){
	printf("argv's num error\n");
	return 0;
    }else{
	printf("%s\n",argv[0]);
	printf("%s\n",argv[1]);
   }
	
    AVFormatContext *pFormatCtx;  
    int             i, audioStream,videoStream;  
    AVCodecContext  *pCodecCtx;  
    AVCodec         *pCodec;  
    AVPacket        *packet;  
    uint8_t         *out_buffer;  
    AVFrame         *pFrame;  
    SDL_AudioSpec wanted_spec;  
    int ret;  
    uint32_t len = 0;  
    int got_picture;  
    int index = 0;  
    int64_t in_channel_layout;  
    
    struct SwrContext *au_convert_ctx;  
  
    FILE *pFile=NULL;  
	FILE *h264File=NULL;
    char *url = argv[1];
//    char url[]="Adele.mp3";  
//    char url[]="SABK90UMC_sd.avi";
  
	AVInputFormat *pInputFormat;
	AVStream **streams;

    av_register_all();//----------------------------------------------------------
    avformat_network_init();  
    pFormatCtx = avformat_alloc_context();  
    //Open  
    if(avformat_open_input(&pFormatCtx,url,NULL,NULL)!=0){  //----------------------------------------------------------
        printf("Couldn't open input stream.\n");  
        return -1;  
    }  


	printf("****************************************\n");
	pInputFormat = pFormatCtx->iformat;
	printf("name:%s\n",pInputFormat->name);
	printf("long_name:%s\n",pInputFormat->long_name);
	printf("extensions:%s\n",pInputFormat->extensions);
	printf("raw_codec_id:%s\n",pInputFormat->raw_codec_id);
	printf("****************************************\n");
	printf("nb_streams=%d\n",pFormatCtx->nb_streams);
	printf("****************************************\n");

	streams = pFormatCtx->streams;
	int streamsLength = pFormatCtx->nb_streams;
	//ARR_LEGNTH(pFormatCtx->streams,streamsLength);

	//printf("streamsLength=%d\n",streamsLength);
	for(int j=0;j<streamsLength;j++){
		printf("id=%d\n",streams[j]->id);
		//printf("codec=%s\n",streams[j]->codec);
		printf("time_base=%d\n",streams[j]->time_base);
		printf("r_frame_rate=%d\n",streams[j]->r_frame_rate);
	}
	printf("****************************************\n");
	printf("duration=%d\n",pFormatCtx->duration);
	printf("****************************************\n");
	printf("bit_rate=%d\n",pFormatCtx->bit_rate);
	printf("****************************************\n");

	
    // Retrieve stream information  
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){  //----------------------------------------------------------
        printf("Couldn't find stream information.\n");  
        return -1;  
    }  
    // Dump valid information onto standard error  
//    av_dump_format(pFormatCtx, 0, url, false); //print video/audio info
 
  
    // Find the first audio stream  
    audioStream=-1;  
videoStream = -1;
    for(i=0; i < pFormatCtx->nb_streams; i++){  
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){  
            audioStream=i;  
            break;  
        }


    }  

    for(i=0; i < pFormatCtx->nb_streams; i++){  

	if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
            videoStream=i;  
            break;  
        } 
    }
	
   


  
    if(audioStream==-1){  
        printf("Didn't find a audio stream.\n");  
        return -1;  
    } 

    if(videoStream==-1){  
        printf("Didn't find a video stream.\n");  
        return -1;  
    }   

    printf("audioStream=%d\n",audioStream);
    printf("videoStream=%d\n",videoStream);

printf("**************************************\n");

  
    // Get a pointer to the codec context for the audio stream  
    pCodecCtx=pFormatCtx->streams[audioStream]->codec;  
  
    // Find the decoder for the audio stream  
    //pCodec=avcodec_find_decoder(pCodecCtx->codec_id);

    //video 
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);    
    if(pCodec==NULL){  
        printf("Codec not found.\n");  
        return -1;  
    }  
  
    // Open codec  
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){  
        printf("Could not open codec.\n");  
        return -1;  
    }  

    /*printf("avcodecCtx channels%dx%d\n",pCodecCtx->width,pCodecCtx->height);
    printf("avcodecCtx channels  %s;  %s;  %d; %d;\n",pCodecCtx->codec->name,pCodecCtx->codec->long_name,pCodecCtx->codec->type,pCodecCtx->codec->id);*/

//return -1;

    printf("avcodecCtx sample_rate   %d\n",pCodecCtx->sample_rate);//ONLY AUDIO DATA
    printf("avcodecCtx channels   %d\n",pCodecCtx->channels);//ONLY AUDIO DATA
    printf("avcodecCtx sample_fmt   %d\n",pCodecCtx->sample_fmt);//ONLY AUDIO DATA
printf("end.........\n");
  
      
#if OUTPUT_PCM  
    pFile=fopen("output.pcm", "wb");  
	h264File=fopen("test.h264","wb");
#endif  

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));  
    av_init_packet(packet);  

  
    //Out Audio Param  
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;  
    //nb_samples: AAC-1024 MP3-1152  
    int out_nb_samples=pCodecCtx->frame_size;  
    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;  
    int out_sample_rate=44100;  
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);  
    //Out Buffer Size  
    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);  
  
    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);  
    pFrame=av_frame_alloc();  
//SDL------------------  
#if USE_SDL  
    //Init  
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {    
        printf( "Could not initialize SDL - %s\n", SDL_GetError());   
        return -1;  
    }  
    //SDL_AudioSpec  
    wanted_spec.freq = out_sample_rate;   
    wanted_spec.format = AUDIO_S16SYS;   
    wanted_spec.channels = out_channels;   
    wanted_spec.silence = 0;   
    wanted_spec.samples = out_nb_samples;   
    wanted_spec.callback = fill_audio;   
    wanted_spec.userdata = pCodecCtx;   
  
    if (SDL_OpenAudio(&wanted_spec, NULL)<0){   
        printf("can't open audio.\n");   
        return -1;   
    }   
#endif  

  
    //FIX:Some Codec's Context Information is missing  
    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);  
    //Swr  
  
    au_convert_ctx = swr_alloc();  
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,  
        in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);  
    swr_init(au_convert_ctx);  
  
    //Play  
    SDL_PauseAudio(0);  
//  printf("before while  %d\n",pFormatCtx->internal->packet_buffer);



    while(av_read_frame(pFormatCtx, packet)>=0){  
printf("-------------in while\n");
        if(packet->stream_index==audioStream){  
	
printf("-------------%d",packet->size);
	//fprintf(h264File,"%s\n",packet);
//	fwrite(packet->data,1,packet->size,h264File);

            ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);  
            if ( ret < 0 ) {  
                printf("Error in decoding audio frame.\n");  
                return -1;  
            }  
            if ( got_picture > 0 ){  
                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);  
#if 1  
                printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);  
#endif  
  
  
#if OUTPUT_PCM  
                //Write PCM  
		printf("will wirte info to PCM\n");
                fwrite(out_buffer, 1, out_buffer_size, pFile);  
#endif  
                index++;  
            }  
  
#if USE_SDL  
            while(audio_len>0)//Wait until finish  
                SDL_Delay(1);   
  
            //Set audio buffer (PCM data)  
            audio_chunk = (Uint8 *) out_buffer;   
            //Audio buffer length  
            audio_len =out_buffer_size;  
            audio_pos = audio_chunk;  
  
#endif  
        }  
        av_free_packet(packet);  
    }  
  printf("after while");

    swr_free(&au_convert_ctx);  
  
#if USE_SDL  
    SDL_CloseAudio();//Close SDL  
    SDL_Quit();  
#endif  
      
#if OUTPUT_PCM  
    fclose(pFile);  
#endif  
    av_free(out_buffer);  
    avcodec_close(pCodecCtx);  
    avformat_close_input(&pFormatCtx);  
  
    return 0;  
} 
