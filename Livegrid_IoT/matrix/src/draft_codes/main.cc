// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// This code is public domain
// (but note, once linked against the led-matrix library, this is
// covered by the GPL v2)
//
// This is a grab-bag of various demos and not very readable.
// Ancient AV versions forgot to set this.
#define __STDC_CONSTANT_MACROS        //video viewer

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "pixel-mapper.h"
#include "graphics.h"

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include "FastNoise.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

//image viewer
#include "content-streamer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <map>
#include <string>

#include <Magick++.h>
#include <magick/image.h>

//video viewer
// libav: "U NO extern C in header ?"
extern "C" {
#  include <libavcodec/avcodec.h>
#  include <libavformat/avformat.h>
#  include <libavutil/imgutils.h>
#  include <libswscale/swscale.h>
}
#include <time.h>

//Text Viewer
//empty

using namespace rgb_matrix;
using namespace std;

//image viewer
using rgb_matrix::GPIO;
using rgb_matrix::Canvas;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamReader;
using rgb_matrix::StreamWriter;
using rgb_matrix::StreamIO;

std::string boardSize = "S";  //"S", "M","L","XL"

int baseR = 255;
int baseG = 255;
int baseB = 255;
float globalBrightness = 0.0f;
float scale = 5.0f;
float speed = 1.0f;
float contrast = 50.0f;
float k = 0.0f; 
float contrastFactor;

int demo = -1;
#define PatternDemo 0
#define ImageDemo 1
#define VideoDemo 2
#define TextDemo 3
// enum {PatternDemo, ImageDemo, VideoDemo, TextDemo};

int noiseType = 0; 
#define SimplexType 0
#define CellularType 1
#define CubicType 2
// enum {Simplex, Cellular, Cubic};

std::string imageList[] = {"images/1.jpeg", "images/2.jpeg", "images/3.jpeg", "images/4.jpeg", "images/5.jpeg", "images/6.jpeg"};
std::string videoList[] = {"videos/1.webm", "videos/2.webm", "videos/3.webm", "videos/4.webm"};

int selectedImageIndex = 0;
int selectedVideoIndex = 0;

std::string textContent;
int textFontSize = 1;  
#define Small 0
#define Medium 1
#define Large 2
std::string fontTypeList[] = {"font1.bdf", "font2.bdf", "font3.bdf", "font4.bdf", "font5.bdf", "font6.bdf"};
int textFontType = 1;   // total 6 types
int textRed = 100;      
int textGreen = 100;
int textBlue = 100;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static void parseJson()
{
  /*1.  json parsing
      get demo value from json values, demo=0,1,2,..  0:patterns, 1:images, 2:videos, 3:text
      get baseR,baseG...
  */
  
  printf("First parsing");   
  std::ifstream json_file("/data/data.json", std::ifstream::binary);
  std::ostringstream  ss;
  ss << json_file.rdbuf(); // reading data
  std::string stringJson = ss.str();
  //  printf("data.json: %s",stringJson);

  stringstream s_stream(stringJson); //create string stream from the string
  const unsigned int BUFFERSIZE = 256;
  //temporary buffer
  char buffer[BUFFERSIZE];
  memset(buffer, 0, BUFFERSIZE * sizeof(char));

  vector<string> result;
  
  while(s_stream.good()) {
    string substr;
    getline(s_stream, substr, ','); //get first string delimited by comma
    result.push_back(substr);
  }

  for(int i = 0; i<static_cast<int>(result.size()); i++) {
    if(i == 0) {  //boardSize
      boardSize = result.at(0).substr(result.at(0).find(":") + 1);
    } 
    else if(i == 1) {
      demo = atoi(result.at(1).substr(result.at(1).find(":") + 1).c_str());
    } 
    else if(i == 2) {
      noiseType = atoi(result.at(2).substr(result.at(2).find(":") + 1).c_str());
    }
    else if(i == 3) {
      // printf("%s \n", result.at(0).substr(result.at(0).find(":") + 1).c_str());
      speed = atof(result.at(3).substr(result.at(3).find(":") + 1).c_str())/25;
    }
    else if(i == 4) {
      // printf("%s \n", result.at(1).substr(result.at(1).find(":") + 1).c_str());
      scale = atof(result.at(4).substr(result.at(4).find(":") + 1).c_str())/10;
    }
    else if(i == 5) {
      // printf("%s \n", result.at(2).substr(result.at(2).find(":") + 1).c_str());
      baseR = atoi(result.at(5).substr(result.at(5).find(":") + 1).c_str());
    }      
    else if(i == 6) {
      // printf("%s \n", result.at(3).substr(result.at(3).find(":") + 1).c_str());
      baseG = atoi(result.at(6).substr(result.at(6).find(":") + 1).c_str());
    }
    else if(i == 7) {
      // printf("%s \n", result.at(4).substr(result.at(4).find(":") + 1).c_str());
      baseB = atoi(result.at(7).substr(result.at(7).find(":") + 1).c_str());
    }
    else if(i == 8) {	
      // printf("%s \n",  result.at(5).substr(result.at(5).find(":") + 1).c_str());
      globalBrightness = atof(result.at(8).substr(result.at(8).find(":") + 1).c_str());        
    }
    else if(i == 9) {
      string a = result.at(9).substr(result.at(9).find(":") + 1);
      // printf("%s \n",  a.substr(0,a.size()-1).c_str());
      contrast = atof(a.substr(0,a.size()-1).c_str());
      contrastFactor = ((259.0f*(contrast+255.0f))/(255.0f*(259.0f-contrast)));
    }
    else if(i == 10) {	
      selectedImageIndex = atoi(result.at(10).substr(result.at(10).find(":") + 1).c_str());
    }
    else if(i == 11) {	
      selectedVideoIndex = atoi(result.at(11).substr(result.at(11).find(":") + 1).c_str());
    }
    else if(i == 12) {	
      textContent = result.at(12).substr(result.at(12).find(":") + 1);
    }
    else if(i == 13) {	
      textFontSize = atoi(result.at(13).substr(result.at(13).find(":") + 1).c_str());
    }
    else if(i == 14) {	
      textFontType = atoi(result.at(14).substr(result.at(14).find(":") + 1).c_str());
    }
    else if(i == 15) {	
      textRed = atoi(result.at(15).substr(result.at(15).find(":") + 1).c_str());
    }
    else if(i == 16) {	
      textGreen = atoi(result.at(16).substr(result.at(16).find(":") + 1).c_str());
    }
    else if(i == 17) {	
      textBlue = atoi(result.at(17).substr(result.at(17).find(":") + 1).c_str());
    }    
  }

  // printf("speed: %f \n",  speed);
  // printf("scale: %f \n",  scale);
  // printf("globalBrightness: %f \n",  globalBrightness);
  // printf("baseR: %d \n",  baseR);
  // printf("baseG: %d \n",  baseG);
  // printf("baseB: %d \n",  baseB);
  // printf("contrast: %f \n",  contrast);
}

