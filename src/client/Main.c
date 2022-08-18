#include "P2.c"

int main() {
  P2_Init();

#if 0
  P2_Print("Oh and also, the logs preserve across runs.");
#else
  P2_Print("Printf2 (Printf on steroids) **Now with faster output**.", "test.c");
  P2_Print("This is a library/GUI package that enchances printf debugging.", "test.c");
  P2_Print("The main drawback of printf is that the data is output into the console.", "file2.c");
  P2_Print("This library allows you to sort the logs.", "file3.c");
  P2_Print("It's planned for this library to also visualize things printf can't do, like bitmaps.", "destroy.c");
  P2_Print("$For now in sorting properties you can choose to either filter by inclusion.", "destroy.c");
  P2_Print("Or_by_exclusion", "destrdaasdoy.c");
  P2_Print("@Or_by_both", "test.c");
  P2_Print("That's all I have for now! Cheers :P", "file2.c");
#endif

  P2_Terminate();
  return 0;
}