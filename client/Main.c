#include "Core.c"

int main() {
  Core_Init();

#if 0
  Core_OutputLog("Oh and also, the logs preserve across runs.");
#else

  Core_OutputLog("Printf2 (Printf on steroids) **Now with faster output**.");
  Core_OutputLog("This is a library/GUI package that enchances printf debugging.");
  Core_OutputLog("The main drawback of printf is that the data is output into the console.");
  Core_OutputLog("This library allows you to sort the logs.");
  Core_OutputLog("It's planned for this library to also visualize things printf can't do, like bitmaps.");
  Core_OutputLog("$For now in sorting properties you can choose to either sort by inclusion.");
  Core_OutputLog("Or_by_exclusion");
  Core_OutputLog("@Or_by_both");
  Core_OutputLog("That's all I have for now! Cheers :P");
#endif

  Core_Deinit();
  return 0;
}