//----------------------Text Display------------------------------------------


static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

static bool FullSaturation(const Color &c) {
    return (c.r == 0 || c.r == 255)
        && (c.g == 0 || c.g == 255)
        && (c.b == 0 || c.b == 255);
}

static int text_usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads text from stdin and displays it. "
          "Empty string: clear screen\n");
  fprintf(stderr, "Options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  fprintf(stderr,
          "\t-f <font-file>    : Use given font.\n"
          "\t-b <brightness>   : Sets brightness percent. Default: 100.\n"
          "\t-x <x-origin>     : X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin>     : Y-Origin of displaying text (Default: 0)\n"
          "\t-S <spacing>      : Spacing pixels between letters (Default: 0)\n"
          "\t-C <r,g,b>        : Color. Default 255,255,0\n"
          "\t-B <r,g,b>        : Font Background-Color. Default 0,0,0\n"
          "\t-O <r,g,b>        : Outline-Color, e.g. to increase contrast.\n"
          "\t-F <r,g,b>        : Panel flooding-background color. Default 0,0,0\n"
          );
  return 1;
}

static int TextViewer() {
  int argc = 5;
  std::string fontPathStr = "../fonts/";
  if(textFontSize == Small) fontPathStr += "S/";
  else if(textFontSize == Medium) fontPathStr += "M/";
  else if(textFontSize == Large) fontPathStr += "L/";
  fontPathStr += fontTypeList[textFontType];

  std::string colorStr = std::to_string(textRed) + "," + std::to_string(textGreen) + "," + std::to_string(textBlue);
  const char *argv[] = { "./demo", "-f", fontPathStr.c_str(), "-C", colorStr.c_str()};
// sudo ./text-example -f ../fonts/8x13.bdf -C 255,255,0

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, (char***)&argv,
                                         &matrix_options, &runtime_opt)) {
    return text_usage(argv[0]);
  }

  if(boardSize == "S") {
    matrix_options.rows = 16;
    matrix_options.cols = 32;
  } else if(boardSize == "M") {
    matrix_options.rows = 32;
    matrix_options.cols = 32;
  } else if(boardSize == "L") {
    matrix_options.rows = 32;
    matrix_options.cols = 64;
  } else if(boardSize == "XL") {
    matrix_options.rows = 48;
    matrix_options.cols = 96;
  }


  Color color(255, 255, 0);
  Color bg_color(0, 0, 0);
  Color flood_color(0, 0, 0);
  Color outline_color(0,0,0);
  bool with_outline = false;

  const char *bdf_font_file = NULL;
  int x_orig = 0;
  int y_orig = 0;
  int brightness = 100;
  int letter_spacing = 0;

  int opt;
  while ((opt = getopt(argc, const_cast<char* const *>(argv), "x:y:f:C:B:O:b:S:F:")) != -1) {
    switch (opt) {
    case 'b': brightness = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'S': letter_spacing = atoi(optarg); break;
    case 'C':
      if (!parseColor(&color, optarg)) {
        fprintf(stderr, "Invalid color spec: %s\n", optarg);
        return text_usage(argv[0]);
      }
      break;
    case 'B':
      if (!parseColor(&bg_color, optarg)) {
        fprintf(stderr, "Invalid background color spec: %s\n", optarg);
        return text_usage(argv[0]);
      }
      break;
    case 'O':
      if (!parseColor(&outline_color, optarg)) {
        fprintf(stderr, "Invalid outline color spec: %s\n", optarg);
        return text_usage(argv[0]);
      }
      with_outline = true;
      break;
    case 'F':
      if (!parseColor(&flood_color, optarg)) {
        fprintf(stderr, "Invalid background color spec: %s\n", optarg);
        return text_usage(argv[0]);
      }
      break;
    default:
      return text_usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return text_usage(argv[0]);
  }

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  rgb_matrix::Font font;
  if (!font.LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return 1;
  }

  /*
   * If we want an outline around the font, we create a new font with
   * the original font as a template that is just an outline font.
   */
  rgb_matrix::Font *outline_font = NULL;
  if (with_outline) {
      outline_font = font.CreateOutlineFont();
  }

  if (brightness < 1 || brightness > 100) {
    fprintf(stderr, "Brightness is outside usable range.\n");
    return 1;
  }

  RGBMatrix *canvas = rgb_matrix::CreateMatrixFromOptions(matrix_options,
                                                          runtime_opt);
  if (canvas == NULL)
    return 1;

  canvas->SetBrightness(brightness);

  const bool all_extreme_colors = (brightness == 100)
      && FullSaturation(color)
      && FullSaturation(bg_color)
      && FullSaturation(outline_color);
  if (all_extreme_colors)
    canvas->SetPWMBits(1);

  const int x = x_orig;
  int y = y_orig;

  if (isatty(STDIN_FILENO)) {
    // Only give a message if we are interactive. If connected via pipe, be quiet
    printf("Enter lines. Full screen or empty line clears screen.\n"
           "Supports UTF-8. CTRL-D for exit.\n");
  }

  canvas->Fill(flood_color.r, flood_color.g, flood_color.b);

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);
  int curTextFontSize = textFontSize;
  int curTextFontType = textFontType;
  int curTextRed = textRed;
  int curTextGreen = textGreen;
  int curTextBlue = textBlue;

  char line[1024];
  while ((demo == TextDemo) && (curTextFontSize == textFontSize) && (curTextFontType == textFontType) && (curTextRed == textRed) && (curTextGreen == textGreen) && (curTextBlue == textBlue) && !interrupt_received) {
    strcpy(line, textContent.c_str());
    // fgets(line, sizeof(line), stdin)
    const size_t last = strlen(line);
    if (last > 0) line[last - 1] = '\0';  // remove newline.
    bool line_empty = strlen(line) == 0;
    if ((y + font.height() > canvas->height()) || line_empty) {
      canvas->Fill(flood_color.r, flood_color.g, flood_color.b);
      y = y_orig;
    }
    if (line_empty)
      continue;
    if (outline_font) {
      // The outline font, we need to write with a negative (-2) text-spacing,
      // as we want to have the same letter pitch as the regular text that
      // we then write on top.
      rgb_matrix::DrawText(canvas, *outline_font,
                           x - 1, y + font.baseline(),
                           outline_color, &bg_color, line, letter_spacing - 2);
    }
    // The regular text. Unless we already have filled the background with
    // the outline font, we also fill the background here.
    rgb_matrix::DrawText(canvas, font, x, y + font.baseline(),
                         color, outline_font ? NULL : &bg_color, line,
                         letter_spacing);
    y += font.height();

    parseJson();
  }

  // Finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}

