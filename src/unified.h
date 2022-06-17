#pragma once

#include <stdint.h>
typedef uint8_t     byte;
typedef uint64_t    memsize;
typedef const char* strlit;
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef uint8_t     bool8;
typedef uint16_t    bool16;
typedef uint32_t    bool32;
typedef uint64_t    bool64;
typedef float       f32;
typedef double      f64;

#define internal static
#define persist  static
#define global   static
#define aliasing auto&
#define lambda   const auto

#define MACRO_CONCAT__(X, Y)                            X##Y
#define MACRO_CONCAT_(X, Y)                             MACRO_CONCAT__(X, Y)
#define MACRO_STRINGIFY__(X)                            #X
#define MACRO_STRINGIFY_(X)                             MACRO_STRINGIFY__(X)
#define MACRO_EXPAND_(X)                                X
#define MACRO_OVERLOADED_2_(_0, _1, MACRO, ...)         MACRO
#define MACRO_OVERLOADED_3_(_0, _1, _2, MACRO, ...)     MACRO
#define MACRO_OVERLOADED_4_(_0, _1, _2, _3, MACRO, ...) MACRO
#define MACRO_HEAD_ARGS_(X, ...)                        (X)
#define MACRO_TAIL_ARGS_(X, ...)                        (__VA_ARGS__)
#define MACRO_FOR_EACH_0_(MACRO,    ...)
#define MACRO_FOR_EACH_1_(MACRO, x     )                MACRO(x)
#define MACRO_FOR_EACH_2_(MACRO, x, ...)                MACRO(x) MACRO_FOR_EACH_1_(MACRO, __VA_ARGS__)
#define MACRO_FOR_EACH_3_(MACRO, x, ...)                MACRO(x) MACRO_EXPAND_(MACRO_FOR_EACH_2_(MACRO, __VA_ARGS__))
#define MACRO_FOR_EACH_4_(MACRO, x, ...)                MACRO(x) MACRO_EXPAND_(MACRO_FOR_EACH_3_(MACRO, __VA_ARGS__))
#define MACRO_FOR_EACH_(MACRO, ...)                     MACRO_EXPAND_(MACRO_OVERLOADED_4_(__VA_ARGS__, MACRO_FOR_EACH_4_, MACRO_FOR_EACH_3_, MACRO_FOR_EACH_2_, MACRO_FOR_EACH_1_, MACRO_FOR_EACH_0_)(MACRO, __VA_ARGS__))
#define MACRO_STRINGIFY_COMMA_(X)                       MACRO_STRINGIFY_(X),
#define MACRO_STRINGIFY_ARGS_(...)                      MACRO_FOR_EACH_(MACRO_STRINGIFY_COMMA_, __VA_ARGS__)

#define CAPACITY_OF_ARRAY_(XS)              (sizeof(XS) / sizeof((XS)[0]))
#define CAPACITY_OF_MEMBER_(TYPE, MEMBER)   (CAPACITY_OF_ARRAY_(((TYPE*) 0)->MEMBER))
#define ARRAY_CAPACITY(...)                 (MACRO_EXPAND_(MACRO_OVERLOADED_2_(__VA_ARGS__, CAPACITY_OF_MEMBER_, CAPACITY_OF_ARRAY_)(__VA_ARGS__)))

#define FOR_INTERVAL_(NAME, MINI, MAXI)     for (i32 NAME = (MINI); NAME < (MAXI); NAME += 1)
#define FOR_INDICIES_(NAME, MAXI)           FOR_INTERVAL_(NAME, 0, (MAXI))
#define FOR_REPEAT_(MAXI)                   FOR_INTERVAL_(MACRO_CONCAT_(FOR_REPEAT_, __LINE__), 0, (MAXI))
#define FOR_RANGE(...)                      MACRO_EXPAND_(MACRO_OVERLOADED_3_(__VA_ARGS__, FOR_INTERVAL_, FOR_INDICIES_, FOR_REPEAT_)(__VA_ARGS__))
#define FOR_INTERVAL_REV_(NAME, MINI, MAXI) for (i32 NAME = (MAXI) - 1, MACRO_CONCAT_(NAME, min); NAME >= MACRO_CONCAT_(NAME, min); NAME -= 1)
#define FOR_INDICIES_REV_(NAME, MAXI)       FOR_INTERVAL_REV_(NAME, 0, (MAXI))
#define FOR_RANGE_REV(...)                  MACRO_EXPAND_(MACRO_OVERLOADED_3_(__VA_ARGS__, FOR_INTERVAL_REV_, FOR_INDICIES_REV_)(__VA_ARGS__))

#define FOR_POINTER_(NAME, XS, COUNT)       for (i32 MACRO_CONCAT_(NAME, _index) = 0; MACRO_CONCAT_(NAME, _index) < (COUNT); MACRO_CONCAT_(NAME, _index) += 1) if (const auto NAME = &(XS)[MACRO_CONCAT_(NAME, _index)]; false); else
#define FOR_ARRAY_(NAME, XS)                FOR_POINTER_(NAME, (XS), ARRAY_CAPACITY(XS))
#define FOR_IT_(XS)                         FOR_POINTER_(it, (XS), ARRAY_CAPACITY(XS))
#define FOR_ELEMS(...)                      MACRO_EXPAND_(MACRO_OVERLOADED_3_(__VA_ARGS__, FOR_POINTER_, FOR_ARRAY_, FOR_IT_)(__VA_ARGS__))
#define FOR_POINTER_REV_(NAME, XS, COUNT)   for (i32 MACRO_CONCAT_(NAME, _index) = (COUNT) - 1; MACRO_CONCAT_(NAME, _index) >= 0; MACRO_CONCAT_(NAME, _index) -= 1) if (const auto NAME = &(XS)[MACRO_CONCAT_(NAME, _index)]; false); else
#define FOR_ARRAY_REV_(NAME, XS)            FOR_POINTER_REV_(NAME, (XS), ARRAY_CAPACITY(XS))
#define FOR_ELEMS_REV(...)                  MACRO_EXPAND_(MACRO_OVERLOADED_3_(__VA_ARGS__, FOR_POINTER_REV_, FOR_ARRAY_REV_)(__VA_ARGS__))

