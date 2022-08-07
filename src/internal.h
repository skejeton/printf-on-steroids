
// This is a sketch of implementation

struct FmtMapping typedef FmtMapping;
struct FmtMapping {
    char const  *fmt_name;
    FmtCallback *callback;
    // If we are searching the whole thing anyway maybe make it a linked list?
    FmtMapping  *next;
};

static FmtMapping *head = NULL;

// Some functions to add and retrieve formats from the list
static int fmt_list_add(FmtMapping mapping);
static int fmt_list_find(StrSlice name, FmtMapping *found);

