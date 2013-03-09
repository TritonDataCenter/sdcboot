#include "keyboard.h"

int main()
{
    while (!AltKeyDown());
    
    while (!KeyPressed());
        
//    ClearKeyboard();
    printf("%d", ReadKey());
}
