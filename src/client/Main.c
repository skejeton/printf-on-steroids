#include "P2.c"

int main() {
  P2_Init();

#if 0
  P2_Print("Oh and also, the logs preserve across runs.");
#else
  for (int i = 0; i < 2; ++i) {
    P2_Print("Printf2 (Printf on steroids) **Now with faster output**.", i);
    P2_Print("This is a library/GUI package that enchances printf debugging.", i);
    P2_Print("The main drawback of printf is that the data is output into the console.", i);
    P2_Print("This library allows you to sort the logs.", i);
    P2_Print("It's planned for this library to also visualize things printf can't do, like bitmaps.", i);
    P2_Print("$For now in sorting properties you can choose to either filter by inclusion.", i);
    P2_Print("Or_by_exclusion", i);
    P2_Print("@Or_by_both", i);
    P2_Print("That's all I have for now! Cheers :P", i);
  }
#endif

  for (int i = 0; i < 100000; ++i) {
    P2_Print("PlayerPosition %d %d", i, i);
    sleep(1);
  }

  P2_Terminate();
  return 0;
}