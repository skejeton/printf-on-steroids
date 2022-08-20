// IT DOES NOT WORK YET. DON'T USE IT.
#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "Common.h"

static struct {
  Display *dpy;
  int screen, child_pid;
  Window they, me, root_window;
  Atom clipboard, utf8, stolen;
  unsigned char *property_data;
} CLIPBOARD_DATA;

sem_t CLIPBOARD_SEMAPHORE_CLIENT;
sem_t CLIPBOARD_SEMAPHORE_SERVER;

// The X11 functions are real jank:
// 
// XGetWindowProperty(
//   Display, The window, Selection ID,
//   Starting offset, Size to read, Delete property?, Type of the property,
//   *Actual type of the property, *Format, *Size, *Remaining bytes, *Output string
// )
char *GetSelectionValue(Window window, Atom selection) {
  Atom type, dummy;
  int format;
  unsigned long size, offset;

  if (CLIPBOARD_DATA.property_data != NULL) {
    // Cleanup old data.
    XFree(CLIPBOARD_DATA.property_data);
    CLIPBOARD_DATA.property_data = NULL;
  }

  const Atom INCREMENTAL = XInternAtom(CLIPBOARD_DATA.dpy, "INCR", False);

  XGetWindowProperty(CLIPBOARD_DATA.dpy, window, selection, 0, 0, False, AnyPropertyType, &type, &format, &offset, &size, &CLIPBOARD_DATA.property_data);

  // Free "data" here, we didn't read any data because the size was 0, it was still allocated tho.
  XFree(CLIPBOARD_DATA.property_data);

  if (type == INCREMENTAL) {
    // No incremental fetching support.
    LOG_ERROR("Tried to fetch incrementally. Can't.");
  }

  LOG_INFO("Selection size is %ld bytes.", size);

  // Fetch data now.
  // We pass offset twice into output because we don't care about it, and dont' want to overwrite the size.
  XGetWindowProperty(CLIPBOARD_DATA.dpy, window, selection, 0, size, False, type, &dummy, &format, &offset, &offset, &CLIPBOARD_DATA.property_data);

  // Tell our own window we were a good boy and read the data :P.
  XDeleteProperty(CLIPBOARD_DATA.dpy, window, selection);

  return (char*)CLIPBOARD_DATA.property_data;
}

void ClipboardThread(Display *dpy) {
  while (1) {
    sem_wait(&CLIPBOARD_SEMAPHORE_CLIENT);
    LOG_INFO("Got signal to handle clipboard event.");
    XEvent event;
    XNextEvent(CLIPBOARD_DATA.dpy, &event);

    switch (event.type) {
      case SelectionNotify:
        LOG_INFO("Got notification about selections.");
        if (event.xselection.property == None) {
          LOG_ERROR("Can not convert formats.");
        } else {
          LOG_INFO("Selection value: \x1b[33m%s\x1b[0m", GetSelectionValue(CLIPBOARD_DATA.me, CLIPBOARD_DATA.stolen));
        }
        sem_post(&CLIPBOARD_SEMAPHORE_SERVER);
    }
  }

}

void RequestClipboardData() {
  // Find selection owner
  CLIPBOARD_DATA.they = XGetSelectionOwner(CLIPBOARD_DATA.dpy, CLIPBOARD_DATA.clipboard);
  if (CLIPBOARD_DATA.they == None) {
    LOG_ERROR("No selection owner.");
  }

  // Read owner window title
  char *window_name = NULL;
  XFetchName(CLIPBOARD_DATA.dpy, CLIPBOARD_DATA.they, &window_name);
  if (window_name == NULL){
    LOG_INFO("Owner of clipboard doesn't have a title, their id is 0x%lx.", CLIPBOARD_DATA.they);
  } else {
    LOG_INFO("Owner of clipboard is: \"%s\".", window_name);
  }
  XFree(window_name);

  // Event request
  XConvertSelection(CLIPBOARD_DATA.dpy, CLIPBOARD_DATA.clipboard, CLIPBOARD_DATA.utf8, CLIPBOARD_DATA.stolen, CLIPBOARD_DATA.me, CurrentTime);

  LOG_INFO("Requesting clipboard data.");
  // Request data
  sem_post(&CLIPBOARD_SEMAPHORE_CLIENT);
  sem_wait(&CLIPBOARD_SEMAPHORE_SERVER);
  LOG_INFO("Semaphore gives OK.");
}

void ClipboardInit() {
  XInitThreads();
  sem_init(&CLIPBOARD_SEMAPHORE_CLIENT, 1, 0);
  sem_init(&CLIPBOARD_SEMAPHORE_SERVER, 1, 0);
  CLIPBOARD_DATA.dpy = XOpenDisplay(NULL);

  if (CLIPBOARD_DATA.dpy == NULL) {
    LOG_ERROR("Can not open display.");
  }

  LOG_INFO("Opened display.");

  // Init some atoms
  CLIPBOARD_DATA.utf8 = XInternAtom(CLIPBOARD_DATA.dpy, "UTF8_STRING", False);
  CLIPBOARD_DATA.clipboard = XInternAtom(CLIPBOARD_DATA.dpy, "CLIPBOARD", False);
  CLIPBOARD_DATA.stolen = XInternAtom(CLIPBOARD_DATA.dpy, "STOLEN", False);
  CLIPBOARD_DATA.screen = DefaultScreen(CLIPBOARD_DATA.dpy);
  CLIPBOARD_DATA.root_window = RootWindow(CLIPBOARD_DATA.dpy, CLIPBOARD_DATA.screen);
  CLIPBOARD_DATA.me = XCreateSimpleWindow(CLIPBOARD_DATA.dpy, CLIPBOARD_DATA.root_window, 0, 0, 1, 1, 0, 0, 0);

  if ((CLIPBOARD_DATA.child_pid = fork())) {
    if (CLIPBOARD_DATA.child_pid < 0) {
      LOG_ERROR("Fork failed.");
    }
  } else {
    LOG_INFO("Fork started.");
    ClipboardThread(CLIPBOARD_DATA.dpy);
  }
}

void ClipboardDeinit() {
  // Deinit
  XCloseDisplay(CLIPBOARD_DATA.dpy);
  LOG_INFO("Closed display.");
}

