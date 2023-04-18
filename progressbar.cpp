#include <iostream>

using namespace std;

void draw_progress_bar(string text,int current, int max, long frame_time){
    int all_time_second = (frame_time*(max-current))/1000;
    int hr=(int)(all_time_second/3600);
    int min=((int)(all_time_second/60))%60;
    int sec=(int)(all_time_second%60);
    int size = 40;
    string hr_str;
    string min_str;
    string sec_str;
    if (hr <10) {
        hr_str += "0";
    }
    hr_str+= to_string(hr);
    if (min <10) {
        min_str += "0";
    }
    min_str+= to_string(min);
    if (sec <10) {
        sec_str += "0";
    }
    sec_str+= to_string(sec);

    int x = int(size*current/max);

    string drawen;
    for (int i = 0; i < x; i++){
        drawen += "█";
    }

    string notDrawen;
    for (int i = 0; i < size-x; i++){
        notDrawen += '.';
    }
    cout << text << ": [" << drawen << notDrawen << "] " << current << '/' << max
    << " ETA: " << hr_str << ':' << min_str << ':' << sec_str <<  '\r';
}
