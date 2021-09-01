// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "FastNoise.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>

//#include <json/json.h>
//#include <jsoncpp/json/json.h>
//#include <json/value.h>
#include <fstream>

#include<vector>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool interrupt_received = false;

int baseR = 255;
int baseG = 255;
int baseB = 255;
float globalBrightness = 0.0f;
float scale = 5.0f;
float speed = 1.0f;
float contrast = 50.0f;
float k = 0.0f;
float contrastFactor;

static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static void DrawOnCanvas(Canvas *canvas, FastNoise noise) {
  canvas->Fill(0, 0, 0);
  while (!interrupt_received) {
     printf("First parsing");
     std::ifstream json_file("/data/data.json", std::ifstream::binary);
     std::ostringstream ss;
     ss << json_file.rdbuf(); // reading data
     std::string stringJson = ss.str();
    //  printf("data.json: %s",stringJson);

     vector<string> result;
     stringstream s_stream(stringJson); //create string stream from the string
     while(s_stream.good()) {
      string substr;
      getline(s_stream, substr, ','); //get first string delimited by comma
      result.push_back(substr);
     }
     for(int i = 0; i<result.size(); i++) {
      if(i == 0) {
	// printf("%s \n", result.at(0).substr(result.at(0).find(":") + 1).c_str());
	speed = atof(result.at(0).substr(result.at(0).find(":") + 1).c_str())/25;
      }
      if(i == 1) {
	// printf("%s \n", result.at(1).substr(result.at(1).find(":") + 1).c_str());
        scale = atof(result.at(1).substr(result.at(1).find(":") + 1).c_str())/10;
      }
      if(i == 2) {
	// printf("%s \n", result.at(2).substr(result.at(2).find(":") + 1).c_str());
	baseR = atoi(result.at(2).substr(result.at(2).find(":") + 1).c_str());
      }

      if(i == 3) {
	// printf("%s \n", result.at(3).substr(result.at(3).find(":") + 1).c_str());
        baseG = atoi(result.at(3).substr(result.at(3).find(":") + 1).c_str());
      }
      if(i == 4) {
	// printf("%s \n", result.at(4).substr(result.at(4).find(":") + 1).c_str());
        baseB = atoi(result.at(4).substr(result.at(4).find(":") + 1).c_str());
      }
      if(i == 5) {	
	// printf("%s \n",  result.at(5).substr(result.at(5).find(":") + 1).c_str());
      	globalBrightness = atof(result.at(5).substr(result.at(5).find(":") + 1).c_str());        
      }
      if(i == 6) {
	string a = result.at(6).substr(result.at(6).find(":") + 1);
	// printf("%s \n",  a.substr(0,a.size()-1).c_str());
	contrast = atof(a.substr(0,a.size()-1).c_str());
  contrastFactor = ((259.0f*(contrast+255.0f))/(255.0f*(259.0f-contrast)));
      }
     }

    // printf("speed: %f \n",  speed);
    // printf("scale: %f \n",  scale);
    // printf("globalBrightness: %f \n",  globalBrightness);
    // printf("baseR: %d \n",  baseR);
    // printf("baseG: %d \n",  baseG);
    // printf("baseB: %d \n",  baseB);
    // printf("contrast: %f \n",  contrast);

    for(float i=0.0f; i<canvas->width(); i+=1.0f) {
      for(float j=0.0f; j<canvas->height(); j+=1.0f) {
        float col = (noise.GetNoise(i*scale,j*scale,k*speed)+1.0f)/2.0f*255.0f;
        col = col + globalBrightness - 127;
        if(col > 255) col = 255;
        col = (contrastFactor * (col - 127) + 127);
        if(col > 255) col = 255;
        else if (col < 0) col = 0;
        col  = col / 255.0f;
        // int col = (int)((noise.GetNoise(i,j,k)+1.0)/2.0*100.0);
        canvas->SetPixel(i, j, col * baseR, col * baseG, col * baseB);
      }
    }
    k+=1.0f;
    if(k > 999999.0f)
      k = 0;
    usleep(15 * 1000);
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options defaults;
  // defaults.hardware_mapping = "adafruit-hat";
  defaults.rows = 32;
  defaults.cols = 64;
  defaults.chain_length = 1;
  defaults.parallel = 1;
  defaults.show_refresh_rate = false;
  
  FastNoise noise;

  noise.SetNoiseType(FastNoise::Simplex);
  // noise.SetSeed(1337);

  Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults);
  if (canvas == NULL)
    return 1;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // DrawOnCanvas(canvas);    // Using the canvas.
  while(!interrupt_received)
    DrawOnCanvas(canvas, noise);

  // Animation finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  printf("\%s. Exiting.\n",
         interrupt_received ? "Received CTRL-C" : "Timeout reached");

  return 0;
}
