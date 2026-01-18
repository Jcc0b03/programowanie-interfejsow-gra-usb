#include "../lib/padInput.h"
#include <windows.h>

using namespace std;

void setCursorPosition(int x, int y) {
    static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout.flush();
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hOut, coord);
}

int main(){
    SetConsoleOutputCP(CP_UTF8);

    PadInput pad;
    cout << "Pad jest podłączony: " << pad.isConnected() << endl;
    
    //pad.runMaskWizard();
    //return 0;

    system("cls");

    while(true){
        pad.update();

        setCursorPosition(0, 0);

        pad.print();

        ButtonState* buttonState = pad.getButtonState("1");
        if(buttonState){
            cout << "Button 1: " << buttonState->currentState << endl;
        }

        Sleep(16);
    }

    return 0;
}