#define FOR_NODES_(NAME, NODES)             if (auto NAME = (NODES); false); else for (i32 MACRO_CONCAT_(NAME, _index) = 0; NAME; MACRO_CONCAT_(NAME, _index) += 1, NAME = NAME->next_node)
#define FOR_IT_NODES_(NODES)                FOR_NODES_(it, (NODES))
#define FOR_NODES(...)                      MACRO_EXPAND_(MACRO_OVERLOADED_2_(__VA_ARGS__, FOR_NODES_, FOR_IT_NODES_)(__VA_ARGS__))

#define IMPLIES(P, Q)                       (!(P) || (Q))
#define IFF(P, Q)                           (!(P) == !(Q))
#define IN_RANGE(X, MINI, MAXI)             ((MINI) <= (X) && (X) < (MAXI))
#define SWAP(X, Y)                          do { auto MACRO_CONCAT_(SWAP_, __LINE__) = *(X); *(X) = *(Y); *(Y) = MACRO_CONCAT_(SWAP_, __LINE__); } while (false)
#define KIBIBYTES_OF(N)                     (1024ULL *             (N))
#define MEBIBYTES_OF(N)                     (1024ULL * KIBIBYTES_OF(N))
#define GIBIBYTES_OF(N)                     (1024ULL * MEBIBYTES_OF(N))
#define TEBIBYTES_OF(N)                     (1024ULL * GIBIBYTES_OF(N))

#if DEBUG
	#include <assert.h>
	#include <stdio.h>
	#include <windows.h>
	#undef interface
	#undef min
	#undef max

	#define ASSERT(EXPRESSION) do { if (!(EXPRESSION)) { *((i32*)(0)) = 0; } } while (false)

	#define DEBUG_printf(FSTR, ...)\
	do\
	{\
		char MACRO_CONCAT_(DEBUG_PRINTF_, __LINE__)[1024];\
		sprintf_s(MACRO_CONCAT_(DEBUG_PRINTF_, __LINE__), sizeof(MACRO_CONCAT_(DEBUG_PRINTF_, __LINE__)), (FSTR), __VA_ARGS__);\
		OutputDebugStringA(MACRO_CONCAT_(DEBUG_PRINTF_, __LINE__));\
	}\
	while (false)

	#define DEBUG_once\
	for (persist bool32 MACRO_CONCAT_(DEBUG_ONCE_, __LINE__) = true; MACRO_CONCAT_(DEBUG_ONCE_, __LINE__); MACRO_CONCAT_(DEBUG_ONCE_, __LINE__) = false)

	#define DEBUG_PROFILER_create_group(GROUP_NAME, ...)\
	enum struct GROUP_NAME : u8 { __VA_ARGS__, CAPACITY };\
	constexpr strlit        MACRO_CONCAT_(GROUP_NAME, _STRS)[GROUP_NAME::CAPACITY] = { MACRO_STRINGIFY_ARGS_(__VA_ARGS__) };\
	persist   LARGE_INTEGER MACRO_CONCAT_(GROUP_NAME, _LI_0)[GROUP_NAME::CAPACITY] = {};\
	persist   LARGE_INTEGER MACRO_CONCAT_(GROUP_NAME, _LI_1)[GROUP_NAME::CAPACITY] = {};\
	persist   u64           MACRO_CONCAT_(GROUP_NAME, _DATA)[GROUP_NAME::CAPACITY] = {}

	#define DEBUG_PROFILER_flush_group(GROUP_NAME, COUNT, GOAL)\
	do\
	{\
		if (persist u64 MACRO_CONCAT_(GROUP_NAME, _COUNTER) = 0; ++MACRO_CONCAT_(GROUP_NAME, _COUNTER) >= (COUNT))\
		{\
			MACRO_CONCAT_(GROUP_NAME, _COUNTER) = 0;\
			u64           MACRO_CONCAT_(GROUP_NAME, _TOTAL) = 0;\
			LARGE_INTEGER MACRO_CONCAT_(GROUP_NAME, _FREQUENCY);\
			QueryPerformanceFrequency(&MACRO_CONCAT_(GROUP_NAME, _FREQUENCY));\
			FOR_RANGE(MACRO_CONCAT_(GROUP_NAME, _INDEX), static_cast<u8>(GROUP_NAME::CAPACITY))\
			{\
				MACRO_CONCAT_(GROUP_NAME, _TOTAL) += MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)];\
			}\
			DEBUG_printf\
			(\
				#GROUP_NAME " TOTAL\n\t(%fs) (%.2f%%) %llu\n",\
				MACRO_CONCAT_(GROUP_NAME, _TOTAL) / static_cast<f64>((COUNT) * MACRO_CONCAT_(GROUP_NAME, _FREQUENCY).QuadPart),\
				MACRO_CONCAT_(GROUP_NAME, _TOTAL) / static_cast<f64>((COUNT) * MACRO_CONCAT_(GROUP_NAME, _FREQUENCY).QuadPart) / (GOAL) * 100.0,\
				MACRO_CONCAT_(GROUP_NAME, _TOTAL)\
			);\
			FOR_RANGE(MACRO_CONCAT_(GROUP_NAME, _INDEX), static_cast<u8>(GROUP_NAME::CAPACITY))\
			{\
				DEBUG_printf\
				(\
					"%s\n\t(%.3f) %llu\n",\
					MACRO_CONCAT_(GROUP_NAME, _STRS)[MACRO_CONCAT_(GROUP_NAME, _INDEX)],\
					MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)] / static_cast<f64>(MACRO_CONCAT_(GROUP_NAME, _TOTAL)),\
					MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)]\
				);\
				MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)] = 0;\
			}\
			DEBUG_printf("\n");\
		}\
	}\
	while (false)

	#define DEBUG_PROFILER_start(GROUP_NAME, SUBGROUP_NAME)\
	do { QueryPerformanceCounter(&MACRO_CONCAT_(GROUP_NAME, _LI_0)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)]); } while (false)

	#define DEBUG_PROFILER_end(GROUP_NAME, SUBGROUP_NAME)\
	do\
	{\
		QueryPerformanceCounter(&MACRO_CONCAT_(GROUP_NAME, _LI_1)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)]);\
		MACRO_CONCAT_(GROUP_NAME, _DATA)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)] += MACRO_CONCAT_(GROUP_NAME, _LI_1)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)].QuadPart - MACRO_CONCAT_(GROUP_NAME, _LI_0)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)].QuadPart;\
	} while (false)

	#define DEBUG_STDOUT_HALT()\
	do\
	{\
		printf\
		(\
			"======================== DEBUG_STDOUT_HALT ========================\n"\
			"A stdout halt occured from file `" __FILE__ "` on line " MACRO_STRINGIFY_(__LINE__) ".\n"\
			"===================================================================\n"\
		);\
		fgetc(stdin);\
	}\
	while (false)
