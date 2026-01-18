#include "PadInput.h"
#include <setupapi.h>
#include <string>
#include <iomanip>
#include <vector>

#pragma comment(lib, "setupapi.lib")

using namespace std;

void ButtonState::update(bool isPressed){
    this->previousState = this->currentState;
    this->currentState = isPressed;

    if(this->currentState == this->previousState){
        this->sameStateCount++;
    } else {
        this->sameStateCount = 0;
    }
}

void AxisState::update(int x, int y){
    this->previousState[0] = this->currentState[0];
    this->previousState[1] = this->currentState[1];

    int calcX = (x * 2) - 255;
    int calcY = (-y * 2) + 255; // Twoja inwersja Y jest OK

    // 3. ZASTOSUJ DEADZONE (Martwą strefę)
    // Wartość 10-20 jest bezpieczna dla tanich padów.
    // Eliminuje to problem -1/1 oraz lekkie zużycie sprężyn w padzie.
    const int DEADZONE = 15; 

    if (abs(calcX) < DEADZONE) {
        this->currentState[0] = 0; // Idealna cisza
    } else {
        this->currentState[0] = calcX;
    }

    if (abs(calcY) < DEADZONE) {
        this->currentState[1] = 0; // Idealna cisza
    } else {
        this->currentState[1] = calcY;
    }
}

void AxisState::print(){
    cout << "X: " << currentState[0] << " Y: " << currentState[1];
}

PadInput::PadInput(){
    //GUID dla urządzeń HID
    GUID hidGuid;

    //lista wszystkich urządzeń HID
    HDEVINFO deviceInfoSet;

    //załadowanie biblioteki HID.dll
    HMODULE hHidLib = LoadLibrary("HID.dll");
    if(!hHidLib){
        cout << "Nie można załadować biblioteki HID.dll" << endl;
    }

    cout << "Pomyślnie załadowano bibliotekę HID.dll" << endl;

    //pobranie funkcji z HID.dll
    typedef void (__stdcall *PHidD_GetHidGuid)(OUT LPGUID HidGuid);
    PHidD_GetHidGuid GetHidGuid = (PHidD_GetHidGuid)GetProcAddress(hHidLib, "HidD_GetHidGuid");

    if(!GetHidGuid){
        cout << "Nie można pobrać funkcji GetHidGuid" << endl;
    }

    cout << "Pomyślnie pobrano funkcję GetHidGuid" << endl;

    //pobranie GUID dla urządzeń HID
    GetHidGuid(&hidGuid);
    cout << "GUID: " << hidGuid.Data1 << "-" << hidGuid.Data2 << "-" << hidGuid.Data3 << "-" << hidGuid.Data4 << endl;

    //pobranie listy wszystkich urządzeń HID
    deviceInfoSet = SetupDiGetClassDevs(
        &hidGuid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if(deviceInfoSet != INVALID_HANDLE_VALUE){
        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        DWORD detailSize = 0;
        DWORD memberIndex = 0;
        while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, memberIndex, &deviceInterfaceData)) {
            memberIndex++;
            SetupDiGetDeviceInterfaceDetail(
                deviceInfoSet,
                &deviceInterfaceData,
                NULL,
                0,
                &detailSize,
                NULL
            );

            PSP_DEVICE_INTERFACE_DETAIL_DATA detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)new BYTE[detailSize];
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (SetupDiGetDeviceInterfaceDetail(
                deviceInfoSet,
                &deviceInterfaceData,
                detail,
                detailSize,
                NULL,
                NULL))
            {
                string path = detail->DevicePath;
                if(path.find(TARGET_VID_PID) != string::npos){
                    cout << "Znaleziono pad: " << path << endl;
                    this->hidDeviceObject = CreateFile(
                        detail->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                    );

                    if(this->hidDeviceObject != INVALID_HANDLE_VALUE){
                        cout << "Pomyślnie otworzono pad" << endl;
                        this->connected = true;

                        free(detail);
                        break;
                    }
                }
            }

            free(detail);
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }

    FreeLibrary(hHidLib);
}

PadInput::~PadInput() {
    if (this->hidDeviceObject != INVALID_HANDLE_VALUE) {
        CloseHandle(this->hidDeviceObject);
        this->hidDeviceObject = INVALID_HANDLE_VALUE;
    }
}

bool PadInput::isConnected() {
    return this->connected;
}

void PadInput::update() {
    if (!connected) return;

    DWORD bytesRead = 0;
    // Odczyt surowych danych
    if (!ReadFile(hidDeviceObject, inputBuffer, sizeof(inputBuffer), &bytesRead, NULL)) return;

    // --- 1. OBSŁUGA ANALOGÓW (Bez zmian) ---
    for(int i=0; i<2; i++){
        this->axisStates[i].update(
            this->inputBuffer[this->axisMapping[i].byteXIndex],
            this->inputBuffer[this->axisMapping[i].byteYIndex]
        );
    }

    // --- 2. OBSŁUGA D-PADA (Hat Switch) ---
    int hatByte = 6; 
    int hatValue = inputBuffer[hatByte] & 0x0F;

    this->dpadState[0].update(hatValue == 0 || hatValue == 1 || hatValue == 7); // UP
    this->dpadState[1].update(hatValue == 4 || hatValue == 3 || hatValue == 5); // DOWN
    this->dpadState[2].update(hatValue == 6 || hatValue == 5 || hatValue == 7); // LEFT
    this->dpadState[3].update(hatValue == 2 || hatValue == 1 || hatValue == 3); // RIGHT


    // --- 3. OBSŁUGA STANDARDOWYCH PRZYCISKÓW ---
    for(int i=0; i<12; i++){
        bool isPressed = (this->inputBuffer[this->buttonsMapping[i].byteIndex] & this->buttonsMapping[i].bitMask) != 0;
        this->buttonStates[i].update(isPressed);
    }
}