//----------------------Video Viewer------------------------------------------
struct LedPixel {
  uint8_t r, g, b;
};
void CopyFrame(AVFrame *pFrame, FrameCanvas *canvas,
               int offset_x, int offset_y,
               int width, int height) {
  for (int y = 0; y < height; ++y) {
    LedPixel *pix = (LedPixel*) (pFrame->data[0] + y*pFrame->linesize[0]);
    for (int x = 0; x < width; ++x, ++pix) {
      canvas->SetPixel(x + offset_x, y + offset_y, pix->r, pix->g, pix->b);
    }
  }
}

// Scale "width" and "height" to fit within target rectangle of given size.
void ScaleToFitKeepAscpet(int fit_in_width, int fit_in_height,
                          int *width, int *height) {
  if (*height < fit_in_height && *width < fit_in_width) return; // Done.
  const float height_ratio = 1.0 * (*height) / fit_in_height;
  const float width_ratio  = 1.0 * (*width) / fit_in_width;
  const float ratio = (height_ratio > width_ratio) ? height_ratio : width_ratio;
  *width = roundf(*width / ratio);
  *height = roundf(*height / ratio);
}

static void add_nanos(struct timespec *accumulator, long nanoseconds) {
  accumulator->tv_nsec += nanoseconds;
  while (accumulator->tv_nsec > 1000000000) {
    accumulator->tv_nsec -= 1000000000;
    accumulator->tv_sec += 1;
  }
}

// Convert deprecated color formats to new and manually set the color range.
// YUV has funny ranges (16-235), while the YUVJ are 0-255. SWS prefers to
// deal with the YUV range, but then requires to set the output range.
// https://libav.org/documentation/doxygen/master/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5
SwsContext *CreateSWSContext(const AVCodecContext *codec_ctx,
                             int display_width, int display_height) {
  AVPixelFormat pix_fmt;
  bool src_range_extended_yuvj = true;
  // Remap deprecated to new pixel format.
  switch (codec_ctx->pix_fmt) {
  case AV_PIX_FMT_YUVJ420P: pix_fmt = AV_PIX_FMT_YUV420P; break;
  case AV_PIX_FMT_YUVJ422P: pix_fmt = AV_PIX_FMT_YUV422P; break;
  case AV_PIX_FMT_YUVJ444P: pix_fmt = AV_PIX_FMT_YUV444P; break;
  case AV_PIX_FMT_YUVJ440P: pix_fmt = AV_PIX_FMT_YUV440P; break;
  default:
    src_range_extended_yuvj = false;
    pix_fmt = codec_ctx->pix_fmt;
  }
  SwsContext *swsCtx = sws_getContext(codec_ctx->width, codec_ctx->height,
                                      pix_fmt,
                                      display_width, display_height,
                                      AV_PIX_FMT_RGB24, SWS_BILINEAR,
                                      NULL, NULL, NULL);
  if (src_range_extended_yuvj) {
    // Manually set the source range to be extended. Read modify write.
    int dontcare[4];
    int src_range, dst_range;
    int brightness, contrast, saturation;
    sws_getColorspaceDetails(swsCtx, (int**)&dontcare, &src_range,
                             (int**)&dontcare, &dst_range, &brightness,
                             &contrast, &saturation);
    const int* coefs = sws_getCoefficients(SWS_CS_DEFAULT);
    src_range = 1;  // New src range.
    sws_setColorspaceDetails(swsCtx, coefs, src_range, coefs, dst_range,
                             brightness, contrast, saturation);
  }
  return swsCtx;
}

