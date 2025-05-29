#include <M5AtomS3.h>
#include "USB_Conexion.h"
#include <BleConnectionStatus.h>
#include <BleCompositeHID.h>
#include <XboxGamepadDevice.h>
#include <NimBLEDevice.h>

// --- グローバル定数定義 ---
constexpr int MY_NOTE_MIN = 48;
constexpr int MY_NOTE_MAX = 72;
constexpr int NOTE_RANGE  = MY_NOTE_MAX - MY_NOTE_MIN;
constexpr char DEVICE_NAME[] = "Midi2XInput";
constexpr char DEVICE_MANUFACTURER[] = "Onigiri";

// --- グローバル変数 ---
XboxGamepadDevice *gamepad = nullptr;
BleCompositeHID compositeHID(DEVICE_NAME, DEVICE_MANUFACTURER, 100);

// --- HSV→RGB変換（0-255出力）---
void hsv2rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    float r1 = 0, g1 = 0, b1 = 0;
    if      (h <  60) { r1 = c; g1 = x; }
    else if (h < 120) { r1 = x; g1 = c; }
    else if (h < 180) { g1 = c; b1 = x; }
    else if (h < 240) { g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; b1 = c; }
    else              { r1 = c; b1 = x; }
    r = static_cast<uint8_t>((r1 + m) * 255);
    g = static_cast<uint8_t>((g1 + m) * 255);
    b = static_cast<uint8_t>((b1 + m) * 255);
}

// --- MIDI受信時の色・LED処理 ---
void showNoteColor(int note, int velocity) {
    int n = constrain(note - MY_NOTE_MIN, 0, NOTE_RANGE);
    float h = (240.0f * n) / NOTE_RANGE;
    uint8_t r, g, b;
    hsv2rgb(h, 1.0f, 0.8f, r, g, b); // 彩度100%、明度80%
    uint32_t color = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(r) << 8) | b; // GRB
    uint8_t val = map(velocity, 1, 127, 20, 100); // Velocityで輝度
    AtomS3.dis.setBrightness(val);
    AtomS3.dis.drawpix(color);
    AtomS3.update();
}

// --- MIDI OFF時のLED消灯 ---
void clearNoteColor() {
    AtomS3.dis.drawpix(0x000000);
    AtomS3.update();
}

// --- USB_Conexionを継承し、MIDIイベント処理を行うクラス ---
class MyUSBConexion : public USB_Conexion {
public:
    void onMidiDataReceived(const uint8_t* data, size_t length) override {
        // USB-MIDIの仕様で4バイト/3バイトパケット両方に対応
        const uint8_t* midiData = (length == 3) ? data : (length >= 4 ? data + 1 : nullptr);
        if (!midiData) return;

        uint8_t midiStatus = midiData[0] & 0xF0;
        uint8_t note = midiData[1];
        uint8_t velocity = midiData[2];

        if (midiStatus == 0x90 && velocity > 0) {
            showNoteColor(note, velocity);
            onMidiNoteReceived(note, true);
        } else if (midiStatus == 0x80 || (midiStatus == 0x90 && velocity == 0)) {
            clearNoteColor();
            onMidiNoteReceived(note, false);
        }
    }
};
MyUSBConexion myUSB;

// --- Xbox入力タイプ ---
enum XboxInputType : uint8_t {
    BUTTON, DPAD, TRIGGER, THUMBSTICK
};

// --- MIDIノート→Xbox入力へのマッピング情報 ---
struct XboxInputMapping {
    XboxInputType type;
    uint16_t button = 0;                                               // BUTTON
    XboxDpadFlags dpad = (XboxDpadFlags)0;                             // DPAD
    struct { bool left = false; bool positive = false; } trigger;      // left/right, on/off
    struct { bool left = false; int16_t x = 0; int16_t y = 0; } thumb; // left/right, x, y
};

