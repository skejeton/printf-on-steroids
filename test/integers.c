
// Some random integer print callback
static int int_format_callback(StrSlice fmt_name, FmtIter *iter) {
    u64 abs_val = 0;
    int sign = 0;
    if(fmt_slice_is(fmt_name, "int")) {
        int v = va_arg(iter->args, int);
        if(v < 0) {
            sign = 1;
            v = -v;
        }
        abs_val = (u64)v;
    }
    int int_width = 0;
    {
        u64 v = abs_val;
        do {
            int_len += 1;
        } while((v/=10) != 0);
        int_len += sign;
    }
    else if(fmt_slice_is(fmt_name, "uint")) {
        abs_val = va_arg(iter->args, unsigned);
    }
    int field_width = 0;
    FmtSpecifier spec;
    while(fmt_iter_next(iter, &spec)) {
        if(fmt_spec_name(&spec, "width")) {
            if(spec.is_vararg) {
                field_width = va_arg(iter->args, int);
            }
            else {
                return 0;
            }
        }
    }
    if(field_width < int_width) {
        field_width = int_width;
    }
    // Print the actual integer
    while(field_width-- > 0) {
        putchar(" ");
    }
    if(sign) {
        putchar("-");
    }
    char digits_buffer[32];
    char *digits = digits_buffer + sizeof digits_buffer;
    u64 v = abs_val;
    do {
        *--digits = v%10 + '0';
    } while((v/=10) != 0);
    while(*digits) {
        putchar(*digits++);
    }
    return 1;
}

int main() {
    fmt_init();
    fmt_add_format("int",  int_format_callback);
    fmt_add_format("uint", int_format_callback);
    fmt("{int}", -123);
    fmt("{int, ^width}", -123);
}