#else
	#define ASSERT(EXPRESSION)
	#define DEBUG_printf(FSTR, ...)
	#define DEBUG_once                       if (true); else
	#define DEBUG_PROFILER_create_group(...)
	#define DEBUG_PROFILER_flush_group(...)
	#define DEBUG_PROFILER_start(...)
	#define DEBUG_PROFILER_end(...)
	#define DEBUG_STDOUT_HALT()
#endif


#define enum_loose(NAME, TYPE)\
enum struct NAME : TYPE;\
internal constexpr TYPE operator+  (const NAME& a               ) { return static_cast<TYPE>(a); }\
internal constexpr TYPE operator<  (const NAME& a, const NAME& b) { return +a <  +b; }\
internal constexpr TYPE operator<= (const NAME& a, const NAME& b) { return +a <= +b; }\
internal constexpr TYPE operator>  (const NAME& a, const NAME& b) { return +a >  +b; }\
internal constexpr TYPE operator>= (const NAME& a, const NAME& b) { return +a >= +b; }\
enum struct NAME : TYPE

#define flag_struct(NAME, TYPE)\
enum struct NAME : TYPE;\
internal constexpr TYPE operator+  (const NAME& a               ) { return     static_cast<TYPE>(a);            }\
internal constexpr NAME operator&  (const NAME& a, const NAME& b) { return     static_cast<NAME>( (+a) & (+b)); }\
internal constexpr NAME operator|  (const NAME& a, const NAME& b) { return     static_cast<NAME>( (+a) | (+b)); }\
internal constexpr NAME operator^  (const NAME& a, const NAME& b) { return     static_cast<NAME>( (+a) ^ (+b)); }\
internal constexpr NAME operator<< (const NAME& a, const i32&  n) { return     static_cast<NAME>( (+a) << n  ); }\
internal constexpr NAME operator>> (const NAME& a, const i32&  n) { return     static_cast<NAME>( (+a) >> n  ); }\
internal constexpr NAME operator~  (const NAME& a               ) { return     static_cast<NAME>( ~+a        ); }\
internal constexpr NAME operator&= (      NAME& a, const NAME& b) { return a = static_cast<NAME>( (+a) & (+b)); }\
internal constexpr NAME operator|= (      NAME& a, const NAME& b) { return a = static_cast<NAME>( (+a) | (+b)); }\
internal constexpr NAME operator^= (      NAME& a, const NAME& b) { return a = static_cast<NAME>( (+a) ^ (+b)); }\
internal constexpr NAME operator<<=(      NAME& a, const i32&  n) { return a = static_cast<NAME>( (+a) << n  ); }\
internal constexpr NAME operator>>=(      NAME& a, const i32&  n) { return a = static_cast<NAME>( (+a) >> n  ); }\
enum struct NAME : TYPE

#define enum_start_region(NAME) MACRO_CONCAT_(NAME, _START), MACRO_CONCAT_(NAME, _START_) = MACRO_CONCAT_(NAME, _START) - 1,
#define enum_end_region(NAME)   MACRO_CONCAT_(NAME, _END), MACRO_CONCAT_(NAME, _COUNT) = MACRO_CONCAT_(NAME, _END) - MACRO_CONCAT_(NAME, _START), MACRO_CONCAT_(NAME, _END_) = MACRO_CONCAT_(NAME, _END) - 1,

#include <utility>
#define DEFER auto MACRO_CONCAT_(DEFER_, __LINE__) = DEFER_EMPTY_ {} + [&]()

template <typename F>
struct DEFER_
{
	F f;
	DEFER_(F f) : f(f) {}
	~DEFER_() { f(); }
};

struct DEFER_EMPTY_ {};

template <typename F>
internal DEFER_<F> operator+(DEFER_EMPTY_, F&& f)
{
	return DEFER_<F>(std::forward<F>(f));
}

//
// Misc.
//

template <typename TYPE>
internal TYPE* pop_node(TYPE** node)
{
	TYPE* head = *node;
	*node = (*node)->next_node;
	return head;
}

template <typename TYPE>
internal void push_single_node(TYPE* head, TYPE** node)
{
	head->next_node = *node;
	*node           = head;
}

template <typename TYPE>
internal void push_entire_node(TYPE* head, TYPE** node)
{
	if (head)
	{
		TYPE* last = head;
		while (last->next_node)
		{
			last = last->next_node;
		}
		last->next_node = *node;
		*node           = head;
	}
}

//
// Memory.
//

#define memory_arena_checkpoint(ARENA)\
memsize MACRO_CONCAT_(MEMORY_ARENA_CHECKPOINT_, __LINE__) = (ARENA)->used;\
DEFER { (ARENA)->used = MACRO_CONCAT_(MEMORY_ARENA_CHECKPOINT_, __LINE__); }

