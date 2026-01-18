#pragma once
#include <windows.h> // Wymagane dla typów HANDLE, BYTE itp.
#include <iostream>
#include <string>

struct ButtonState {
    bool currentState;
    bool previousState;

    int sameStateCount = 0;

    void update(bool isPressed);
};

struct AxisState {
    int currentState[2]; //[X, Y]
    int previousState[2];

    int sameStateCount[2] = {0, 0};
    void update(int x, int y);
    void print();
};

struct ButtonMapping{
    int byteIndex;
    int bitMask;
};

struct AxisMapping{
    int byteXIndex;
    int byteYIndex;
};

class PadInput {
    public:
        PadInput();   // Konstruktor - tu nawiążemy połączenie
        ~PadInput();  // Destruktor - tu zamkniemy połączenie

        void update(); // Pobranie nowych danych z pada
        void print();  // Wyświetlenie statusu w konsoli
        bool isConnected(); // Pomocnicza funkcja sprawdzająca czy pad działa
        void runMaskWizard();

        ButtonState getButtonState(char* buttonId);
        AxisState getAxisState(char* axisId);

    private:
        HANDLE hidDeviceObject = INVALID_HANDLE_VALUE; // Uchwyt do pliku urządzenia
        BYTE inputBuffer[64] = {0};   // Bufor na dane z pada
        bool connected = false;         // Flaga statusu
        
        // Stała dla Twojego urządzenia
        const char* TARGET_VID_PID = "vid_0079&pid_0006";

        const char* buttons[12] = {"1", "2", "3", "4", "start", "select", "L1", "L2", "R1", "R2", "analog_L", "analog_R"};
        const char* axis[2] = {"analog_L", "analog_R"};

        //mapping (jakaś tablica który bajt i jaka maska bitowa)
        ButtonMapping buttonsMapping[12] = {
            {6, 0x10}, // 1
            {6, 0x20}, // 2
            {6, 0x40}, // 3
            {6, 0x80}, // 4
            {7, 0x20}, // start
            {7, 0x10}, // select
            {7, 0x01}, // L1
            {7, 0x04}, // L2
            {7, 0x02}, // R1
            {7, 0x08}, // R2
            {7, 0x40}, // analog_L
            {7, 0x80}, // analog_R
        };

        AxisMapping axisMapping[2] = {
            {1, 2},
            {4, 5}
        };

        const char* dpad[4] = {"up", "down", "left", "right"};

        ButtonState buttonStates[12];
        ButtonState dpadState[4];
        AxisState axisStates[2];
};