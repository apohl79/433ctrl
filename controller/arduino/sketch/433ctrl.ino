#define PIN 3
#define PINW 4
#define MAX_TIME_VALUES 128
const double MIN_TIME_DIFF_RATIO = 0.7;
const uint32_t PULSE_LEN_STEPS = 50;

uint32_t g_times[MAX_TIME_VALUES];
uint32_t g_last = 0;
uint32_t g_time_idx = 0;
uint32_t g_pulselen = 0;
uint8_t g_sync_count = 0;
bool g_capture_done = false;
bool g_capturing = false;

char g_read_buffer[1024];
int g_read_pos = 0;

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
    if (!g_capturing || g_capture_done) {
        return;
    }
    uint32_t now = micros();
    if (0 == g_last) {
        g_last = now;
    } else {
        uint32_t timediff = now - g_last;
        g_last = now;
        if (g_time_idx < MAX_TIME_VALUES) {
            g_times[g_time_idx++] = timediff;
        } else {
            // reset
            g_time_idx = 0;
            g_sync_count = 0;
        }
        if (timediff > 3000) {
            g_sync_count++;
            if (g_sync_count == 2 && g_time_idx > 20) {
                g_pulselen = get_pulselen();
                // Serial.print("-- Captured ");
                // Serial.print(g_time_idx);
                // Serial.print(" pulses. Pulselen is ");
                // Serial.print(g_pulselen);
                // Serial.println(".");
                if (g_pulselen > 30 && g_pulselen < 1000) {
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

void send() {
    for (int x = 0; x < 10; x++) {
        int out = 1;
        for (uint32_t i = 0; i < g_time_idx; i++) {
            digitalWrite(PINW, out);
            delayMicroseconds(g_times[i]);
            out = out? 0: 1;
        }
    }
}

void handler(const char* cmd, uint16_t len) {
    if (!strncmp(cmd, "RCV", 3)) {
        g_capturing = true;
        //Serial.print("OK\r\n");
    } else if (!strncmp(cmd, "SND ", 4)) {
        g_pulselen = atoi(cmd + 4);
        uint32_t i = 0;
        const char* pos = strchr(cmd + 4, ' ');
        while (NULL != pos) {
            g_times[i++] = atoi(pos + 1) * g_pulselen;
            pos = strchr(pos + 1, ' ');
        }
        g_time_idx = i;
        // Serial.print("Pulse Len: ");
        // Serial.print(g_pulselen);
        // Serial.print("\r\n");
        // Serial.print("Received ");
        // Serial.print(g_time_idx);
        // Serial.print(" time values\r\n");
        send();
        Serial.print("OK\r\n");
    } else if (!strncmp(cmd, "SNX ", 4)) {
        uint32_t i = 0;
        const char* pos = strchr(cmd + 4, ' ');
        while (NULL != pos) {
            g_times[i++] = atoi(pos + 1);
            pos = strchr(pos + 1, ' ');
        }
        g_time_idx = i;
        send();
        Serial.print("OK\r\n");
    } else {
        Serial.print("ERROR\r\n");
    }
}

void setup() {
    // put your setup code here, to run once:
    pinMode(PIN, INPUT);
    pinMode(PINW, OUTPUT);
    digitalWrite(PINW, 0);
    Serial.begin(9600);
    attachInterrupt(/*digitalPinToInterrupt(PIN)*/ 1, interrupt_handler, CHANGE);
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        switch (c) {
        case '\r':
        case '\n':
            if (g_read_pos > 0) {
                g_read_buffer[g_read_pos] = 0;
                handler(g_read_buffer, g_read_pos);
            }
            g_read_pos = 0;
            break;
        default:
            g_read_buffer[g_read_pos++] = c;
        }        
    }
    if (g_capturing && g_capture_done == true) {
        Serial.print(round((float) g_pulselen / PULSE_LEN_STEPS) * PULSE_LEN_STEPS);
        Serial.print(" ");
        for (uint32_t i = 0; i < g_time_idx; i++) {
            uint32_t pulses = round((float) g_times[i] / g_pulselen);
            // as the pulse length is the average of all short values, we could get 0 here
            if (pulses == 0) {
                pulses = 1;
            }
            Serial.print(pulses);
            Serial.print(" ");
        }
        Serial.println();
        /*
          Serial.println("waiting for 5sec than sending the same code");
          delay(5000);
          for (int x = 0; x < 10; x++) {
          int out = 1;
          int pulselen = round((float) g_pulselen / PULSE_LEN_STEPS) * PULSE_LEN_STEPS;
          for (uint32_t i = 0; i < g_time_idx; i++) {
          uint32_t pulses = round((float) g_times[i] / g_pulselen) * pulselen;
          digitalWrite(PIN, out);
          delayMicroseconds(pulses);
          out = out? 0: 1;
          }
          }
          Serial.println("done");
        */
        g_last = 0;
        g_time_idx = 0;
        g_sync_count = 0;
        g_pulselen = 0;
        g_capture_done = false;
        g_capturing = false;
    }
}