static int video_usage(const char *progname, const char *msg = nullptr) {
  if (msg) {
    fprintf(stderr, "%s\n", msg);
  }
  fprintf(stderr, "Show one or a sequence of video files on the RGB-Matrix\n");
  fprintf(stderr, "usage: %s [options] <video> [<video>...]\n", progname);
  fprintf(stderr, "Options:\n"
          "\t-F                 : Full screen without black bars; aspect ratio might suffer\n"
          "\t-O<streamfile>     : Output to stream-file instead of matrix (don't need to be root).\n"
          "\t-s <count>         : Skip these number of frames in the beginning.\n"
          "\t-c <count>         : Only show this number of frames (excluding skipped frames).\n"
          "\t-V<vsync-multiple> : Instead of native video framerate, playback framerate\n"
          "\t                     is a fraction of matrix refresh. In particular with a stable refresh,\n"
          "\t                     this can result in more smooth playback. Choose multiple for desired framerate.\n"
          "\t                     (Tip: use --led-limit-refresh for stable rate)\n"
          "\t-v                 : verbose; prints video metadata and other info.\n"
          "\t-f                 : Loop forever.\n");

  fprintf(stderr, "\nGeneral LED matrix options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

static int VideoViewer()
{
  //needtouch, making argc and argv
  int argc = 3;
  char *argv[] = {"./demo", "-f", const_cast<char*>(videoList[selectedVideoIndex].c_str())};

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, (char***)&argv,
                                         &matrix_options, &runtime_opt)) {
    return video_usage(argv[0]);
  }

  if(boardSize == "S") {
    matrix_options.rows = 16;
    matrix_options.cols = 32;
  } else if(boardSize == "M") {
    matrix_options.rows = 32;
    matrix_options.cols = 32;
  } else if(boardSize == "L") {
    matrix_options.rows = 32;
    matrix_options.cols = 64;
  } else if(boardSize == "XL") {
    matrix_options.rows = 48;
    matrix_options.cols = 96;
  }

  matrix_options.chain_length = 4;
  matrix_options.parallel = 3;
  matrix_options.multiplexing = 2;
  matrix_options.hardware_mapping = "adafruit-hat";

  runtime_opt.gpio_slowdown = 2;  

  int vsync_multiple = 1;
  bool use_vsync_for_frame_timing = false;
  bool maintain_aspect_ratio = true;
  bool verbose = false;
  bool forever = false;
  int stream_output_fd = -1;
  unsigned int frame_skip = 0;
  unsigned int framecount_limit = UINT_MAX;  // even at 60fps, that is > 2yrs

  int opt;
  while ((opt = getopt(argc, argv, "vO:R:Lfc:s:FV:")) != -1) {
    switch (opt) {
      case 'v':
        verbose = true;
        break;
      case 'f':
        forever = true;
        break;
      case 'O':
        stream_output_fd = open(optarg, O_CREAT|O_TRUNC|O_WRONLY, 0644);
        if (stream_output_fd < 0) {
          perror("Couldn't open output stream");
          return 1;
        }
        break;
      case 'L':
        fprintf(stderr, "-L is deprecated. Use\n\t--led-pixel-mapper=\"U-mapper\" --led-chain=4\ninstead.\n");
        return 1;
        break;
      case 'R':
        fprintf(stderr, "-R is deprecated. "
                "Use --led-pixel-mapper=\"Rotate:%s\" instead.\n", optarg);
        return 1;
        break;
      case 'c':
        framecount_limit = atoi(optarg);
        break;
      case 's':
        frame_skip = atoi(optarg);
        break;
      case 'F':
        maintain_aspect_ratio = false;
        break;
      case 'V':
        vsync_multiple = atoi(optarg);
        if (vsync_multiple <= 0)
          return video_usage(argv[0],
                      "-V: VSync-multiple needs to be a positive integer");
        use_vsync_for_frame_timing = true;
        break;
      default:
        return video_usage(argv[0]);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Expected video filename.\n");
    return video_usage(argv[0]);
  }

  const bool multiple_videos = (argc > optind + 1);

  // We want to have the matrix start unless we actually write to a stream.
  runtime_opt.do_gpio_init = (stream_output_fd < 0);
  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL) {
    return 1;
  }
  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();

  long frame_count = 0;
  StreamIO *stream_io = NULL;
  StreamWriter *stream_writer = NULL;
  if (stream_output_fd >= 0) {
    stream_io = new rgb_matrix::FileStreamIO(stream_output_fd);
    stream_writer = new StreamWriter(stream_io);
    if (forever) {
      fprintf(stderr, "-f (forever) doesn't make sense with -O; disabling\n");
      forever = false;
    }
  }

  // If we only have to loop a single video, we can avoid doing the
  // expensive video stream set-up and just repeat in an inner loop.
  const bool one_video_forever = forever && !multiple_videos;
  const bool multiple_video_forever = forever && multiple_videos;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif
  avformat_network_init();

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  int curSelectedVideoIndex = selectedVideoIndex;  //added new

  do {
    for (int m = optind; m < argc && (demo == VideoDemo) && (curSelectedVideoIndex==selectedVideoIndex) && !interrupt_received; ++m) {
      const char *movie_file = argv[m];
      if (strcmp(movie_file, "-") == 0) {
        movie_file = "/dev/stdin";
      }

      AVFormatContext *format_context = avformat_alloc_context();
      if (avformat_open_input(&format_context, movie_file, NULL, NULL) != 0) {
        perror("Issue opening file: ");
        return -1;
      }

      if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Couldn't find stream information\n");
        return -1;
      }

      if (verbose) av_dump_format(format_context, 0, movie_file, 0);

      // Find the first video stream
      int videoStream = -1;
      AVCodecParameters *codec_parameters = nullptr;
      AVCodec *av_codec = nullptr;
      for (int i = 0; i < (int)format_context->nb_streams; ++i) {
        codec_parameters = format_context->streams[i]->codecpar;
        av_codec = avcodec_find_decoder(codec_parameters->codec_id);
        if (!av_codec) continue;
        if (codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
          videoStream = i;
          break;
        }
      }
      if (videoStream == -1)
        return false;

      // Frames per second; calculate wait time between frames.
      AVStream *const stream = format_context->streams[videoStream];
      AVRational rate = av_guess_frame_rate(format_context, stream, nullptr);
      const long frame_wait_nanos = 1e9 * rate.den / rate.num;
      if (verbose) fprintf(stderr, "FPS: %f\n", 1.0*rate.num / rate.den);

      AVCodecContext *codec_context = avcodec_alloc_context3(av_codec);
      if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0)
        return -1;
      if (avcodec_open2(codec_context, av_codec, NULL) < 0)
        return -1;

      /*
       * Prepare frame to hold the scaled target frame to be send to matrix.
       */
      int display_width = codec_context->width;
      int display_height = codec_context->height;
      if (maintain_aspect_ratio) {
        display_width = codec_context->width;
        display_height = codec_context->height;
        // Make display fit within canvas.
        ScaleToFitKeepAscpet(matrix->width(), matrix->height(),
                             &display_width, &display_height);
      } else {
        display_width = matrix->width();
        display_height = matrix->height();
      }
      // Letterbox or pillarbox black bars.
      const int display_offset_x = (matrix->width() - display_width)/2;
      const int display_offset_y = (matrix->height() - display_height)/2;

      // The output_frame_ will receive the scaled result.
      AVFrame *output_frame = av_frame_alloc();
      if (av_image_alloc(output_frame->data, output_frame->linesize,
                         display_width, display_height, AV_PIX_FMT_RGB24,
                         64) < 0) {
        return -1;
      }

      if (verbose) {
        fprintf(stderr, "Scaling %dx%d -> %dx%d; black border x:%d y:%d\n",
                codec_context->width, codec_context->height,
                display_width, display_height,
                display_offset_x, display_offset_y);
      }

      // initialize SWS context for software scaling
      SwsContext *const sws_ctx = CreateSWSContext(
        codec_context, display_width, display_height);
      if (!sws_ctx) {
        fprintf(stderr, "Trouble doing scaling to %dx%d :(\n",
                matrix->width(), matrix->height());
        return 1;
      }


      struct timespec next_frame;

      AVPacket *packet = av_packet_alloc();
      AVFrame *decode_frame = av_frame_alloc();  // Decode video into this
      do {
        unsigned int frames_left = framecount_limit;
        unsigned int frames_to_skip = frame_skip;
        if (one_video_forever) {
          av_seek_frame(format_context, videoStream, 0, AVSEEK_FLAG_ANY);
          avcodec_flush_buffers(codec_context);
        }
        clock_gettime(CLOCK_MONOTONIC, &next_frame);
        while ((demo == VideoDemo) && (curSelectedVideoIndex==selectedVideoIndex) && !interrupt_received && av_read_frame(format_context, packet) >= 0
               && frames_left > 0) {
          // Is this a packet from the video stream?
          if (packet->stream_index == videoStream) {
            // Determine absolute end of this frame now so that we don't include
            // decoding overhead. TODO: skip frames if getting too slow ?
            add_nanos(&next_frame, frame_wait_nanos);

            // Decode video frame
            if (avcodec_send_packet(codec_context, packet) < 0)
              continue;

            if (avcodec_receive_frame(codec_context, decode_frame) < 0)
              continue;

            if (frames_to_skip) { frames_to_skip--; continue; }

            // Convert the image from its native format to RGB
            sws_scale(sws_ctx, (uint8_t const * const *)decode_frame->data,
                      decode_frame->linesize, 0, codec_context->height,
                      output_frame->data, output_frame->linesize);
            CopyFrame(output_frame, offscreen_canvas,
                      display_offset_x, display_offset_y,
                      display_width, display_height);
            frame_count++;
            frames_left--;
            if (stream_writer) {
              if (verbose) fprintf(stderr, "%6ld", frame_count);
              stream_writer->Stream(*offscreen_canvas, frame_wait_nanos/1000);
            } else {
              offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas,
                                                     vsync_multiple);
            }
          }
          if (!stream_writer && !use_vsync_for_frame_timing) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_frame, NULL);
          }
          av_packet_unref(packet);

          parseJson();
        }
      } while ((demo == VideoDemo) && (curSelectedVideoIndex==selectedVideoIndex) && one_video_forever && !interrupt_received);

      av_packet_free(&packet);

      av_frame_free(&output_frame);
      av_frame_free(&decode_frame);
      avcodec_close(codec_context);
      avformat_close_input(&format_context);
    }
  } while ((demo == VideoDemo) && (curSelectedVideoIndex==selectedVideoIndex) && multiple_video_forever && !interrupt_received);

  if (interrupt_received) {
    // Feedback for Ctrl-C, but most importantly, force a newline
    // at the output, so that commandline-shell editing is not messed up.
    fprintf(stderr, "Got interrupt. Exiting\n");
  }

  delete matrix;
  delete stream_writer;
  delete stream_io;
  fprintf(stderr, "Total of %ld frames decoded\n", frame_count);

  return 0;
}

