#ifndef ESP_HEAP_CAPS_H_STUB
#define ESP_HEAP_CAPS_H_STUB
#include <cstdlib>
#define MALLOC_CAP_8BIT     0
#define MALLOC_CAP_SPIRAM   0
#define MALLOC_CAP_INTERNAL 0
#define MALLOC_CAP_DEFAULT  0
inline void* heap_caps_malloc(size_t s, unsigned) { return std::malloc(s); }
inline void* heap_caps_calloc(size_t n, size_t s, unsigned) { return std::calloc(n, s); }
inline void  heap_caps_free(void* p) { std::free(p); }
inline size_t heap_caps_get_free_size(unsigned) { return 0; }
#endif
