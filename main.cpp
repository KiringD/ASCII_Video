#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#include <chrono>
#include <thread>
#include <filesystem>

#include "Video.h"
#include "Progressbar.h"

using namespace cv;
using namespace std;

string ascii_characters = "`^\",:;lI!i~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%BB@$";



string* convert_to_ascii_art(int*** image, int height, int width);
char convert_pixel_to_character(int r, int g, int b);
void print_in_terminal(string all_frame);
bool is_video_cache(string name, int rows, int columns, int fps);
string* compress(string* array, int frame_count);
void decompress(string* array, int frame_count);
bool isNumeric(char* str);
void help();


int main(int argc, char **argv)
{
    ios_base::sync_with_stdio(true);

    string video_name;
    int fps;

    bool create_cache = false;
    bool save_frames = false;
    bool dont_use_cache = false;
    bool recreate_frames = false;

    int counter = 0;
    for(int i = 0; i<argc; i++){
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (string_view(argv[i]) == "--help"){
                help();
                return 0;
            }
            else if (string_view(argv[i]) == "--create-cache"){
                create_cache = true;
            }
            else if (string_view(argv[i]) == "--save-frames"){
                save_frames = true;
            }
            else if (string_view(argv[i]) == "--dont-use-cache"){
                dont_use_cache = true;
            }
            else if (string_view(argv[i]) == "--recreate-frames"){
                recreate_frames = true;
                save_frames = true;
            }
            else{
                cout << "Unknown parameter: " << argv[i] << endl << "Use --help to find out all available parameters." << endl;
                return 0;
            }
        }
        else{
            switch (counter) {
                case 0:
                    counter++;
                    break;
                case 1:
                    video_name = argv[i];
                    counter++;
                    break;
                case 2:
                    if (!isNumeric(argv[i])){
                        cout << "Second argument (fps) must be numeric." << endl << "Use --help to find out usage information." << endl;
                        return 0;
                    }
                    else {
                        fps = stoi(argv[i]);
                        if (fps > 60 || fps < 1){
                            cout << "FPS must be between 1 and 60." << endl << "Use --help to find out usage information." << endl;
                            return 0;
                        }
                    }
                    counter++;
                    break;
                default:
                    cout << "Too much arguments." << endl << "Use --help to find out usage information." << endl;
                    return 0;
            }

        }
    }

    if (counter != 3){
        cout << "Wrong number of arguments." << endl << "Use --help to find out usage information." << endl;
        return 0;
    }

    struct winsize w{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    string* FramesList;
    int frame_count;
    if (!is_video_cache(video_name, w.ws_row, w.ws_col, fps) || dont_use_cache){
        frame_count = split_video(video_name, fps, recreate_frames);
        if (frame_count == -1){
            cout << "Video file not found" << endl;
            return 0;
        }

        Mat frame;

        FramesList = new string[frame_count];

        string all_frame;
        int** imList[w.ws_row];
        for (int i = 0; i <w.ws_row; i++){
            imList[i] = new int*[w.ws_col];
        }

        auto time_start = chrono::system_clock::now();
        auto time_end = time_start;
        cout <<"\33[?25l";
        for (int k = 0; k<frame_count; k++) {
            auto time_diff =  std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start);
            draw_progress_bar("Rendering", k, frame_count,time_diff.count());
            time_start = chrono::system_clock::now();
            frame = imread(format("%s-%d-opencv/frame%d.jpg",video_name.c_str(),fps,k));
            Mat resizedFrame;

            resize(frame, resizedFrame, Size(w.ws_col, w.ws_row), INTER_LINEAR);

            for(int i = 0; i < w.ws_row; i++)
            {
                for(int j = 0; j < w.ws_col; j++)
                {
                    Vec3b bgrPixel = resizedFrame.at<Vec3b>(i, j);
                    int r = bgrPixel[0];
                    int g = bgrPixel[1];
                    int b = bgrPixel[2];
                    imList[i][j] = new int[3] {r,g,b};
                }

            }
            string* ascii_art = convert_to_ascii_art(imList, w.ws_row, w.ws_col);

            for(int i = 0; i < w.ws_row; i++)
            {
                for(int j = 0; j < w.ws_col; j++)
                {
                    delete[] imList[i][j];
                }

            }
            all_frame.clear();
            for (int i = 0; i < w.ws_row; i++){
                all_frame += ascii_art[i];
                all_frame += '\n';
            }
            all_frame.erase(all_frame.size()-1);


            FramesList[k] = all_frame;

            time_end = chrono::system_clock::now();
        }
        cout <<"\33[?25h" << endl;

        for (int i = 0; i <w.ws_row; i++){
            delete[] imList[i];
        }

        if (create_cache){
            filesystem::create_directory("cache");

            string* compressedFrames = compress(FramesList, frame_count);

            ofstream fs(format("cache/%s-%d-%d-%d.bin",video_name.c_str(),w.ws_row,w.ws_col,fps), ios::binary);

            fs.write((char *) &frame_count, sizeof(int));
            for (int i = 0; i < frame_count; i++){
                int size = compressedFrames[i].length();
                fs.write((char *) &size, sizeof(int));
                fs.write(compressedFrames[i].c_str(), size);
            }
            fs.close();
        }

        if (!save_frames){
            bool is_not_created = filesystem::create_directory(format("%s-%d-opencv",video_name.c_str(),fps));
            if (!is_not_created){
                filesystem::remove_all(format("%s-%d-opencv",video_name.c_str(),fps));
            }

        }
    }
    else{
        ifstream fs(format("cache/%s-%d-%d-%d.bin",video_name.c_str(),w.ws_row,w.ws_col,fps), ios::binary);


        int size;
        char* buffer;

        fs.read((char *) &frame_count,sizeof(int));
        FramesList = new string[frame_count];
        for (int i = 0; i < frame_count; i++){
            fs.read((char *) &size, sizeof(int));
            buffer = new char[size];
            fs.read(buffer, size);
            FramesList[i] = string(buffer, size);
            delete[] buffer;
        }
        fs.close();

        decompress(FramesList, frame_count);

    }



    ios_base::sync_with_stdio(false);

    for (int i = 0; i < frame_count; i++){
        print_in_terminal(FramesList[i]);
        this_thread::sleep_for(chrono::milliseconds (int((float)1/(float)fps*(float)1000)));

    }

    return 0;
}