//--------------------Image Viewer -------------------------------------------

typedef int64_t tmillis_t;
static const tmillis_t distant_future = (1LL<<40); // that is a while.

struct ImageParams {
  ImageParams() : anim_duration_ms(distant_future), wait_ms(1500),
                  anim_delay_ms(-1), loops(-1), vsync_multiple(1) {}
  tmillis_t anim_duration_ms;  // If this is an animation, duration to show.
  tmillis_t wait_ms;           // Regular image: duration to show.
  tmillis_t anim_delay_ms;     // Animation delay override.
  int loops;
  int vsync_multiple;
};

struct FileInfo {
  ImageParams params;      // Each file might have specific timing settings
  bool is_multi_frame;
  rgb_matrix::StreamIO *content_stream;
};

static tmillis_t GetTimeInMillis() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void SleepMillis(tmillis_t milli_seconds) {
  if (milli_seconds <= 0) return;
  struct timespec ts;
  ts.tv_sec = milli_seconds / 1000;
  ts.tv_nsec = (milli_seconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

static void StoreInStream(const Magick::Image &img, int delay_time_us,
                          bool do_center,
                          rgb_matrix::FrameCanvas *scratch,
                          rgb_matrix::StreamWriter *output) {
  scratch->Clear();
  const int x_offset = do_center ? (scratch->width() - img.columns()) / 2 : 0;
  const int y_offset = do_center ? (scratch->height() - img.rows()) / 2 : 0;
  for (size_t y = 0; y < img.rows(); ++y) {
    for (size_t x = 0; x < img.columns(); ++x) {
      const Magick::Color &c = img.pixelColor(x, y);
      if (c.alphaQuantum() < 256) {
        scratch->SetPixel(x + x_offset, y + y_offset,
                          ScaleQuantumToChar(c.redQuantum()),
                          ScaleQuantumToChar(c.greenQuantum()),
                          ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
  output->Stream(*scratch, delay_time_us);
}

static void CopyStream(rgb_matrix::StreamReader *r,
                       rgb_matrix::StreamWriter *w,
                       rgb_matrix::FrameCanvas *scratch) {
  uint32_t delay_us;
  while (r->GetNext(scratch, &delay_us)) {
    w->Stream(*scratch, delay_us);
  }
}

// Load still image or animation.
// Scale, so that it fits in "width" and "height" and store in "result".
static bool LoadImageAndScale(const char *filename,
                              int target_width, int target_height,
                              bool fill_width, bool fill_height,
                              std::vector<Magick::Image> *result,
                              std::string *err_msg) {
  std::vector<Magick::Image> frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception& e) {
    if (e.what()) *err_msg = e.what();
    return false;
  }
  if (frames.size() == 0) {
    fprintf(stderr, "No image found.");
    return false;
  }

  // Put together the animation from single frames. GIFs can have nasty
  // disposal modes, but they are handled nicely by coalesceImages()
  if (frames.size() > 1) {
    Magick::coalesceImages(result, frames.begin(), frames.end());
  } else {
    result->push_back(frames[0]);   // just a single still image.
  }

  const int img_width = (*result)[0].columns();
  const int img_height = (*result)[0].rows();
  const float width_fraction = (float)target_width / img_width;
  const float height_fraction = (float)target_height / img_height;
  if (fill_width && fill_height) {
    // Scrolling diagonally. Fill as much as we can get in available space.
    // Largest scale fraction determines that.
    const float larger_fraction = (width_fraction > height_fraction)
      ? width_fraction
      : height_fraction;
    target_width = (int) roundf(larger_fraction * img_width);
    target_height = (int) roundf(larger_fraction * img_height);
  }
  else if (fill_height) {
    // Horizontal scrolling: Make things fit in vertical space.
    // While the height constraint stays the same, we can expand to full
    // width as we scroll along that axis.
    target_width = (int) roundf(height_fraction * img_width);
  }
  else if (fill_width) {
    // dito, vertical. Make things fit in horizontal space.
    target_height = (int) roundf(width_fraction * img_height);
  }

  for (size_t i = 0; i < result->size(); ++i) {
    (*result)[i].scale(Magick::Geometry(target_width, target_height));
  }

  return true;
}

void DisplayAnimation(const FileInfo *file,
                      RGBMatrix *matrix, FrameCanvas *offscreen_canvas) {
  const tmillis_t duration_ms = (file->is_multi_frame
                                 ? file->params.anim_duration_ms
                                 : file->params.wait_ms);
  rgb_matrix::StreamReader reader(file->content_stream);
  int loops = file->params.loops;
  const tmillis_t end_time_ms = GetTimeInMillis() + duration_ms;
  const tmillis_t override_anim_delay = file->params.anim_delay_ms;
  for (int k = 0;
       (loops < 0 || k < loops)
         && !interrupt_received
         && GetTimeInMillis() < end_time_ms;
       ++k) {
    uint32_t delay_us = 0;
    while (!interrupt_received && GetTimeInMillis() <= end_time_ms
           && reader.GetNext(offscreen_canvas, &delay_us)) {
      const tmillis_t anim_delay_ms =
        override_anim_delay >= 0 ? override_anim_delay : delay_us / 1000;
      const tmillis_t start_wait_ms = GetTimeInMillis();
      offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas,
                                             file->params.vsync_multiple);
      const tmillis_t time_already_spent = GetTimeInMillis() - start_wait_ms;
      SleepMillis(anim_delay_ms - time_already_spent);
    }
    reader.Rewind();
  }
}

static int image_usage(const char *progname) {
  fprintf(stderr, "usage: %s [options] <image> [option] [<image> ...]\n",
          progname);

  fprintf(stderr, "Options:\n"
          "\t-O<streamfile>            : Output to stream-file instead of matrix (Don't need to be root).\n"
          "\t-C                        : Center images.\n"

          "\nThese options affect images FOLLOWING them on the command line,\n"
          "so it is possible to have different options for each image\n"
          "\t-w<seconds>               : Regular image: "
          "Wait time in seconds before next image is shown (default: 1.5).\n"
          "\t-t<seconds>               : "
          "For animations: stop after this time.\n"
          "\t-l<loop-count>            : "
          "For animations: number of loops through a full cycle.\n"
          "\t-D<animation-delay-ms>    : "
          "For animations: override the delay between frames given in the\n"
          "\t                            gif/stream animation with this value. Use -1 to use default value.\n"
          "\t-V<vsync-multiple>        : For animation (expert): Only do frame vsync-swaps on multiples of refresh (default: 1)\n"
          "\t                            (Tip: use --led-limit-refresh for stable rate)\n"

          "\nOptions affecting display of multiple images:\n"
          "\t-f                        : "
          "Forever cycle through the list of files on the command line.\n"
          "\t-s                        : If multiple images are given: shuffle.\n"
          );

  fprintf(stderr, "\nGeneral LED matrix options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);

  fprintf(stderr,
          "\nSwitch time between files: "
          "-w for static images; -t/-l for animations\n"
          "Animated gifs: If both -l and -t are given, "
          "whatever finishes first determines duration.\n");

  fprintf(stderr, "\nThe -w, -t and -l options apply to the following images "
          "until a new instance of one of these options is seen.\n"
          "So you can choose different durations for different images.\n");

  return 1;
}

static int ImageViewer()
{
  //  needtouch, need to set argc and argv
  int argc = 3;
  char *argv[] = {"./demo", "-f", const_cast<char*>(imageList[selectedImageIndex].c_str())};  
  Magick::InitializeMagick(*argv);

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, (char***)(&argv),
                                         &matrix_options, &runtime_opt)) {
    return image_usage(argv[0]);
  }

  if(boardSize == "S") {
    matrix_options.rows = 16;
    matrix_options.cols = 32;
  } else if(boardSize == "M") {
    matrix_options.rows = 32;
    matrix_options.cols = 32;
  } else if(boardSize == "L") {
    matrix_options.rows = 32;
    matrix_options.cols = 64;
  } else if(boardSize == "XL") {
    matrix_options.rows = 48;
    matrix_options.cols = 96;
  }

  // matrix_options.chain_length = 4;
  // matrix_options.parallel = 3;
  matrix_options.multiplexing = 2;
  matrix_options.hardware_mapping = "adafruit-hat";

  runtime_opt.gpio_slowdown = 2;  

  bool do_forever = false;
  bool do_center = false;
  bool do_shuffle = false;

  // We remember ImageParams for each image, which will change whenever
  // there is a flag modifying them. This map keeps track of filenames
  // and their image params (also for unrelated elements of argv[], but doesn't
  // matter).
  // We map the pointer instad of the string of the argv parameter so that
  // we can have two times the same image on the commandline list with different
  // parameters.
  std::map<const void *, struct ImageParams> filename_params;

  // Set defaults.
  ImageParams img_param;
  for (int i = 0; i < argc; ++i) {
    filename_params[argv[i]] = img_param;
  }

  const char *stream_output = NULL;

  int opt;
  while ((opt = getopt(argc, argv, "w:t:l:fr:c:P:LhCR:sO:V:D:")) != -1) {
    switch (opt) {
    case 'w':
      img_param.wait_ms = roundf(atof(optarg) * 1000.0f);
      break;
    case 't':
      img_param.anim_duration_ms = roundf(atof(optarg) * 1000.0f);
      break;
    case 'l':
      img_param.loops = atoi(optarg);
      break;
    case 'D':
      img_param.anim_delay_ms = atoi(optarg);
      break;
    case 'f':
      do_forever = true;
      break;
    case 'C':
      do_center = true;
      break;
    case 's':
      do_shuffle = true;
      break;
    case 'r':
      fprintf(stderr, "Instead of deprecated -r, use --led-rows=%s instead.\n",
              optarg);
      matrix_options.rows = atoi(optarg);
      break;
    case 'c':
      fprintf(stderr, "Instead of deprecated -c, use --led-chain=%s instead.\n",
              optarg);
      matrix_options.chain_length = atoi(optarg);
      break;
    case 'P':
      matrix_options.parallel = atoi(optarg);
      break;
    case 'L':
      fprintf(stderr, "-L is deprecated. Use\n\t--led-pixel-mapper=\"U-mapper\" --led-chain=4\ninstead.\n");
      return 1;
      break;
    case 'R':
      fprintf(stderr, "-R is deprecated. "
              "Use --led-pixel-mapper=\"Rotate:%s\" instead.\n", optarg);
      return 1;
      break;
    case 'O':
      stream_output = strdup(optarg);
      break;
    case 'V':
      img_param.vsync_multiple = atoi(optarg);
      if (img_param.vsync_multiple < 1) img_param.vsync_multiple = 1;
      break;
    case 'h':
    default:
      return image_usage(argv[0]);
    }

    // Starting from the current file, set all the remaining files to
    // the latest change.
    for (int i = optind; i < argc; ++i) {
      filename_params[argv[i]] = img_param;
    }
  }

  const int filename_count = argc - optind;
  if (filename_count == 0) {
    fprintf(stderr, "Expected image filename.\n");
    return image_usage(argv[0]);
  }

  // Prepare matrix
  runtime_opt.do_gpio_init = (stream_output == NULL);
  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return 1;

  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();

  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
         matrix->width(), matrix->height(), matrix_options.hardware_mapping);

  // These parameters are needed once we do scrolling.
  const bool fill_width = false;
  const bool fill_height = false;

  // In case the output to stream is requested, set up the stream object.
  rgb_matrix::StreamIO *stream_io = NULL;
  rgb_matrix::StreamWriter *global_stream_writer = NULL;
  if (stream_output) {
    int fd = open(stream_output, O_CREAT|O_WRONLY, 0644);
    if (fd < 0) {
      perror("Couldn't open output stream");
      return 1;
    }
    stream_io = new rgb_matrix::FileStreamIO(fd);
    global_stream_writer = new rgb_matrix::StreamWriter(stream_io);
  }

  const tmillis_t start_load = GetTimeInMillis();
  fprintf(stderr, "Loading %d files...\n", argc - optind);
  // Preparing all the images beforehand as the Pi might be too slow to
  // be quickly switching between these. So preprocess.
  std::vector<FileInfo*> file_imgs;
  for (int imgarg = optind; imgarg < argc; ++imgarg) {
    const char *filename = argv[imgarg];
    FileInfo *file_info = NULL;

    std::string err_msg;
    std::vector<Magick::Image> image_sequence;
    if (LoadImageAndScale(filename, matrix->width(), matrix->height(),
                          fill_width, fill_height, &image_sequence, &err_msg)) {
      file_info = new FileInfo();
      file_info->params = filename_params[filename];
      file_info->content_stream = new rgb_matrix::MemStreamIO();
      file_info->is_multi_frame = image_sequence.size() > 1;
      rgb_matrix::StreamWriter out(file_info->content_stream);
      for (size_t i = 0; i < image_sequence.size(); ++i) {
        const Magick::Image &img = image_sequence[i];
        int64_t delay_time_us;
        if (file_info->is_multi_frame) {
          delay_time_us = img.animationDelay() * 10000; // unit in 1/100s
        } else {
          delay_time_us = file_info->params.wait_ms * 1000;  // single image.
        }
        if (delay_time_us <= 0) delay_time_us = 100 * 1000;  // 1/10sec
        StoreInStream(img, delay_time_us, do_center, offscreen_canvas,
                      global_stream_writer ? global_stream_writer : &out);
      }
    } else {
      // Ok, not an image. Let's see if it is one of our streams.
      int fd = open(filename, O_RDONLY);
      if (fd >= 0) {
        file_info = new FileInfo();
        file_info->params = filename_params[filename];
        file_info->content_stream = new rgb_matrix::FileStreamIO(fd);
        StreamReader reader(file_info->content_stream);
        if (reader.GetNext(offscreen_canvas, NULL)) {  // header+size ok
          file_info->is_multi_frame = reader.GetNext(offscreen_canvas, NULL);
          reader.Rewind();
          if (global_stream_writer) {
            CopyStream(&reader, global_stream_writer, offscreen_canvas);
          }
        } else {
          err_msg = "Can't read as image or compatible stream";
          delete file_info->content_stream;
          delete file_info;
          file_info = NULL;
        }
      }
      else {
        perror("Opening file");
      }
    }

    if (file_info) {
      file_imgs.push_back(file_info);
    } else {
      fprintf(stderr, "%s skipped: Unable to open (%s)\n",
              filename, err_msg.c_str());
    }
  }

  if (stream_output) {
    delete global_stream_writer;
    delete stream_io;
    if (file_imgs.size()) {
      fprintf(stderr, "Done: Output to stream %s; "
              "this can now be opened with led-image-viewer with the exact same panel configuration settings such as rows, chain, parallel and hardware-mapping\n", stream_output);
    }
    if (do_shuffle)
      fprintf(stderr, "Note: -s (shuffle) does not have an effect when generating streams.\n");
    if (do_forever)
      fprintf(stderr, "Note: -f (forever) does not have an effect when generating streams.\n");
    // Done, no actual output to matrix.
    return 0;
  }

  // Some parameter sanity adjustments.
  if (file_imgs.empty()) {
    // e.g. if all files could not be interpreted as image.
    fprintf(stderr, "No image could be loaded.\n");
    return 1;
  } else if (file_imgs.size() == 1) {
    // Single image: show forever.
    file_imgs[0]->params.wait_ms = distant_future;
  } else {
    for (size_t i = 0; i < file_imgs.size(); ++i) {
      ImageParams &params = file_imgs[i]->params;
      // Forever animation ? Set to loop only once, otherwise that animation
      // would just run forever, stopping all the images after it.
      if (params.loops < 0 && params.anim_duration_ms == distant_future) {
        params.loops = 1;
      }
    }
  }

  fprintf(stderr, "Loading took %.3fs; now: Display.\n",
          (GetTimeInMillis() - start_load) / 1000.0);

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  int curSelectedImageIndex = selectedImageIndex;
  do {
    if (do_shuffle) {
      std::random_shuffle(file_imgs.begin(), file_imgs.end());
    }
    for (size_t i = 0; i < file_imgs.size() && !interrupt_received; ++i) {
      DisplayAnimation(file_imgs[i], matrix, offscreen_canvas);
    }
    parseJson();
    if(demo != ImageDemo) break;
    if(curSelectedImageIndex != selectedImageIndex) break;
  } while (do_forever && !interrupt_received);

  if (interrupt_received) {
    fprintf(stderr, "Caught signal. Exiting.\n");
  }

  // Animation finished. Shut down the RGB matrix.
  matrix->Clear();
  delete matrix;

  // Leaking the FileInfos, but don't care at program end.
  return 0;
}

//-------------ImageViewer END------------------------

//-------------Patterns Display-------------------------
class SimplexNoiseGenerator : public ThreadedCanvasManipulator {
public:
  SimplexNoiseGenerator(Canvas *m) : ThreadedCanvasManipulator(m) {}
  void Run() {
    canvas()->Fill(0, 0, 0);  
    FastNoise noise;
    noise.SetNoiseType(FastNoise::Simplex);
    while ((demo==PatternDemo) && (noiseType==SimplexType) && running() && !interrupt_received) {    
      parseJson();    
      for(float i=0.0f; i<canvas()->width(); i+=1.0f) {
        for(float j=0.0f; j<canvas()->height(); j+=1.0f) {
          float col = (noise.GetNoise(i*scale,j*scale,k*speed)+1.0f)/2.0f*255.0f;
          col = col + globalBrightness - 127;
          if(col > 255) col = 255;
          col = (contrastFactor * (col - 127) + 127);
          if(col > 255) col = 255;
          else if (col < 0) col = 0;
          col  = col / 255.0f;
          // int col = (int)((noise.GetNoise(i,j,k)+1.0)/2.0*100.0);
          canvas()->SetPixel(i, j, col * baseR, col * baseG, col * baseB);
        }
      }
      k+=1.0f;
      usleep(15 * 1000);
    }
  }
};

class CellularNoiseGenerator : public ThreadedCanvasManipulator {
public:
  CellularNoiseGenerator(Canvas *m) : ThreadedCanvasManipulator(m) {}
  void Run() {
    canvas()->Fill(0, 0, 0);  
    FastNoise noise;
    noise.SetNoiseType(FastNoise::Cellular);
    while ((demo==PatternDemo) && (noiseType==CellularType) && running() && !interrupt_received) {    
      parseJson();    
      for(float i=0.0f; i<canvas()->width(); i+=1.0f) {
        for(float j=0.0f; j<canvas()->height(); j+=1.0f) {
          float col = (noise.GetNoise(i*scale,j*scale,k*speed)+1.0f)/2.0f*255.0f;
          col = col + globalBrightness - 127;
          if(col > 255) col = 255;
          col = (contrastFactor * (col - 127) + 127);
          if(col > 255) col = 255;
          else if (col < 0) col = 0;
          col  = col / 255.0f;
          // int col = (int)((noise.GetNoise(i,j,k)+1.0)/2.0*100.0);
          canvas()->SetPixel(i, j, col * baseR, col * baseG, col * baseB);
        }
      }
      k+=1.0f;
      usleep(15 * 1000);
    }
  }
};

class CubicNoiseGenerator : public ThreadedCanvasManipulator {
public:
  CubicNoiseGenerator(Canvas *m) : ThreadedCanvasManipulator(m) {}
  void Run() {
    canvas()->Fill(0, 0, 0);  
    FastNoise noise;
    noise.SetNoiseType(FastNoise::Cubic);
    while ((demo==PatternDemo) && (noiseType==CubicType) && running() && !interrupt_received) {    
      parseJson();    
      for(float i=0.0f; i<canvas()->width(); i+=1.0f) {
        for(float j=0.0f; j<canvas()->height(); j+=1.0f) {
          float col = (noise.GetNoise(i*scale,j*scale,k*speed)+1.0f)/2.0f*255.0f;
          col = col + globalBrightness - 127;
          if(col > 255) col = 255;
          col = (contrastFactor * (col - 127) + 127);
          if(col > 255) col = 255;
          else if (col < 0) col = 0;
          col  = col / 255.0f;
          // int col = (int)((noise.GetNoise(i,j,k)+1.0)/2.0*100.0);
          canvas()->SetPixel(i, j, col * baseR, col * baseG, col * baseB);
        }
      }
      k+=1.0f;
      usleep(15 * 1000);
    }
  }
};

static int NoisePatternGenerator()
{
  // OR it can be set argc and argv manually,  needtouch
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  if(boardSize == "S") {
    matrix_options.rows = 16;
    matrix_options.cols = 32;
  } else if(boardSize == "M") {
    matrix_options.rows = 32;
    matrix_options.cols = 32;
  } else if(boardSize == "L") {
    matrix_options.rows = 32;
    matrix_options.cols = 64;
  } else if(boardSize == "XL") {
    matrix_options.rows = 48;
    matrix_options.cols = 96;
  }

  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.show_refresh_rate = false;
  matrix_options.limit_refresh_rate_hz = 240;
  // matrix_options.multiplexing = 2;
  matrix_options.hardware_mapping = "adafruit-hat";

  runtime_opt.gpio_slowdown = 2;  

  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);

  if (matrix == NULL)
    return 1;
  
  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
        matrix->width(), matrix->height(), matrix_options.hardware_mapping);
  Canvas *canvas = matrix;
  
  ThreadedCanvasManipulator *image_gen = NULL;
  switch(noiseType)  //noiseType is gotten by parsing json
  {
    case SimplexType:
      image_gen = new SimplexNoiseGenerator(canvas);
      break;
    case CellularType:
      image_gen = new CellularNoiseGenerator(canvas);
      break;
    case CubicType:
      image_gen = new CubicNoiseGenerator(canvas);
      break;
  }
  
  if (image_gen == NULL)
    return 1;

  // Set up an interrupt handler to be able to stop animations while they go
  // on. Note, each demo tests for while (running() && !interrupt_received) {},
  // so they exit as soon as they get a signal.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  int noiseTypeBackup = noiseType;
  // Image generating demo is crated. Now start the thread.
  image_gen->Start();

  // Now, the image generation runs in the background. We can do arbitrary
  // things here in parallel. In this demo, we're essentially just
  // waiting for one of the conditions to exit.
  printf("Press <CTRL-C> to exit and reset LEDs\n");
  while ((demo == PatternDemo) && (noiseType == noiseTypeBackup) && !interrupt_received) {
    // if(demo == 0)
    //     DrawOnCanvas(canvas, noise);    
    // else break;
    sleep(1); // Time doesn't really matter. The syscall will be interrupted.
  }
 
  // Stop image generating thread. The delete triggers
  delete image_gen;
  // Animation finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  printf("\%s. Exiting.\n",
         interrupt_received ? "Received CTRL-C" : "Timeout reached");

  return 0;
}