void PadInput::print(){
    if (!connected) {
        cout << "Pad niepodlaczony." << endl;
        return;
    }

    cout << "-------analogi--------" << endl;

    // Wyświetlanie analogów
    for(int i=0; i<2; i++){
        cout << this->axis[i] << ": ";
        this->axisStates[i].print();
        cout << "          " << endl;
    }

    cout << "-------przyciski-------" << endl;

    // Wyświetlanie przycisków
    for(int i=0; i<12; i++){
        cout << this->buttons[i] << ": " << this->buttonStates[i].currentState;
        
        if (this->buttonStates[i].sameStateCount > 10 && this->buttonStates[i].currentState) {
            cout << " (HOLD)";
        }
        cout << "          " << endl;
    }
    
    cout << "-------DPAD--------" << endl;
    for(int i=0; i<4; i++){
        cout << this->dpad[i] << ": " << this->dpadState[i].currentState;
        
        if (this->dpadState[i].sameStateCount > 10 && this->dpadState[i].currentState) {
            cout << " (HOLD)";
        }
        cout << "          " << endl;
    }
    cout << "          " << endl;

}

void PadInput::runMaskWizard() {
    if (!connected) {
        cout << "BLAD: Pad nie jest podlaczony!" << endl;
        return;
    }

    struct DetectedMask {
        int byte;
        int mask;
    };
    vector<DetectedMask> results;

    cout << "=== KREATOR MASEK BITOWYCH ===" << endl;
    cout << "Bede pobieral numer bajtu z Twojej tablicy 'buttonsMapping'." << endl;
    cout << "Twoim zadaniem jest tylko wcisnac odpowiedni przycisk." << endl;
    cout << "----------------------------------------------" << endl;

    // Pobieramy stan początkowy, żeby mieć odniesienie
    update();
    
    // Iterujemy po 12 przyciskach zdefiniowanych w tablicy nazw
    for (int i = 0; i < 12; i++) {
        // 1. Pobieramy bajt, który Ty zdefiniowałeś w kodzie
        int targetByte = buttonsMapping[i].byteIndex;

        // Zabezpieczenie przed wyjściem poza bufor
        if (targetByte < 0 || targetByte >= 64) {
            cout << "BLAD w konfiguracji: Przycisk [" << buttons[i] << "] ma nieprawidlowy bajt " << targetByte << endl;
            results.push_back({targetByte, 0}); // Pusty wynik
            continue;
        }

        // Pobieramy stan spoczynkowy DLA TEGO KONKRETNEGO BAJTU
        // Robimy to tuż przed wciśnięciem, żeby zminimalizować ryzyko błędów
        update();
        BYTE baseline = inputBuffer[targetByte];

        cout << ">>> Wcisnij: [" << buttons[i] << "] (Obserwuje bajt: " << targetByte << ")..." << flush;

        int foundMask = 0;

        // Pętla detekcji
        while (true) {
            update();
            BYTE currentVal = inputBuffer[targetByte];

            // Sprawdzamy czy ten konkretny bajt się zmienił
            if (currentVal != baseline) {
                // XOR wykryje różnicę (czyli wciśnięty bit)
                foundMask = currentVal ^ baseline;
                break; 
            }
            Sleep(10);
        }

        cout << " OK! (Maska: 0x" << hex << uppercase << foundMask << dec << ")" << endl;
        results.push_back({targetByte, foundMask});

        // Czekanie na puszczenie przycisku
        cout << "    Pusc przycisk..." << endl;
        while (true) {
            update();
            // Czekamy aż wartość wróci do bazowej (lub bit zniknie)
            if ((inputBuffer[targetByte] & foundMask) == 0) break;
            Sleep(10);
        }
        Sleep(500); // Krótka pauza
    }

    // --- GENEROWANIE KODU ---
    cout << "\n\n=== GOTOWY KOD DO WKLEJENIA ===" << endl << endl;

    cout << "ButtonMapping buttonsMapping[12] = {" << endl;
    for (size_t i = 0; i < results.size(); i++) {
        cout << "    {" << results[i].byte << ", 0x" 
             << hex << uppercase << setfill('0') << setw(2) << results[i].mask 
             << "}, // " << buttons[i] << endl;
    }
    // Dopełnienie do 12 elementów (jeśli tablica ma 12, a przycisków 14?? no, buttons is 12 now)
    for (size_t i = results.size(); i < 12; i++) {
        cout << "    {0, 0x00}, // (puste)" << endl;
    }
    cout << "};" << dec << endl;

    cout << "\n===============================" << endl;
    system("PAUSE");
    exit(0);
}