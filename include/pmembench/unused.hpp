#pragma once

#define UNUSED1(z) (void)(z)
#define UNUSED2(y, z) UNUSED1(y), UNUSED1(z)
#define UNUSED3(x, y, z) UNUSED1(x), UNUSED2(y, z)
#define UNUSED4(b, x, y, z) UNUSED2(b, x), UNUSED2(y, z)
#define UNUSED5(a, b, x, y, z) UNUSED2(a, b), UNUSED3(x, y, z)
#define VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)
#define UNUSED_IMPL_(nargs) UNUSED##nargs
#define UNUSED_IMPL(nargs) UNUSED_IMPL_(nargs)
#define UNUSED(...) UNUSED_IMPL(VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)