// static void DrawOnCanvas(Canvas *canvas, FastNoise noise) {
//   canvas->Fill(0, 0, 0);
//   while (demo==0 && !interrupt_received) {
//     parseJson();    

//     for(float i=0.0f; i<canvas->width(); i+=1.0f) {
//       for(float j=0.0f; j<canvas->height(); j+=1.0f) {
//         float col = (noise.GetNoise(i*scale,j*scale,k*speed)+1.0f)/2.0f*255.0f;
//         col = col + globalBrightness - 127;
//         if(col > 255) col = 255;
//         col = (contrastFactor * (col - 127) + 127);
//         if(col > 255) col = 255;
//         else if (col < 0) col = 0;
//         col  = col / 255.0f;
//         // int col = (int)((noise.GetNoise(i,j,k)+1.0)/2.0*100.0);
//         canvas->SetPixel(i, j, col * baseR, col * baseG, col * baseB);
//       }
//     }
//     k+=1.0f;
//     usleep(15 * 1000);
//   }
// }

// static int NoisePatternGenerator()
// {
//   // OR it can be set argc and argv manually,  needtouch
//   RGBMatrix::Options matrix_options;
//   rgb_matrix::RuntimeOptions runtime_opt;

//   // These are the defaults when no command-line flags are given.
//   matrix_options.rows = 32;
//   matrix_options.cols = 64;
//   matrix_options.chain_length = 1;
//   matrix_options.parallel = 1;
//   matrix_options.show_refresh_rate = false;
//   matrix_options.limit_refresh_rate_hz = 240;