struct MemoryArena
{
	memsize size;
	byte*   base;
	memsize used;
};

template <typename TYPE>
internal TYPE* memory_arena_allocate(MemoryArena* arena, const memsize& count = 1)
{
	ASSERT(arena->used + sizeof(TYPE) * count <= arena->size);
	byte* allocation = arena->base + arena->used;
	arena->used += sizeof(TYPE) * count;
	return reinterpret_cast<TYPE*>(allocation);
}

template <typename TYPE>
internal TYPE* memory_arena_allocate_zero(MemoryArena* arena, const memsize& count = 1)
{
	ASSERT(arena->used + sizeof(TYPE) * count <= arena->size);
	byte* allocation = arena->base + arena->used;
	memset(allocation, static_cast<unsigned char>(0), sizeof(TYPE) * count);
	arena->used += sizeof(TYPE) * count;
	return reinterpret_cast<TYPE*>(allocation);
}

internal MemoryArena memory_arena_reserve(MemoryArena* arena, const memsize& size)
{
	ASSERT(arena->used + size <= arena->size);
	MemoryArena reservation;
	reservation.size = size;
	reservation.base = arena->base + arena->used;
	reservation.used = 0;
	arena->used += size;
	return reservation;
}

template <typename TYPE>
internal TYPE* memory_arena_allocate_from_available(TYPE** available, MemoryArena* arena)
{
	if (*available)
	{
		TYPE* allocation = *available;
		*available = (*available)->next_node;
		return allocation;
	}
	else
	{
		return memory_arena_allocate<TYPE>(arena);
	}
}

//
// Math.
//

#include <algorithm>

using std::clamp;
using std::min;
using std::max;

global constexpr f32 TAU   = 6.28318530717f;
global constexpr f32 SQRT2 = 1.41421356237f;

struct vf2
{
	union
	{
		struct { f32 x; f32 y; };
		f32 coordinates[2];
	};
};

struct vf3
{
	union
	{
		struct { f32 x; f32 y; f32 z; };
		vf2 xy;
		f32 coordinates[3];
	};
};

struct vf4
{
	union
	{
		struct { f32 x; f32 y; f32 z; f32 w; };
		vf3 xyz;
		vf2 xy;
		f32 coordinates[4];
	};
};