// --- キーマッピング配列１ ---
const XboxInputMapping noteToInput1[25] = {
    {TRIGGER,     0,                  (XboxDpadFlags)0,   {1, 1}},                         // 48:LT
    {BUTTON,      0},                                                                      // 49:未使用
    {TRIGGER,     0,                  (XboxDpadFlags)0,   {0, 1}},                         // 50:RT
    {BUTTON,      0},                                                                      // 51:未使用
    {DPAD,        0,                  XboxDpadFlags::SOUTH},                               // 52:↓
    {BUTTON,      XBOX_BUTTON_A},                                                          // 53:A
    {BUTTON,      0},                                                                      // 54:未使用
    {DPAD,        0,                  XboxDpadFlags::WEST},                                // 55:←
    {BUTTON,      0},                                                                      // 56:未使用
    {BUTTON,      XBOX_BUTTON_X},                                                          // 57:X
    {BUTTON,      0},                                                                      // 58:未使用
    {DPAD,        0,                  XboxDpadFlags::NORTH},                               // 59:↑
    {BUTTON,      XBOX_BUTTON_Y},                                                          // 60:Y
    {BUTTON,      0},                                                                      // 61:未使用
    {DPAD,        0,                  XboxDpadFlags::EAST},                                // 62:→
    {BUTTON,      0},                                                                      // 63:未使用
    {BUTTON,      XBOX_BUTTON_B},                                                          // 64:B
    {BUTTON,      XBOX_BUTTON_LB},                                                         // 65:LB
    {BUTTON,      0},                                                                      // 66:未使用
    {BUTTON,      XBOX_BUTTON_RB},                                                         // 67:RB
    {BUTTON,      0},                                                                      // 68:未使用
    {THUMBSTICK,  0,                  (XboxDpadFlags)0,   {0,0}, {1, -XBOX_STICK_MAX, 0}}, // 69:左←
    {BUTTON,      0},                                                                      // 70:未使用
    {THUMBSTICK,  0,                  (XboxDpadFlags)0,   {0,0}, {0, -XBOX_STICK_MAX, 0}}, // 71:右←
    {THUMBSTICK,  0,                  (XboxDpadFlags)0,   {0,0}, {1, XBOX_STICK_MAX, 0}},  // 72:左→
};

// --- キーマッピング配列２ ---
const XboxInputMapping noteToInput2[25] = {
    {DPAD,        0,                  XboxDpadFlags::SOUTH},                               // 48:↓
    {BUTTON,      0},                                                                      // 49:未使用
    {DPAD,        0,                  XboxDpadFlags::WEST},                                // 50:←
    {BUTTON,      0},                                                                      // 51:未使用
    {DPAD,        0,                  XboxDpadFlags::NORTH},                               // 52:↑
    {THUMBSTICK,  0,                  (XboxDpadFlags)0, {0,0}, {1, 0, XBOX_STICK_MAX}},    // 53:左↓
    {BUTTON,      0},                                                                      // 54:未使用
    {THUMBSTICK,  0,                  (XboxDpadFlags)0, {0,0}, {1, -XBOX_STICK_MAX, 0}},   // 55:左←
    {BUTTON,      0},                                                                      // 56:未使用
    {BUTTON,      XBOX_BUTTON_LB},                                                         // 57:LB
    {BUTTON,      0},                                                                      // 58:未使用
    {TRIGGER,     0,                  (XboxDpadFlags)0,   {1, 1}},                         // 59:LT
    {THUMBSTICK,  0,                  (XboxDpadFlags)0, {0,0}, {0, 0, XBOX_STICK_MAX}},    // 60:右↓
    {BUTTON,      0},                                                                      // 61:未使用
    {THUMBSTICK,  0,                  (XboxDpadFlags)0, {0,0}, {0, XBOX_STICK_MAX, 0}},    // 62:右→
    {BUTTON,      0},                                                                      // 63:未使用
    {THUMBSTICK,  0,                  (XboxDpadFlags)0, {0,0}, {0, 0, -XBOX_STICK_MAX}},   // 64:右↑
    {BUTTON,      XBOX_BUTTON_A},                                                          // 65:A
    {BUTTON,      0},                                                                      // 66:未使用
    {BUTTON,      XBOX_BUTTON_B},                                                          // 67:B
    {BUTTON,      0},                                                                      // 68:未使用
    {BUTTON,      XBOX_BUTTON_Y},                                                          // 69:Y
    {BUTTON,      0},                                                                      // 70:未使用
    {BUTTON,      XBOX_BUTTON_RB},                                                         // 71:RB
    {TRIGGER,     0,                  (XboxDpadFlags)0,   {0, 1}},                         // 72:RT
};

