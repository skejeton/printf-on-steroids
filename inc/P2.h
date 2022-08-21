#ifndef H_PRINTF2_P2
#define H_PRINTF2_P2

#include <stdarg.h>

#if !(defined(__clang__) || defined(__GNUC__))
#define __attribute__(x) 
#endif

/* enum P2_InitStatus
//
// Status code from P2_Init() call.
// Read P2_Init() documentation for more info.
*/
enum P2_InitStatus {
  P2_INIT_OK,
  P2_INIT_CONNECTFAIL,
  P2_INIT_WASINITFAIL,
  P2_INIT_INITFAIL,
}
typedef P2_InitStatus;

/* enum P2_Status
//
// Activity status of Printf2.
//   * P2_INACTIVE :: The log monitor is disconnected.
//   * P2_ACTIVE   :: The log monitor is connected.
*/
enum P2_Status {
  P2_INACTIVE,
  P2_ACTIVE
}
typedef P2_Status;

/* P2_Init() -> Status code (P2_InitStatus)
//
// Initializes Printf2, attempting to connect to main server.
//
// Returns status codes:
//   * P2_INIT_OK             :: Initialization OK.
//   * P2_INIT_STARTFAIL      :: Failed to connect to the server.
//   * P2_INIT_WASRUNNINGFAIL :: Printf2 is already connected.
//   * P2_INIT_INITFAIL       :: Failed to initialize one of required Printf2 subsystems.
*/
int P2_Init();

/* P2_GetStatus() -> Status code (P2_Status)
// 
// Returns status of the current connection. 
// You may call P2_Init to re-connect again.
*/
int P2_GetStatus();

/* P2_Deinit();
//
// De initializes Printf2. 
*/
void P2_Deinit();

/*
// P2_Print_(Line, File, FormatString, ...)
//
// The underlying function for P2_Print(), don't use it unless you know what you're doing.
*/
__attribute__((format(printf, 3, 4)))
void P2_Print_(int line, const char *file, const char *fmt, ...);

/*
// P2_Print(FormatString, ...)
//
// Functions like printf(), outputs the logs into the server.
*/
#define P2_Print(...) P2_Print_(__LINE__, __FILE__, __VA_ARGS__)

/* P2_Printv_(Line, File, FormatString, VaList)
//
// va_list alternative to P2_Print.
// May be used by custom formatters to output logs dynamically.
*/
void P2_Printv_(int line, const char *file, const char *fmt, va_list va);

#endif // H_PRINTF2_P2