internal constexpr bool32 operator+ (const vf2& v              ) { return v.x || v.y;               }
internal constexpr bool32 operator+ (const vf3& v              ) { return v.x || v.y || v.z;        }
internal constexpr bool32 operator+ (const vf4& v              ) { return v.x || v.y || v.z || v.w; }
internal constexpr vf2    operator- (const vf2& v              ) { return { -v.x, -v.y             }; }
internal constexpr vf3    operator- (const vf3& v              ) { return { -v.x, -v.y, -v.z       }; }
internal constexpr vf4    operator- (const vf4& v              ) { return { -v.x, -v.y, -v.z, -v.w }; }
internal constexpr bool32 operator==(const vf2& u, const vf2& v) { return u.x == v.x && u.y == v.y;                             }
internal constexpr bool32 operator==(const vf3& u, const vf3& v) { return u.x == v.x && u.y == v.y && u.z == v.z;               }
internal constexpr bool32 operator==(const vf4& u, const vf4& v) { return u.x == v.x && u.y == v.y && u.z == v.z && u.w == v.w; }
internal constexpr bool32 operator!=(const vf2& u, const vf2& v) { return !(u == v); }
internal constexpr bool32 operator!=(const vf3& u, const vf3& v) { return !(u == v); }
internal constexpr bool32 operator!=(const vf4& u, const vf4& v) { return !(u == v); }
internal constexpr vf2    operator+ (const vf2& u, const vf2& v) { return { u.x + v.x, u.y + v.y                       }; }
internal constexpr vf3    operator+ (const vf3& u, const vf3& v) { return { u.x + v.x, u.y + v.y, u.z + v.z            }; }
internal constexpr vf4    operator+ (const vf4& u, const vf4& v) { return { u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w }; }
internal constexpr vf2    operator- (const vf2& u, const vf2& v) { return { u.x - v.x, u.y - v.y                       }; }
internal constexpr vf3    operator- (const vf3& u, const vf3& v) { return { u.x - v.x, u.y - v.y, u.z - v.z            }; }
internal constexpr vf4    operator- (const vf4& u, const vf4& v) { return { u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w }; }
internal constexpr vf2    operator/ (const vf2& v, const f32& k) { return { v.x / k, v.y / k                   }; }
internal constexpr vf3    operator/ (const vf3& v, const f32& k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vf4    operator/ (const vf4& v, const f32& k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vf2    operator* (const vf2& u, const vf2& v) { return { u.x * v.x, u.y * v.y                       }; }
internal constexpr vf3    operator* (const vf3& u, const vf3& v) { return { u.x * v.x, u.y * v.y, u.z * v.z            }; }
internal constexpr vf4    operator* (const vf4& u, const vf4& v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }
internal constexpr vf2    operator* (const vf2& v, const f32& k) { return { v.x * k, v.y * k                   }; }
internal constexpr vf3    operator* (const vf3& v, const f32& k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vf4    operator* (const vf4& v, const f32& k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vf2    operator* (const f32& k, const vf2& v) { return v * k; }
internal constexpr vf3    operator* (const f32& k, const vf3& v) { return v * k; }
internal constexpr vf4    operator* (const f32& k, const vf4& v) { return v * k; }
internal constexpr vf2&   operator+=(      vf2& u, const vf2& v) { return u = u + v; }
internal constexpr vf3&   operator+=(      vf3& u, const vf3& v) { return u = u + v; }
internal constexpr vf4&   operator+=(      vf4& u, const vf4& v) { return u = u + v; }
internal constexpr vf2&   operator-=(      vf2& u, const vf2& v) { return u = u - v; }
internal constexpr vf3&   operator-=(      vf3& u, const vf3& v) { return u = u - v; }
internal constexpr vf4&   operator-=(      vf4& u, const vf4& v) { return u = u - v; }
internal constexpr vf2&   operator*=(      vf2& u, const vf2& v) { return u = u * v; }
internal constexpr vf3&   operator*=(      vf3& u, const vf3& v) { return u = u * v; }
internal constexpr vf4&   operator*=(      vf4& u, const vf4& v) { return u = u * v; }
internal constexpr vf2&   operator*=(      vf2& v, const f32& k) { return v = v * k; }
internal constexpr vf3&   operator*=(      vf3& v, const f32& k) { return v = v * k; }
internal constexpr vf4&   operator*=(      vf4& v, const f32& k) { return v = v * k; }
internal constexpr vf2&   operator/=(      vf2& v, const f32& k) { return v = v / k; }
internal constexpr vf3&   operator/=(      vf3& v, const f32& k) { return v = v / k; }
internal constexpr vf4&   operator/=(      vf4& v, const f32& k) { return v = v / k; }

struct vi2
{
	union
	{
		struct { i32 x; i32 y; };
		i32 coordinates[2];
	};
};

struct vi3
{
	union
	{
		struct { i32 x; i32 y; i32 z; };
		vi2 xy;
		i32 coordinates[3];
	};
};

struct vi4
{
	union
	{
		struct { i32 x; i32 y; i32 z; i32 w; };
		vi3 xyz;
		vi2 xy;
		i32 coordinates[4];
	};
};

internal constexpr bool32 operator+ (const vi2& v              ) { return v.x || v.y;               }
internal constexpr bool32 operator+ (const vi3& v              ) { return v.x || v.y || v.z;        }
internal constexpr bool32 operator+ (const vi4& v              ) { return v.x || v.y || v.z || v.w; }
internal constexpr vi2    operator- (const vi2& v              ) { return { -v.x, -v.y             }; }
internal constexpr vi3    operator- (const vi3& v              ) { return { -v.x, -v.y, -v.z       }; }
internal constexpr vi4    operator- (const vi4& v              ) { return { -v.x, -v.y, -v.z, -v.w }; }
internal constexpr bool32 operator==(const vi2& u, const vi2& v) { return u.x == v.x && u.y == v.y;                             }
internal constexpr bool32 operator==(const vi3& u, const vi3& v) { return u.x == v.x && u.y == v.y && u.z == v.z;               }
internal constexpr bool32 operator==(const vi4& u, const vi4& v) { return u.x == v.x && u.y == v.y && u.z == v.z && u.w == v.w; }
internal constexpr bool32 operator!=(const vi2& u, const vi2& v) { return !(u == v); }
internal constexpr bool32 operator!=(const vi3& u, const vi3& v) { return !(u == v); }
internal constexpr bool32 operator!=(const vi4& u, const vi4& v) { return !(u == v); }
internal constexpr vi2    operator+ (const vi2& u, const vi2& v) { return { u.x + v.x, u.y + v.y                       }; }
internal constexpr vi3    operator+ (const vi3& u, const vi3& v) { return { u.x + v.x, u.y + v.y, u.z + v.z            }; }
internal constexpr vi4    operator+ (const vi4& u, const vi4& v) { return { u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w }; }
internal constexpr vi2    operator- (const vi2& u, const vi2& v) { return { u.x - v.x, u.y - v.y                       }; }
internal constexpr vi3    operator- (const vi3& u, const vi3& v) { return { u.x - v.x, u.y - v.y, u.z - v.z            }; }
internal constexpr vi4    operator- (const vi4& u, const vi4& v) { return { u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w }; }
internal constexpr vi2    operator/ (const vi2& v, const i32& k) { return { v.x / k, v.y / k                   }; }
internal constexpr vi3    operator/ (const vi3& v, const i32& k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vi4    operator/ (const vi4& v, const i32& k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vi2    operator* (const vi2& u, const vi2& v) { return { u.x * v.x, u.y * v.y                       }; }
internal constexpr vi3    operator* (const vi3& u, const vi3& v) { return { u.x * v.x, u.y * v.y, u.z * v.z            }; }
internal constexpr vi4    operator* (const vi4& u, const vi4& v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }
internal constexpr vi2    operator* (const vi2& v, const i32& k) { return { v.x * k, v.y * k                   }; }
internal constexpr vi3    operator* (const vi3& v, const i32& k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vi4    operator* (const vi4& v, const i32& k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vi2    operator* (const i32& k, const vi2& v) { return v * k; }
internal constexpr vi3    operator* (const i32& k, const vi3& v) { return v * k; }
internal constexpr vi4    operator* (const i32& k, const vi4& v) { return v * k; }
internal constexpr vi2&   operator+=(      vi2& u, const vi2& v) { return u = u + v; }
internal constexpr vi3&   operator+=(      vi3& u, const vi3& v) { return u = u + v; }
internal constexpr vi4&   operator+=(      vi4& u, const vi4& v) { return u = u + v; }
internal constexpr vi2&   operator-=(      vi2& u, const vi2& v) { return u = u - v; }
internal constexpr vi3&   operator-=(      vi3& u, const vi3& v) { return u = u - v; }
internal constexpr vi4&   operator-=(      vi4& u, const vi4& v) { return u = u - v; }
internal constexpr vi2&   operator*=(      vi2& u, const vi2& v) { return u = u * v; }
internal constexpr vi3&   operator*=(      vi3& u, const vi3& v) { return u = u * v; }
internal constexpr vi4&   operator*=(      vi4& u, const vi4& v) { return u = u * v; }
internal constexpr vi2&   operator*=(      vi2& v, const i32& k) { return v = v * k; }
internal constexpr vi3&   operator*=(      vi3& v, const i32& k) { return v = v * k; }
internal constexpr vi4&   operator*=(      vi4& v, const i32& k) { return v = v * k; }
internal constexpr vi2&   operator/=(      vi2& v, const i32& k) { return v = v / k; }
internal constexpr vi3&   operator/=(      vi3& v, const i32& k) { return v = v / k; }
internal constexpr vi4&   operator/=(      vi4& v, const i32& k) { return v = v / k; }

internal constexpr vf2    operator+ (const vf2& u, const vi2& v) { return { u.x + static_cast<f32>(v.x), u.y + static_cast<f32>(v.y)                                                           }; }
internal constexpr vf3    operator+ (const vf3& u, const vi3& v) { return { u.x + static_cast<f32>(v.x), u.y + static_cast<f32>(v.y), u.z + static_cast<f32>(v.z)                              }; }
internal constexpr vf4    operator+ (const vf4& u, const vi4& v) { return { u.x + static_cast<f32>(v.x), u.y + static_cast<f32>(v.y), u.z + static_cast<f32>(v.z), u.w + static_cast<f32>(v.w) }; }
internal constexpr vf2    operator+ (const vi2& u, const vf2& v) { return v + u; }
internal constexpr vf3    operator+ (const vi3& u, const vf3& v) { return v + u; }
internal constexpr vf4    operator+ (const vi4& u, const vf4& v) { return v + u; }
internal constexpr vf2    operator- (const vf2& u, const vi2& v) { return { u.x - static_cast<f32>(v.x), u.y - static_cast<f32>(v.y)                                                           }; }
internal constexpr vf3    operator- (const vf3& u, const vi3& v) { return { u.x - static_cast<f32>(v.x), u.y - static_cast<f32>(v.y), u.z - static_cast<f32>(v.z)                              }; }
internal constexpr vf4    operator- (const vf4& u, const vi4& v) { return { u.x - static_cast<f32>(v.x), u.y - static_cast<f32>(v.y), u.z - static_cast<f32>(v.z), u.w - static_cast<f32>(v.w) }; }
internal constexpr vf2    operator- (const vi2& u, const vf2& v) { return { static_cast<f32>(u.x) - v.x, static_cast<f32>(u.y) - v.y                                                           }; }
internal constexpr vf3    operator- (const vi3& u, const vf3& v) { return { static_cast<f32>(u.x) - v.x, static_cast<f32>(u.y) - v.y, static_cast<f32>(u.z) - v.z                              }; }
internal constexpr vf4    operator- (const vi4& u, const vf4& v) { return { static_cast<f32>(u.x) - v.x, static_cast<f32>(u.y) - v.y, static_cast<f32>(u.z) - v.z, static_cast<f32>(u.w) - v.w }; }
internal constexpr vf2    operator/ (const vf2& v, const i32& k) { return { v.x / k, v.y / k                   }; }
internal constexpr vf3    operator/ (const vf3& v, const i32& k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vf4    operator/ (const vf4& v, const i32& k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vf2    operator/ (const vi2& v, const f32& k) { return { v.x / k, v.y / k                   }; }
internal constexpr vf3    operator/ (const vi3& v, const f32& k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vf4    operator/ (const vi4& v, const f32& k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vf2    operator* (const vf2& u, const vi2& v) { return { u.x * v.x, u.y * v.y                       }; }
internal constexpr vf3    operator* (const vf3& u, const vi3& v) { return { u.x * v.x, u.y * v.y, u.z * v.z            }; }
internal constexpr vf4    operator* (const vf4& u, const vi4& v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }
internal constexpr vf2    operator* (const vi2& u, const vf2& v) { return { u.x * v.x, u.y * v.y                       }; }
internal constexpr vf3    operator* (const vi3& u, const vf3& v) { return { u.x * v.x, u.y * v.y, u.z * v.z            }; }
internal constexpr vf4    operator* (const vi4& u, const vf4& v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }
internal constexpr vf2    operator* (const vf2& v, const i32& k) { return { v.x * k, v.y * k                   }; }
internal constexpr vf3    operator* (const vf3& v, const i32& k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vf4    operator* (const vf4& v, const i32& k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vf2    operator* (const vi2& v, const f32& k) { return { v.x * k, v.y * k                   }; }
internal constexpr vf3    operator* (const vi3& v, const f32& k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vf4    operator* (const vi4& v, const f32& k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vf2    operator* (const f32& k, const vi2& v) { return v * k; }
internal constexpr vf3    operator* (const f32& k, const vi3& v) { return v * k; }
internal constexpr vf4    operator* (const f32& k, const vi4& v) { return v * k; }
internal constexpr vf2&   operator+=(      vf2& u, const vi2& v) { return u = u + v; }
internal constexpr vf3&   operator+=(      vf3& u, const vi3& v) { return u = u + v; }
internal constexpr vf4&   operator+=(      vf4& u, const vi4& v) { return u = u + v; }
internal constexpr vf2&   operator-=(      vf2& u, const vi2& v) { return u = u - v; }
internal constexpr vf3&   operator-=(      vf3& u, const vi3& v) { return u = u - v; }
internal constexpr vf4&   operator-=(      vf4& u, const vi4& v) { return u = u - v; }
internal constexpr vf2&   operator*=(      vf2& u, const vi2& v) { return u = u * v; }
internal constexpr vf3&   operator*=(      vf3& u, const vi3& v) { return u = u * v; }
internal constexpr vf4&   operator*=(      vf4& u, const vi4& v) { return u = u * v; }
internal constexpr vf2&   operator*=(      vf2& v, const i32& k) { return v = v * k; }
internal constexpr vf3&   operator*=(      vf3& v, const i32& k) { return v = v * k; }
internal constexpr vf4&   operator*=(      vf4& v, const i32& k) { return v = v * k; }
internal constexpr vf2&   operator/=(      vf2& v, const i32& k) { return v = v / k; }
internal constexpr vf3&   operator/=(      vf3& v, const i32& k) { return v = v / k; }
internal constexpr vf4&   operator/=(      vf4& v, const i32& k) { return v = v / k; }

internal constexpr vf2    vxx(const vi2& v                            ) { return { static_cast<f32>(v.x), static_cast<f32>(v.y)                                               }; }
internal constexpr vi2    vxx(const vf2& v                            ) { return { static_cast<i32>(v.x), static_cast<i32>(v.y)                                               }; }
internal constexpr vf3    vxx(const vi3& v                            ) { return { static_cast<f32>(v.x), static_cast<f32>(v.y), static_cast<f32>(v.z)                        }; }
internal constexpr vi3    vxx(const vf3& v                            ) { return { static_cast<i32>(v.x), static_cast<i32>(v.y), static_cast<i32>(v.z)                        }; }
internal constexpr vf4    vxx(const vi4& v                            ) { return { static_cast<f32>(v.x), static_cast<f32>(v.y), static_cast<f32>(v.z), static_cast<f32>(v.w) }; }
internal constexpr vi4    vxx(const vf4& v                            ) { return { static_cast<i32>(v.x), static_cast<i32>(v.y), static_cast<i32>(v.z), static_cast<i32>(v.w) }; }
internal constexpr vf3    vxx(const vf2& v, const f32& a              ) { return { v.x, v.y,   a    }; }
internal constexpr vf4    vxx(const vf2& v, const f32& a, const f32& b) { return { v.x, v.y,   a, b }; }
internal constexpr vf4    vxx(const vf3& v, const f32& a              ) { return { v.x, v.y, v.z, a }; }

internal constexpr vf2    vx2(const f32& a) { return { a, a       }; }
internal constexpr vi2    vx2(const i32& a) { return { a, a       }; }
internal constexpr vf3    vx3(const f32& a) { return { a, a, a    }; }
internal constexpr vi3    vx3(const i32& a) { return { a, a, a    }; }
internal constexpr vf4    vx4(const f32& a) { return { a, a, a, a }; }
internal constexpr vi4    vx4(const i32& a) { return { a, a, a, a }; }

template <typename TYPE>
i32 sign(const TYPE& x)
{
	return (static_cast<TYPE>(0) < x) - (x < static_cast<TYPE>(0));
}

internal constexpr f32 square(const f32& x) { return x * x;     }
internal constexpr f32 cube  (const f32& x) { return x * x * x; }

internal constexpr vf4 lerp(const vf4& a, const vf4& b, const f32& t) { return a * (1.0f - t) + b * t; }
internal constexpr vf3 lerp(const vf3& a, const vf3& b, const f32& t) { return a * (1.0f - t) + b * t; }
internal constexpr vf2 lerp(const vf2& a, const vf2& b, const f32& t) { return a * (1.0f - t) + b * t; }
internal constexpr f32 lerp(const f32& a, const f32& b, const f32& t) { return a * (1.0f - t) + b * t; }

internal constexpr vf2 conjugate(const vf2& v) { return {  v.x, -v.y }; }
internal constexpr vi2 conjugate(const vi2& v) { return {  v.x, -v.y }; }
internal constexpr vf2 rotate90 (const vf2& v) { return { -v.y,  v.x }; }

internal vf2 polar(const f32& angle) { return { cosf(angle), sinf(angle) }; }

internal vf2 rotate(const vf2& v, const f32& angle)
{
	vf2 p = polar(angle);
	return { v.x * p.x - v.y * p.y, v.x * p.y + v.y * p.x };
}

internal constexpr f32 dampen(const f32& a, const f32& b, const f32& k, const f32& dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf2 dampen(const vf2& a, const vf2& b, const f32& k, const f32& dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf3 dampen(const vf3& a, const vf3& b, const f32& k, const f32& dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf4 dampen(const vf4& a, const vf4& b, const f32& k, const f32& dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }

internal constexpr f32 dot(const vf2& u, const vf2& v) { return u.x * v.x + u.y * v.y;                         }
internal constexpr f32 dot(const vf3& u, const vf3& v) { return u.x * v.x + u.y * v.y + u.z * v.z;             }
internal constexpr f32 dot(const vf4& u, const vf4& v) { return u.x * v.x + u.y * v.y + u.z * v.z + u.w * v.w; }

internal constexpr vf3 cross(const vf3& u, const vf3& v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }

internal constexpr f32 norm_sq(const vf2& v) { return v.x * v.x + v.y * v.y;                         }
internal constexpr f32 norm_sq(const vf3& v) { return v.x * v.x + v.y * v.y + v.z * v.z;             }
internal constexpr f32 norm_sq(const vf4& v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }

internal f32 norm(const vf2& v) { return sqrtf(norm_sq(v)); }
internal f32 norm(const vf3& v) { return sqrtf(norm_sq(v)); }
internal f32 norm(const vf4& v) { return sqrtf(norm_sq(v)); }

internal vf2 normalize(const vf2& v) { return v / norm(v); }
internal vf3 normalize(const vf3& v) { return v / norm(v); }
internal vf4 normalize(const vf4& v) { return v / norm(v); }

internal constexpr i32 mod(const i32& x, const i32& m) { return (x % m + m) % m; }
internal           f32 mod(const f32& x, const f32& m) { f32 y = fmodf(x, m); return y < 0.0f ? y + m : y; }

internal f32 atan2(const vf2& v) { return atan2f(v.y, v.x); }

#include <xmmintrin.h>

global constexpr __m128 m_0   = {     0.0f,     0.0f,     0.0f,     0.0f };
global constexpr __m128 m_1   = {     1.0f,     1.0f,     1.0f,     1.0f };
global constexpr __m128 m_2   = {     2.0f,     2.0f,     2.0f,     2.0f };
global constexpr __m128 m_3   = {     3.0f,     3.0f,     3.0f,     3.0f };
global constexpr __m128 m_4   = {     4.0f,     4.0f,     4.0f,     4.0f };
global constexpr __m128 m_8   = {     8.0f,     8.0f,     8.0f,     8.0f };
global constexpr __m128 m_255 = {   255.0f,   255.0f,   255.0f,   255.0f };
global constexpr __m128 m_inf = { INFINITY, INFINITY, INFINITY, INFINITY };

internal __m128 square(const __m128& x                                  ) { return _mm_mul_ps(x, x); }
internal __m128 cube  (const __m128& x                                  ) { return _mm_mul_ps(_mm_mul_ps(x, x), x); }
internal __m128 lerp  (const __m128& a, const __m128& b, const __m128& t) { return _mm_add_ps(_mm_mul_ps(a, _mm_sub_ps(m_1, t)), _mm_mul_ps(b, t)); }
internal __m128 clamp (const __m128& x, const __m128& a, const __m128& b) { return _mm_min_ps(_mm_max_ps(x, a), b); }

//
// String.
//

#define PASS_STRING_VIEW(STRING) (STRING).size, (STRING).data
#define STRING_VIEW_OF(STRLIT)   (StringView { sizeof(STRLIT) - 1, (STRLIT) })

struct StringView
{
	i32         size;
	const char* data;
};

struct StringBuilder_CharBufferNode
{
	StringBuilder_CharBufferNode* next_node;
	i32                           count;
	char                          buffer[256];
};

struct StringBuilder
{
	StringBuilder_CharBufferNode* head_char_buffer_node;
	StringBuilder_CharBufferNode* curr_char_buffer_node;
	i32                           size;
};

global StringBuilder_CharBufferNode* StringBuilder_available_char_buffer_node = 0; // @NOTE@ Will be malloc'ed but never freed. OS should be able to handle it.

internal StringBuilder_CharBufferNode* StringBuilder_allocate_char_buffer_node(void)
{
	if (StringBuilder_available_char_buffer_node)
	{
		return pop_node(&StringBuilder_available_char_buffer_node);
	}
	else
	{
		return reinterpret_cast<StringBuilder_CharBufferNode*>(malloc(sizeof(StringBuilder_CharBufferNode)));
	}
}

internal StringBuilder init_string_builder(void)
{
	StringBuilder builder;
	builder.head_char_buffer_node            = StringBuilder_allocate_char_buffer_node();
	builder.head_char_buffer_node->next_node = 0;
	builder.head_char_buffer_node->count     = 0;
	builder.curr_char_buffer_node            = builder.head_char_buffer_node;
	builder.size                             = 0;
	return builder;
}

internal StringView deinit_string_builder(StringBuilder* builder, MemoryArena* arena)
{
	char* string_data = memory_arena_allocate<char>(arena, builder->size);
	char* write_ptr   = string_data;

	FOR_NODES(builder->head_char_buffer_node)
	{
		ASSERT(IFF(it->count == ARRAY_CAPACITY(it->buffer), it->next_node));
		FOR_ELEMS(c, it->buffer, it->count)
		{
			*write_ptr  = *c;
			write_ptr  += 1;
		}
	}
	ASSERT(write_ptr == string_data + builder->size);

	push_entire_node(builder->head_char_buffer_node, &StringBuilder_available_char_buffer_node);

	return { builder->size, string_data };
}

internal void string_builder_append(StringBuilder* builder, const char& c)
{
	if (builder->curr_char_buffer_node->count == ARRAY_CAPACITY(builder->curr_char_buffer_node->buffer))
	{
		builder->curr_char_buffer_node->next_node = StringBuilder_allocate_char_buffer_node();
		builder->curr_char_buffer_node            = builder->curr_char_buffer_node->next_node;
		builder->curr_char_buffer_node->next_node = 0;
		builder->curr_char_buffer_node->count     = 0;
	}

	builder->curr_char_buffer_node->buffer[builder->curr_char_buffer_node->count] = c;
	builder->curr_char_buffer_node->count += 1;
	builder->size                         += 1;
}

internal void string_builder_append(StringBuilder* builder, const StringView& string)
{
	i32 offset = 0;
	while (string.size - offset > 0)
	{
		if (builder->curr_char_buffer_node->count == ARRAY_CAPACITY(builder->curr_char_buffer_node->buffer))
		{
			builder->curr_char_buffer_node->next_node = StringBuilder_allocate_char_buffer_node();
			builder->curr_char_buffer_node            = builder->curr_char_buffer_node->next_node;
			builder->curr_char_buffer_node->next_node = 0;
			builder->curr_char_buffer_node->count     = 0;
		}

		i32 write_count = min<i32>(ARRAY_CAPACITY(builder->curr_char_buffer_node->buffer) - builder->curr_char_buffer_node->count, string.size - offset);
		memcpy(builder->curr_char_buffer_node->buffer + builder->curr_char_buffer_node->count, string.data + offset, write_count);
		builder->size                         += write_count;
		builder->curr_char_buffer_node->count += write_count;
		offset                                += write_count;
	}
}

// @TODO@ Smarter implementation...
template <typename... ARGUMENTS>
internal void string_builder_append(StringBuilder* builder, strlit format, ARGUMENTS... arguments)
{
	char buffer[1024];
	i32  count = sprintf_s(buffer, format, arguments...);
	ASSERT(count < ARRAY_CAPACITY(buffer));
	string_builder_append(builder, { count, buffer });
}

template <typename... ARGUMENTS>
internal StringView string_builder_quick(MemoryArena* arena, strlit format, ARGUMENTS... arguments)
{
	StringBuilder builder = init_string_builder();
	string_builder_append(&builder, format, arguments...);
	return deinit_string_builder(&builder, arena);
}

internal constexpr bool32 is_alpha(const char& c)
{
	return IN_RANGE(c, 'a', 'z' + 1) || IN_RANGE(c, 'A', 'Z' + 1);
}

internal constexpr bool32 is_digit(const char& c)
{
	return IN_RANGE(c, '0', '9' + 1);
}

internal constexpr bool32 operator==(const StringView& a, const StringView& b) { return a.size == b.size && memcmp(a.data, b.data, a.size) == 0; }
internal constexpr bool32 operator!=(const StringView& a, const StringView& b) { return !(a == b); }

internal constexpr bool32 starts_with(const StringView& prefix, const StringView& string)
{
	return string.size >= prefix.size && StringView { prefix.size, string.data } == prefix;
}