// --- マッピング切替フラグ ---
bool mapping2Mode = false;
void toggleMappingMode() { mapping2Mode = !mapping2Mode; }

// --- Xboxコントローラー操作 ---
void onMidiNoteReceived(uint8_t note, bool on) {
    if(note < MY_NOTE_MIN || note > MY_NOTE_MAX) return;
    const XboxInputMapping* mapping = mapping2Mode ? noteToInput2 : noteToInput1;
    const XboxInputMapping& input = mapping[note - MY_NOTE_MIN];

    switch(input.type) {
        case BUTTON:
            if(input.button) {
                if(on) gamepad->press(input.button);
                else   gamepad->release(input.button);
            }
            break;
        case DPAD:
            updateDpadFlags(input.dpad, on);
            break;
        case TRIGGER:
            if(input.trigger.left)
                gamepad->setLeftTrigger(on ? XBOX_TRIGGER_MAX : 0);
            else
                gamepad->setRightTrigger(on ? XBOX_TRIGGER_MAX : 0);
            break;
        case THUMBSTICK:
            if(input.thumb.left)
                gamepad->setLeftThumb(on ? input.thumb.x : 0, on ? input.thumb.y : 0);
            else
                gamepad->setRightThumb(on ? input.thumb.x : 0, on ? input.thumb.y : 0);
            break;
    }
    gamepad->sendGamepadReport();
}

// --- DPad（同時押し対応） ---
uint8_t dpadCurrentFlags = 0;
void updateDpadFlags(XboxDpadFlags flag, bool on) {
    if (on)
        dpadCurrentFlags |= static_cast<uint8_t>(flag);     // 押す
    else
        dpadCurrentFlags &= ~static_cast<uint8_t>(flag);    // 離す

    if (dpadCurrentFlags)
        gamepad->pressDPadDirectionFlag(static_cast<XboxDpadFlags>(dpadCurrentFlags));
    else
        gamepad->releaseDPad();
}

// --- セットアップ ---
void setup() {
    AtomS3.begin(true);
    AtomS3.dis.setBrightness(100);
    AtomS3.dis.drawpix(0x000000);

    // USBホスト初期化
    myUSB.begin();

    // Gamepad BLE HID初期化
    auto* config = new XboxOneSControllerDeviceConfiguration();
    BLEHostConfiguration hostConfig = config->getIdealHostConfiguration();
    gamepad = new XboxGamepadDevice(config);
    compositeHID.addDevice(gamepad);
    compositeHID.begin(hostConfig);

    // Bluetoothの一覧に表示されない場合、ここからコメントアウト ↓↓↓↓↓
    // BLEアドバタイズデータ
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();
    NimBLEAdvertisementData advData;
    advData.setName(DEVICE_NAME);
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
    // Bluetoothの一覧に表示されない場合、ここまでコメントアウト ↑↑↑↑↑
}

// --- 物理ボタンA：長押しでボタン配置切り替え、短押しでマッピング切り替え ---
unsigned long btnPressStart = 0;
bool btnWasPressed = false;
bool thumbInitialized = false;

void loop() {
    AtomS3.update();
    myUSB.task();

    // 接続時の初期化
    if(compositeHID.isConnected() && !thumbInitialized){
        gamepad->setLeftThumb(1, 1);
        gamepad->setRightThumb(1, 1);
        gamepad->setLeftThumb(0, 0);
        gamepad->setRightThumb(0, 0);
        gamepad->sendGamepadReport();
        thumbInitialized = true;
    }

    // ボタンA押下（初回だけ判定）
    if (AtomS3.BtnA.isPressed() && !btnWasPressed) {
        btnPressStart = millis();
        btnWasPressed = true;
        gamepad->press(XBOX_BUTTON_RS);  // RSボタン押下
        gamepad->sendGamepadReport();
    }
    // ボタンA離したとき
    if (AtomS3.BtnA.wasReleased()) {
        unsigned long pressDuration = millis() - btnPressStart;
        if (pressDuration < 1000) { // 1秒未満→マッピング切替
            toggleMappingMode();
        }
        gamepad->release(XBOX_BUTTON_RS); // RSボタン離す
        gamepad->sendGamepadReport();
        btnWasPressed = false;
    }
}
