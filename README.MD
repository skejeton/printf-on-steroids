# Printf2

Printf2 is an off-host debugging monitor. It has the potential to visualize 
more than your regular console. As of version v1.1, It can't do anything
besides basic text.

## Example usage.

You can see an example usage of this library in `examples/pong/` directory.

[Pong Example Video](./examples/pong/video.mp4)

## How to use

You need to link with `libprintf2.a` (Linux) or `printf2.lib` (Windows). 
Add the P2.h header in any place convenient in your project.

An example program:

```
#include "P2.h"

int main() {
  P2_Init();

  P2_Print("Hello! %d", 123);

  P2_Deinit();
}
```

First in `P2_Init` it will try connecting to the server for 5 seconds.
If it can't find a server, it will resume the program without logging anything.

Launch Printf2 server, you should find it in GUI folder if you download the 
GitHub package provided. (Or packaged with `make package`).

A window should appear. Try launching the example code again.
You will see a "Hello 123" log appear.

## GUI Options

| Name       | Description
|------------|--------------------------------
| Filter     | Searches for a specific log, including their line number and file name. 
| Monitor    | Shows related logs of last generation only. (Basically shows the only last log executed on same file:line).
| Group/File | Groups logs in files instead of flat choronological representation.
| Group/None | Groups logs in chronological order, independent of the file.
| Clear      | Clear the log screen.

## Additional API functions.

For additional API functions don't hesitate to look into `P2.h` as it contains most of the documentation.

| Name          | Description
|---------------|--------------------------------
| P2_GetStatus  | Returns current status of the connection as `P2_Status`, if `P2_INACTIVE` is returned, you may call `P2_Init` again.
