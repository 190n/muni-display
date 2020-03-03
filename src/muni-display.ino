#include <math.h>
#include <Adafruit_HX8357.h>
#include <Adafruit_mfGFX.h>

#include "RobotoBold32.h"
#include "better-text.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// #define BACKGROUND 0x39e9
#define BACKGROUND 0x0000
#define TEXT 0xd6dc
#define BLUE 0x54bc
#define RED 0xf24a

#define WIDTH 480
#define HEIGHT 320
#define PADDING 16
#define TEXT_SIZE 32
#define LINE_HEIGHT 40

#define PI 3.14159265f
#define TWO_PI 6.28318531f

// PI / 6
#define HALF_NOTCH_WIDTH 0.52359878f

#define OUTER_RADIUS (PADDING * 3 / 4)
#define INNER_RADIUS (PADDING * 3 / 8)

Adafruit_HX8357 tft = Adafruit_HX8357(D6, D7, D5, D3, -1, D4);

typedef struct {
    uint8_t index;
    String name;
    uint16_t* predictions;
    uint16_t xPos;
} Route;

Route knownRoutes[32];

uint8_t numAwaiting = 0;

uint16_t spinnerFrame = 1;

void drawSpinnerFirstFrame(int16_t x, int16_t y, uint16_t color) {
    for (int16_t ix = x - OUTER_RADIUS; ix < x + OUTER_RADIUS; ix++) {
        for (int16_t iy = y - OUTER_RADIUS; iy < y + OUTER_RADIUS; iy++) {
            float distance = sqrtf((ix - x) * (ix - x) + (iy - y) * (iy - y));
            if (distance >= INNER_RADIUS && distance <= OUTER_RADIUS) {
                float angle = atan2f(iy - y, ix - x);
                if (fabsf(angle) > HALF_NOTCH_WIDTH) {
                    tft.drawPixel(ix, iy, color);
                }
            }
        }
    }
}

void iterateSpinner(int16_t x, int16_t y, uint16_t color, uint16_t clearColor, uint8_t i) {
    float angle = TWO_PI - (i % 32) * PI / 16;
    float lastAngle = TWO_PI - ((i - 1) % 32) * PI / 16;
    // width = pi/3
    // last frame: from -7pi/16 - pi/6 to -7pi/16 + pi/6
    // this frame: from -8pi/16 - pi/6 to -8pi/16 + pi/6
    //  draw pixels where -8pi/16 - pi/6 < angle < -7pi/16 - pi/6
    // erase pixels where -8pi/16 + pi/6 < angle < -7pi/16 + pi/6

    float minErase = fmodf(angle - HALF_NOTCH_WIDTH + PI, TWO_PI) - PI,
        maxErase = fmodf(lastAngle - HALF_NOTCH_WIDTH + PI, TWO_PI) - PI,
        minDraw = fmodf(angle + HALF_NOTCH_WIDTH + PI, TWO_PI) - PI,
        maxDraw = fmodf(lastAngle + HALF_NOTCH_WIDTH + PI, TWO_PI) - PI;


    for (int16_t ix = x - OUTER_RADIUS; ix < x + OUTER_RADIUS; ix++) {
        for (int16_t iy = y - OUTER_RADIUS; iy < y + OUTER_RADIUS; iy++) {
            float distance = sqrtf((ix - x) * (ix - x) + (iy - y) * (iy - y));
            if (distance >= INNER_RADIUS && distance <= OUTER_RADIUS) {
                float ia = atan2f(y - iy, ix - x);

                if (minDraw < ia && ia < maxDraw) {
                    tft.drawPixel(ix, iy, color);
                } else if (maxDraw < minDraw && (ia > minDraw || ia < maxDraw)) {
                    tft.drawPixel(ix, iy, color);
                } else if (minErase < ia && ia < maxErase) {
                    tft.drawPixel(ix, iy, clearColor);
                } else if (maxErase < minErase && (ia > minErase || ia < maxErase)) {
                    tft.drawPixel(ix, iy, clearColor);
                }
            }
        }
    }
}