//   runtime_opt.gpio_slowdown = 2;

//   FastNoise noise;
//   noise.SetNoiseType(FastNoise::Simplex);

//   RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);

//   if (matrix == NULL)
//     return 1;
  
//   printf("Size: %dx%d. Hardware gpio mapping: %s\n",
//         matrix->width(), matrix->height(), matrix_options.hardware_mapping);
//   Canvas *canvas = matrix;
  
//   // Set up an interrupt handler to be able to stop animations while they go
//   // on. Note, each demo tests for while (running() && !interrupt_received) {},
//   // so they exit as soon as they get a signal.
//   signal(SIGTERM, InterruptHandler);
//   signal(SIGINT, InterruptHandler);

//   // Now, the image generation runs in the background. We can do arbitrary
//   // things here in parallel. In this demo, we're essentially just
//   // waiting for one of the conditions to exit.
//   printf("Press <CTRL-C> to exit and reset LEDs\n");
//   while (!interrupt_received) {
//     if(demo == 0)
//         DrawOnCanvas(canvas, noise);    
//     else break;
//   }
 
//   // Animation finished. Shut down the RGB matrix.
//   canvas->Clear();
//   delete canvas;

//   printf("\%s. Exiting.\n",
//          interrupt_received ? "Received CTRL-C" : "Timeout reached");

//   return 0;
// }


int main(int argc, char *argv[]) {
  
  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The 4 routines is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  parseJson();
  // Now, the image generation runs in the background. We can do arbitrary
  // things here in parallel. In this demo, we're essentially just
  // waiting for one of the conditions to exit.
  printf("Press <CTRL-C> to exit and reset LEDs\n");
  while (!interrupt_received) {    
    switch(demo)
    {
      case 0:
        NoisePatternGenerator();
        break;
      case 1:
        ImageViewer();
        break;
      case 2:
        VideoViewer();
        break;
      case 3:
        TextViewer();
        break;
    }
  }
  
  printf("\%s. Exiting.\n",
         interrupt_received ? "Received CTRL-C" : "Timeout reached");
  return 0;
}
