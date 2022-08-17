#include "Core.c"

#define P2_Print(s) Core_OutputLog((LogEntry){.line=__LINE__,.file=__FILE__,.data=(s)})

int main() {
  Core_Init();

#if 0
  Core_OutputLog("Oh and also, the logs preserve across runs.");
#else

  P2_Print("Printf2 (Printf on steroids) **Now with faster output**.");
  P2_Print("This is a library/GUI package that enchances printf debugging.");
  P2_Print("The main drawback of printf is that the data is output into the console.");
  P2_Print("This library allows you to sort the logs.");
  P2_Print("It's planned for this library to also visualize things printf can't do, like bitmaps.");
  P2_Print("$For now in sorting properties you can choose to either sort by inclusion.");
  P2_Print("Or_by_exclusion");
  P2_Print("@Or_by_both");
  P2_Print("That's all I have for now! Cheers :P");
#endif

  Core_Deinit();
  return 0;
}