void help(){
    cout << "Usage: ./ascii_video [video_path] [fps(1-60)] [arguments]" << endl;
    cout << "Awailable arguments:" << endl;
    cout << "--help             This page\n"
            "--create-cache     Create a cache to start playing fast with current settings (terminal size, fps, video_file_name).\n"
            "                   If settings changed, you will have to create a new cache.\n"
            "--save-frames      Save frames of video file with current fps.\n"
            "                   Frames preparation will be skipped in future, if video and fps didn't change.\n"
            "--dont-use-cache   All frames will be rendered regardless of whether the cache is created or not.\n"
            "--recreate-frames  Recreates frames and save it." << endl;

}

bool isNumeric(char* str)
{
    for(int i = 0; str[i] != '\0'; i++) {
        if(!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

string* convert_to_ascii_art(int*** image, int height, int width){
    string* ascii_art = new string[height];
    string line;
    for (int y = 0; y<height-1; y++){
        line.clear();
        for (int x = 0; x<width-1; x++) {
            int r = image[y][x][0];
            int g = image[y][x][1];
            int b = image[y][x][2];

            line.push_back(convert_pixel_to_character(r, g, b));
        }

        ascii_art[y] = line;

    }
    return ascii_art;
}

char convert_pixel_to_character(int r, int g, int b){
    int pixel_brightness = r+g+b;
    int max_brightness = 255 * 3;

    float brightness_weight = (float)ascii_characters.length()/(float)max_brightness;

    int index;
    index = int(pixel_brightness * brightness_weight) - 1;
    if (index < 0){
        index = 0;
    }
    return ascii_characters[index];
}


void print_in_terminal(string all_frame){
    cout << all_frame;
}

bool is_video_cache(string name, int rows, int columns, int fps){
    ifstream i;
    i.open(format("cache/%s-%d-%d-%d.bin",name.c_str(),rows,columns,fps));
    if (!i.is_open()) {
        i.close();
        return false;
    }
    i.close();
    return true;
}

string* compress(string* array, int frame_count){
    string* compressed_array = new string[frame_count];
    int local_counter;
    string frame;
    for (int i = 0; i < frame_count; i++){
        local_counter = 1;
        frame.clear();
        for (int j = 0; j < array[i].length()-1; j++){
            if(array[i][j] == array[i][j+1]){
                local_counter++;
            }
            else {
                frame += to_string(local_counter);
                frame += '>';
                if (array[i][j] != '\n'){
                    frame += array[i][j];
                }
                else{
                    frame += '<';
                }

                local_counter = 1;
            }

        }
        frame += to_string(local_counter);
        frame += '>';
        if (array[i][array[i].length()-1] != '\n'){
            array[i][array[i].length()-1];
        }
        else{
            frame += '<';
        }

        compressed_array[i] = frame;
    }

    return compressed_array;
}

void decompress(string* array, int frame_count){
    string Frame;
    string number;
    bool tmp = false;
    for (int i = 0; i < frame_count; i++){
        Frame.clear();
        number.clear();
        for (int j = 0; j < array[i].length(); j++){
            if (array[i][j] != '>'){
                number += array[i][j];
            }
            else {
                if (array[i][j+1] != '<'){
                    for (int g = 0; g < stoi(number); g++){
                        Frame += array[i][j+1];
                    }
                }
                else{
                    Frame += '\n';
                }

                number.clear();
                j++;
            }
        }
        array[i] = Frame;
    }
}
