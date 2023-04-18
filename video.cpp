#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "Progressbar.h"

using namespace cv;
using namespace std;

int split_video(string video_name, int using_fps, bool recreate_frames){
    VideoCapture cap(video_name);

    
    if(!cap.isOpened()){
        return -1;
    }
    if (recreate_frames){
        filesystem::remove_all(format("%s-%d-opencv",video_name.c_str(),using_fps));
    }
    bool is_not_created = filesystem::create_directory(format("%s-%d-opencv",video_name.c_str(),using_fps));
    int video_fps = cap.get(CAP_PROP_FPS);

    int frames_count = cap.get(CAP_PROP_FRAME_COUNT);
    float duration = frames_count / video_fps;
    float frame_duration = 1/(float)video_fps;
    float new_frame_duration = 1/(float)using_fps;

    int counter = 1;
    int final_counter = -1;
    int iteration_counter = 1;

    auto time_start = chrono::system_clock::now();
    auto time_end = time_start;
    if (is_not_created){
        cout <<"\33[?25l";
        while(true){
            auto time_diff =  std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start);
            draw_progress_bar("Frames generating", iteration_counter, frames_count,time_diff.count());
            time_start = chrono::system_clock::now();

            Mat frame;
            cap >> frame;

            if (frame.empty())
                break;

            if (counter*frame_duration >= new_frame_duration){
                imwrite(format("%s-%d-opencv/frame%d.jpg",video_name.c_str(),using_fps,++final_counter), frame);
                counter = 0;
            }
            counter++;
            iteration_counter++;
            time_end = chrono::system_clock::now();

        }
        cout << endl;
    }
    else {
        while (true) {
            ifstream iff;
            iff.open(format("%s-%d-opencv/frame%d.jpg",video_name.c_str(),using_fps,++final_counter));
            if (!iff.is_open()) {
                iff.close();
                break;
            }
            iff.close();

        }
        final_counter--;
    }


    cap.release();


    return final_counter+1;
}
