


// Debugging: enable "assert"
#include "stm32f10x_type.h"
#undef assert
#define assert(expr) ((expr) ? (void)0 : assert_failed((u8*)__FILE__, __LINE__))
void assert_failed(u8* file, u32 line);

