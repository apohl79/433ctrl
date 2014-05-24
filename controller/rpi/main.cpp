#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <getopt.h>

#include "wiringPi.h"

#include <RadioHead.h>
#include <RH_ASK.h>
#include <RHReliableDatagram.h>

/**
 * \brief Send a raw rc code captured by pilight-debug.
 */
void send_rc_code(int pin, std::string& code, int repeat) {
    // parse the list of micro secs
    std::vector<useconds_t> codes;
    codes.push_back(std::atoi(code.c_str()));
    std::string::size_type pos = 0;
    while (std::string::npos != (pos = code.find(' ', pos + 1))) {
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
    std::cout << std::endl;
}

void send_message(int pin, int speed, std::string& msg, int repeat) {
    RH_ASK drv(speed, pin, pin+1, pin+2);
    RHDatagram mgr(drv, 5);
    std::cout << "init" << std::endl;
    if (!mgr.init()) {
        std::cerr << "init failed" << std::endl;
        return;
    }
    std::cout << "sending" << std::endl;;
    while(repeat--) {
        mgr.sendto((const uint8_t*) msg.c_str(), msg.length(), 5);
        if (!mgr.waitPacketSent()) {
            std::cout << "failed" << std::endl;
            return;
        }
    }
    std::cout << "done" << std::endl;
}

void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <Options>" << std::endl
              << std::endl
              << "Options:" << std::endl
              << "  --rc-code='<code>'      Raw code to send (ints separated with spaces). This code" << std::endl
              << "                          can be captured by running pilight-debug." << std::endl
              << "  --pin=<number>          Transmitter pin. Default is 0." << std::endl
              << "  --repeat=<number>       Number of times to repeat the code/message. This is" << std::endl
              << "                          needed as the RPi can't do real time from user space." << std::endl
              << "                          Default is 20." << std::endl
              << "  --message='<msg>'       Message to send." << std::endl
              << "  --speed=<bps>           Speed in bits per second. Default is 2000." << std::endl
        ;
}

int main(int argc, char** argv) {
    const struct option longopts[] =
        {
            {"pin", required_argument, 0, 'p'},
            {"repeat", required_argument, 0, 'r'},
            {"rc-code", required_argument, 0, 'c'},
            {"message", required_argument, 0, 'm'},
            {"speed", required_argument, 0, 's'},
            {0, 0, 0, 0},
        };
    
    if (argc <= 1) {
        usage(argv[0]);
        return 1;
    }

    std::string msg;
    int mode = 0;
    int pin = 0;
    int repeat = 20;
    int speed = 2000;

    int index;
    int opt;
    while ((opt = getopt_long(argc, argv, "", longopts, &index)) != -1) {
        switch (opt) {
        case 'p':
            pin = std::atoi(optarg);
            break;
        case 'r':
            repeat = std::atoi(optarg);
            break;
        case 'c':
            mode = 1; // RC mode
        case 'm':
            msg = optarg;
            break;
        case 's':
            speed = std::atoi(optarg);
            break;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (wiringPiSetup() == -1) {
        std::cerr << "wiringPiSetup failed" << std::endl;
        return 1;
    }
    pinMode(pin, OUTPUT);

    switch (mode) {
    case 0:
        send_message(pin, speed, msg, repeat);
        break;
    case 1:
        send_rc_code(pin, msg, repeat);
        break;
    }
    
    return 0;
}
