#ifndef base_math_h
#define base_math_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS_WASM
	#include <math.h>
#endif

// float

#define F32Epsilon (0.00001F)
#define F64Epsilon (0.0000001)

bool32 F32Equals(f32 A, f32 B);

bool32 F64Equals(f64 A, f64 B);

typedef struct f32_info {
	f32 Value;

	bool32 Sign;    // 1 bit
	u8 Exponent;  // 8 bits
	i8 UnbiasedExponent;
	u32 Mantissa; // 23 bits

	bool32 IsInfinity;
	bool32 IsNan;
} f32_info;

f32_info F32InfoFromF32(f32 Value);

f32 F32FromF32Info(f32_info Info);

// basic math

u64 DecPowers[] = {
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
	10000000000,
	100000000000,
	1000000000000,
	10000000000000,
	100000000000000,
	1000000000000000,
	10000000000000000,
	100000000000000000,
	1000000000000000000
};

i32 Triangular(i32 N);

f32 Sqrt(f32 N);

// vectors

typedef struct i32_v2 {
	i32 X;
	i32 Y;
} i32_v2;

i32_v2 I32V2(i32 X, i32 Y);

i32_v2 I32V2Add(i32_v2 A, i32_v2 B);
i32_v2 I32V2Minus(i32_v2 A, i32_v2 B);
#define I32V2Subtract I32V2Sub
i32_v2 I32V2Sub(i32_v2 A, i32_v2 B);
bool32 I32V2Equals(i32_v2 A, i32_v2 B);
bool32 I32V2IsZero(i32_v2 V);
i32_v2 I32V2Multiply(i32_v2 V, i32 S);
i32_v2 I32V2Divide(i32_v2 V, i32 S);
i32_v2 I32V2Hadamard(i32_v2 A, i32_v2 B);
i32_v2 I32V2HadamardDivide(i32_v2 A, i32_v2 B);
i32_v2 I32V2GridDirection(i32_v2 V);
i32_v2 I32V2Rotate(i32_v2 V, float Rotation);
i32_v2 I32V2Polar(float R, float Theta);

bool32 RectanglePointTest(i32_v2 RectanglePositon, i32_v2 RectangleSize, i32_v2 Point);

typedef struct int_rect {
	i32_v2 Position;
	i32_v2 Size;
} int_rect;

typedef struct i32_v3 {
	i32 X;
	i32 Y;
	i32 Z;
} i32_v3;

i32_v3 I32V3(i32 X, i32 Y, i32 Z);
i32_v3 I32V3Multiply(i32_v3 V, i32 S);
i32_v3 I32V3Hadamard(i32_v3 A, i32_v3 B);
i32_v3 I32V3Add(i32_v3 A, i32_v3 B);
i32_v3 I32V3Sub(i32_v3 A, i32_v3 B);
i32_v3 I32V3Dot(i32_v3 A, i32_v3 B);
bool32 I32V3Equals(i32_v3 A, i32_v3 B);
i32_v3 I32V3FromI32V2(i32_v2 V);

typedef union u8_v4 {
	struct {
		u8 X;
		u8 Y;
		u8 Z;
		u8 W;
	};
	struct {
		u8 R;
		u8 G;
		u8 B;
		u8 A;
	};
	u8 E[4];
	u64 V;
} u8_v4;

u8_v4 U8V4(u8 X, u8 Y, u8 Z, u8 W);
void U8V4Set(u8_v4 Value, u8 * X, u8 * Y, u8 * Z, u8 * W);

typedef struct f32_v2 {
	f32 X;
	f32 Y;
} f32_v2;

f32_v2 F32V2(f32 X, f32 Y);
f32_v2 F32V2Multiply(f32_v2 V, f32 S);
f32_v2 F32V2Hadamard(f32_v2 A, f32_v2 B);
f32_v2 F32V2HadamardDiv(f32_v2 A, f32_v2 B);
f32_v2 F32V2Add(f32_v2 A, f32_v2 B);
f32_v2 F32V2Sub(f32_v2 A, f32_v2 B);
f32 F32V2Dot(f32_v2 A, f32_v2 B);
f32_v2 F32V2Norm(f32_v2 V);
f32_v2 F32V2Average(f32_v2 A, f32_v2 B);
f32_v2 F32V2Rotate(f32_v2 V, f32 Rotation);
f32_v2 F32V2Polar(f32 R, f32 Theta);
f32_v2 F32V2FromI32V2(i32_v2 V);
i32_v2 F32V2ToI32V2(f32_v2 V);

typedef struct f32_v3 {
	f32 X;
	f32 Y;
	f32 Z;
} f32_v3;

f32_v3 F32V3(f32 X, f32 Y, f32 Z);
f32_v3 F32V3Multiply(f32_v3 V, f32 S);
f32_v3 F32V3Hadamard(f32_v3 A, f32_v3 B);
f32_v3 F32V3Add(f32_v3 A, f32_v3 B);
f32_v3 F32V3Sub(f32_v3 A, f32_v3 B);
f32 F32V3Dot(f32_v3 A, f32_v3 B);
f32_v3 F32V3Cross(f32_v3 A, f32_v3 B);
f32_v3 F32V3Norm(f32_v3 V);
f32_v3 F32V3Average(f32_v3 A, f32_v3 B);
f32_v3 F32V3FromI32V3(i32_v3 V);
f32_v3 F32V3ToI32V3(i32_v3 V);

typedef struct f32_v4 {
	union {
		f32 X;
		f32 R;
	};
	union {
		f32 Y;
		f32 B;
	};
	union {
		f32 Z;
		f32 G;
	};
	union {
		f32 W;
		f32 A;
	};
} f32_v4;

f32_v4 F32V4(f32 X, f32 Y, f32 Z, f32 W);
f32_v4 F32V4Multiply(f32_v4 V, f32 S);
f32_v4 F32V4Hadamard(f32_v4 A, f32_v4 B);
f32_v4 F32V4Add(f32_v4 A, f32_v4 B);
f32_v4 F32V4Sub(f32_v4 A, f32_v4 B);
f32 F32V4Dot(f32_v4 A, f32_v4 B);
f32_v4 F32V4Norm(f32_v4 V);
f32_v4 F32V4Average(f32_v4 A, f32_v4 B);
u8_v4 F32V4ToU8V4(f32_v4 V);

typedef struct f32_m3x3 {
	union {
		f32 E[9];
		struct {
			f32_v3 Rows[3];
		};
	};
} f32_m3x3;

f32_v3 F32M3x3VMultiply(f32_m3x3 M, f32_v3 V);
f32_m3x3 F32M3x3Multiply(f32_m3x3 A, f32_m3x3 B);
f32_m3x3 F32M3x3Identity();
f32_m3x3 F32M3x3ProjScale(f32 X, f32 Y);
f32_m3x3 F32M3x3ProjRotation(f32 Theta);
f32_m3x3 F32M3x3ProjTranslate(f32 X, f32 Y);

#ifdef __cplusplus
}
#endif

#endif