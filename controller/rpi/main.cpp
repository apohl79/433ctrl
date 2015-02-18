#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <thread>
#include <getopt.h>
#include <wiringPi.h>

using namespace std;
using namespace chrono;

const time_point<high_resolution_clock> TP_UNSET;
time_point<high_resolution_clock> g_last(TP_UNSET);
const size_t MAX_TIME_VALUES = 128;
const double MIN_TIME_DIFF_RATIO = 0.7;
vector<uint32_t> g_times(MAX_TIME_VALUES);
uint32_t g_time_idx = 0;
uint8_t g_sync_count = 0;
atomic<bool> g_capture_done(false);

void send_rc_code(int pin, string& code, int repeat) {
    pinMode(pin, OUTPUT);
    // parse the list of micro secs
    vector<useconds_t> codes;
    codes.push_back(atoi(code.c_str()));
    string::size_type pos = 0;
    while (string::npos != (pos = code.find(' ', pos + 1))) {
        codes.push_back(atoi(code.c_str() + pos));        
    }
    // send the code <repeat> times
    while (repeat--) {
        bool out = true; // high/low toggle
        for (auto c : codes) {
            digitalWrite(pin, (int) out);
            usleep(c);
            out = !out;
        }
    }
    digitalWrite(pin, 0);
}

uint32_t get_pulselen() {
    uint32_t pulselen = 4000;
    // find the smallest value first
    for (uint32_t i = 0; i < g_time_idx; i++) {
        if (g_times[i] < pulselen) {
            pulselen = g_times[i];
        }
    }
    // now we calculate the avg of all short values
    uint32_t sum = 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_time_idx; i++) {
        if (g_times[i] / pulselen == 1) {
            if ((double) pulselen / g_times[i] < MIN_TIME_DIFF_RATIO) {
                return 0;
            }
            sum += g_times[i];
            count++;
        }
    }
    pulselen = sum / count;
    return pulselen;
}

void interrupt_handler() {
    if (g_capture_done) {
        return;
    }
    auto now = high_resolution_clock::now();
    if (TP_UNSET == g_last) {
        g_last = now;
    } else {
        uint32_t timediff = duration<uint32_t, micro>(now - g_last).count();
        g_last = now;
        if (g_time_idx < MAX_TIME_VALUES) {
            g_times[g_time_idx++] = timediff;
        } else {
            // reset
            g_time_idx = 0;
            g_sync_count = 0;
        }
        if (timediff > 4000) {
            g_sync_count++;
            if (g_sync_count == 2 && g_time_idx > 20) {
                uint32_t pulselen = get_pulselen();
                if (pulselen > 30 && pulselen < 1000) {
                    // output values as multiple pulselens
                    for (uint32_t i = 0; i < g_time_idx; i++) {
                        uint32_t pulses = g_times[i] / pulselen;
                        // as the pulse length is the average of all short values, we could get 0 here
                        if (pulses == 0) {
                            pulses = 1;
                        }
                        cout << (pulses * pulselen) << " ";
                    }
                    cout << endl;
                    g_capture_done = true;
                } else {
                    // reset
                    g_time_idx = 0;
                    g_sync_count = 0;                    
                }
            } else {
                g_time_idx = 0;
            }
        }
    }
}

void recv_rc_code(int pin) {
    pinMode(pin, INPUT);
    if (wiringPiISR(pin, INT_EDGE_BOTH, interrupt_handler) < 0) {
        cerr << "wiringPiISR failed" << endl;
        return;
    }
    while (!g_capture_done) {
        this_thread::sleep_for(seconds(1));
    }
}

void usage(const char* prog) {
    cerr << "Usage: " << prog << " <Options>" << endl
              << endl
              << "Options:" << endl
              << "  --code='<code>'         Code to send (ints separated with spaces). This code can" << endl
              << "                          be captured via --rc-learn." << endl
              << "  --learn                 Capture a code sequence." << endl
              << "  --pin=<number>          Transmitter/Receiver pin. Default is 0." << endl
              << "  --repeat=<number>       Number of times to repeat the code/message. This is" << endl
              << "                          needed as the RPi can't do real time from user space." << endl
              << "                          Default is 20." << endl
        ;
}

int main(int argc, char** argv) {
    const struct option longopts[] =
        {
            {"pin", required_argument, 0, 'p'},
            {"repeat", required_argument, 0, 'r'},
            {"code", required_argument, 0, 'c'},
            {"learn", optional_argument, 0, 'l'},
            {0, 0, 0, 0},
        };
    
    if (argc <= 1) {
        usage(argv[0]);
        return 1;
    }

    string msg;
    int mode = 0;
    int pin = 0;
    int repeat = 20;

    int index;
    int opt;
    while ((opt = getopt_long(argc, argv, "", longopts, &index)) != -1) {
        switch (opt) {
        case 'p':
            pin = atoi(optarg);
            break;
        case 'r':
            repeat = atoi(optarg);
            break;
        case 'c':
            msg = optarg;
            mode = 0;
            break;
        case 'l':
            mode = 1;
            break;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (wiringPiSetup() == -1) {
        cerr << "wiringPiSetup failed" << endl;
        return 1;
    }

    switch (mode) {
    case 0:
        send_rc_code(pin, msg, repeat);
        break;
    case 1:
        recv_rc_code(pin);
        break;
    }
    
    return 0;
}