void setup() {
    tft.begin(HX8357D);
    tft.setRotation(1);
    tft.fillScreen(BACKGROUND);

    for (int i = 0; i < 32; i++) {
        knownRoutes[i].index = 255;
    }

    Particle.connect();
    drawSpinnerFirstFrame(WIDTH - PADDING - OUTER_RADIUS, HEIGHT - PADDING - OUTER_RADIUS, TEXT);

    while (!Particle.connected()) {
        iterateSpinner(WIDTH - PADDING - OUTER_RADIUS, HEIGHT - PADDING - OUTER_RADIUS, TEXT, BACKGROUND, spinnerFrame);
        spinnerFrame++;
        delay(20);
    }

    Particle.subscribe("hook-response/43_outbound", muniHandler, MY_DEVICES);
    Particle.subscribe("hook-response/44_outbound", muniHandler, MY_DEVICES);
    Particle.subscribe("hook-response/n_inbound", muniHandler, MY_DEVICES);

    numAwaiting = 3;
    Particle.publish("43_outbound", PRIVATE);
    Particle.publish("44_outbound", PRIVATE);
    Particle.publish("n_inbound", PRIVATE);
    // now loop() will take over the animation as numAwaiting > 0
}

void loop() {
    if (numAwaiting > 0) {
        // animation
        iterateSpinner(WIDTH - PADDING - OUTER_RADIUS, HEIGHT - PADDING - OUTER_RADIUS, TEXT, BACKGROUND, spinnerFrame);
        spinnerFrame++;
        // small delay
        delay(20);
    } else {
        delay(15000);
        numAwaiting = 3;
        Particle.publish("43_outbound", PRIVATE);
        Particle.publish("44_outbound", PRIVATE);
        Particle.publish("n_inbound", PRIVATE);
        // draw first frame of animation
        drawSpinnerFirstFrame(WIDTH - PADDING - OUTER_RADIUS, HEIGHT - PADDING - OUTER_RADIUS, TEXT);
        spinnerFrame = 1;
    }
}

void showPredictions(Route r) {
    tft.fillRect(r.xPos, LINE_HEIGHT * r.index + PADDING, WIDTH - PADDING - r.xPos, TEXT_SIZE, BACKGROUND);
    char buf[14];
    sprintf(buf, "%d, %d, %d", r.predictions[0], r.predictions[1], r.predictions[2]);
    uint16_t color = r.predictions[0] < 5 ? RED : BLUE;
    drawText(tft, r.xPos, LINE_HEIGHT * r.index + PADDING, WIDTH - PADDING, LINE_HEIGHT, RobotoBold32, color, BACKGROUND, buf);
}

// example response: 0|43 Inbound|8|18|28|
// index (unique identifier for the route) = 0
// name (displayed to user)                = "43 Inbound"
// predictions                             = [8, 18, 28]
// so we write "43 Inbound: 8, 18, 28"
void muniHandler(const char* name, const char* data) {
    char buf[256];
    strlcpy(buf, data, 255); // so we have a char* to work with
    uint8_t index = atoi(strtok(buf, "|"));
    Route* r = &knownRoutes[index];

    if (r->index == 255) {
        // first time we've seen this route
        r->index = index;
        // set name
        r->name = String(strtok(NULL, "|"));
        // draw name
        uint16_t x = drawText(tft, PADDING, LINE_HEIGHT * index + PADDING, WIDTH - PADDING, LINE_HEIGHT, RobotoBold32, TEXT, BACKGROUND, r->name);
        r->xPos = drawText(tft, x, LINE_HEIGHT * index + PADDING, WIDTH - PADDING, LINE_HEIGHT, RobotoBold32, TEXT, BACKGROUND, ": ");
        // set up array for predictions
        r->predictions = new uint16_t[3];
    } else {
        // call strtok to get past the route name
        strtok(NULL, "|");
    }

    char* token;
    uint16_t p0 = UINT16_MAX, p1 = UINT16_MAX, p2 = UINT16_MAX;
    while ((token = strtok(NULL, "|")) != NULL) {
        int p = atoi(token);
        if (p <= p0) {
            p2 = p1;
            p1 = p0;
            p0 = p;
        } else if (p <= p1) {
            p2 = p1;
            p1 = p;
        } else if (p <= p2) {
            p2 = p;
        }
    }

    r->predictions[0] = p0;
    r->predictions[1] = p1;
    r->predictions[2] = p2;
    showPredictions(*r);
    numAwaiting--;

    if (numAwaiting <= 0) {
        // erase spinner
        tft.fillRect(WIDTH - PADDING - OUTER_RADIUS - OUTER_RADIUS, HEIGHT - PADDING - OUTER_RADIUS - OUTER_RADIUS, OUTER_RADIUS * 2, OUTER_RADIUS * 2, BACKGROUND);
    }
}
