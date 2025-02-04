#include "base_include.h"

// float

bool32 F32Equals(f32 A, f32 B)
{
	if (A + F32Epsilon > B && A - F32Epsilon < B) return true;
	return false;
}

bool32 F64Equals(f64 A, f64 B)
{
	if (A + F64Epsilon > B && A - F64Epsilon < B) return true;
	return false;
}

f32_info F32InfoFromF32(f32 Value) {
	u32 * Binary = (u32 *) &Value;

	bool32 Sign = !!(0b1 << 31 & *Binary);

	u8 Exponent = ((0b11111111 << 23) & *Binary) >> 23;

	i8 UnbiasedExponent = Exponent - 127;

	u32 Mantissa = 0b11111111111111111111111 & *Binary;

	bool32 IsInfinity = Exponent == 0xff && Mantissa == 0;
	bool32 IsNan = Exponent == 0xff && Mantissa != 0;

	return (f32_info) { Value, Sign, Exponent, UnbiasedExponent, Mantissa, IsInfinity, IsNan };
}

f32 F32FromF32Info(f32_info Info) {
	u32 Binary = 0;

	if (Info.Sign) Binary |= 0b1 << 31;

	Info.Exponent = Info.UnbiasedExponent + 127;
	Binary |= Info.Exponent << 23;

	Binary |= Info.Mantissa;

	f32 * Value = (f32 *) &Binary;
	return *Value;
}

// basic math

i32 Triangular(i32 N) {
	return (N * (N + 1)) / 2;
}

f32 Sqrt(f32 N) {
	return sqrtf(N);
}

// vectors

i32_v2 I32V2(i32 X, i32 Y) {
	return (i32_v2) {X, Y};
}

i32_v2 I32V2Add(i32_v2 A, i32_v2 B) {
	return (i32_v2) {A.X + B.X, A.Y + B.Y};
}

i32_v2 I32V2Minus(i32_v2 A, i32_v2 B) {
	return (i32_v2) {A.X - B.X, A.Y - B.Y};
}

i32_v2 I32V2Sub(i32_v2 A, i32_v2 B) {
	return (i32_v2) {A.X - B.X, A.Y - B.Y};
}

bool32 I32V2Equals(i32_v2 A, i32_v2 B) {
	if (A.X == B.X && A.Y == B.Y) return true;
	else return false;
}

bool32 I32V2IsZero(i32_v2 V)
{
	return !(V.X || V.Y);
}

i32_v2 I32V2Multiply(i32_v2 V, i32 S) {
	return (i32_v2) {V.X * S, V.Y * S};
}

i32_v2 I32V2Divide(i32_v2 V, i32 S) {
	return (i32_v2) {V.X / S, V.Y / S};
}

i32_v2 I32V2Hadamard(i32_v2 A, i32_v2 B) {
	return (i32_v2) {A.X * B.X, A.Y * B.Y};
}

i32_v2 I32V2HadamardDivide(i32_v2 A, i32_v2 B) {
	return (i32_v2) {A.X / B.X, A.Y / B.Y};
}

i32_v2 I32V2GridDirection(i32_v2 V) {
	i32 X = V.X == 0 ? 0 : (V.X > 0 ? 1 : -1);  
	i32 Y = V.Y == 0 ? 0 : (V.Y > 0 ? 1 : -1);  
	return (i32_v2) {X, Y};
}

i32_v2 I32V2Rotate(i32_v2 V, float Rotation)
{
	return I32V2(cos(Rotation) * V.X - sin(Rotation) * V.Y, sin(Rotation) * V.X + cos(Rotation) * V.Y);
}

i32_v2 I32V2Polar(float R, float Theta)
{
	return I32V2(cos(Theta) * R, sin(Theta) * R);
}

i32_v2 I32V2Lerp(i32_v2 A, i32_v2 B, float T)
{
	return I32V2(B.X * T + A.X * (1 - T), B.Y * T + A.Y * (1 - T));
}

i32_v2 I32V2FromF32V2(f32_v2 V) {
	return (i32_v2) {(int) V.X, (int) V.Y};
}

bool32 RectanglePointTest(i32_v2 RectanglePositon, i32_v2 RectangleSize, i32_v2 Point)
{
	if (RectanglePositon.X > Point.X) return false;
	if (RectanglePositon.X + RectangleSize.X < Point.X) return false;

	if (RectanglePositon.Y > Point.Y) return false;
	if (RectanglePositon.Y + RectangleSize.Y < Point.Y) return false;

	return true;
}

i32_v3 I32V3(i32 X, i32 Y, i32 Z) {
	return (i32_v3) {X, Y, Z};
}

i32_v3 I32V3Multiply(i32_v3 V, i32 S) {
	return (i32_v3) {V.X * S, V.Y * S, V.Z * S};
}

i32_v3 I32V3Hadamard(i32_v3 A, i32_v3 B) {
	return (i32_v3) {A.X * B.X, A.Y * B.Y, A.Z * B.Z};
}

i32_v3 I32V3Add(i32_v3 A, i32_v3 B) {
	return (i32_v3) {A.X + B.X, A.Y + B.Y, A.Z + B.Z};
}

i32_v3 I32V3Sub(i32_v3 A, i32_v3 B) {
	return (i32_v3) {A.X - B.X, A.Y - B.Y, A.Z - B.Z};
}

i32_v3 I32V3Dot(i32_v3 A, i32_v3 B) {
	return (i32_v3) {A.X * B.X, A.Y * B.Y, A.Z * B.Z};
}

bool32 I32V3Equals(i32_v3 A, i32_v3 B) {
	if (A.X == B.X && A.Y == B.Y && A.Z == B.Z) return true;
	else return false;
}

i32_v3 I32V3FromF32V3(f32_v3 V) {
	return (i32_v3) {(i32) V.X, (i32) V.Y, (i32) V.Z};
}


u8_v4 U8V4(u8 X, u8 Y, u8 Z, u8 W)
{
	u8_v4 Result = { 0 };
	Result.X = X;
	Result.Y = Y;
	Result.Z = Z;
	Result.W = W;
	return Result;
}

void U8V4Set(u8_v4 Value, u8 * X, u8 * Y, u8 * Z, u8 * W)
{
	*X = Value.X;
	*Y = Value.Y;
	*Z = Value.Z;
	*W = Value.W;
}

u8_v4 U8V4FromF32V4(f32_v4 V)
{
	return U8V4(V.X * 255, V.Y * 255, V.Z * 255, V.W * 255);
}


i32_v3 I32V3FromI32V2(i32_v2 V) {
	return (i32_v3) {V.X, V.Y, 0};
}

f32_v3 F32V3(f32 X, f32 Y, f32 Z) {
	return (f32_v3) {X, Y, Z};
}

f32_v3 F32V3Multiply(f32_v3 V, f32 S) {
	return (f32_v3) {V.X * S, V.Y * S, V.Z * S};
}

f32_v3 F32V3Hadamard(f32_v3 A, f32_v3 B) {
	return (f32_v3) {A.X * B.X, A.Y * B.Y, A.Z * B.Z};
}

f32_v3 F32V3Add(f32_v3 A, f32_v3 B) {
	return (f32_v3) {A.X + B.X, A.Y + B.Y, A.Z + B.Z};
}

f32_v3 F32V3Sub(f32_v3 A, f32_v3 B) {
	return (f32_v3) {A.X - B.X, A.Y - B.Y, A.Z - B.Z};
}

f32 F32V3Dot(f32_v3 A, f32_v3 B) {
	return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

f32_v3 F32V3Cross(f32_v3 A, f32_v3 B) {
	return (f32_v3) {A.Y * B.Z - A.Z * B.Y, A.X * B.Z - A.Z * B.X * -1.0f, A.X * B.Y - A.Y * B.X};
}

f32_v3 F32V3Norm(f32_v3 V) {
	return F32V3Multiply(V, 1 / Sqrt(F32V3Dot(V, V)));
}

f32_v3 F32V3Average(f32_v3 A, f32_v3 B) {
	return (f32_v3) {(A.X + B.X)/2.0f, (A.Y + B.Y)/2.0f, (A.Z + B.Z)/2.0f};
}

f32_v3 F32V3FromI32V3(i32_v3 V) {
	return (f32_v3) {(f32) V.X, (f32) V.Y, (f32) V.Z};
}



f32_v4 F32V4(f32 X, f32 Y, f32 Z, f32 W) {
	return (f32_v4) {.X = X, .Y = Y, .Z = Z, .W = W};
}

f32_v4 F32V4Multiply(f32_v4 V, f32 S) {
	return (f32_v4) {.X = V.X * S, .Y = V.Y * S, .Z = V.Z * S, .W = V.W * S};
}

f32_v4 F32V4Hadamard(f32_v4 A, f32_v4 B) {
	return (f32_v4) {.X = A.X * B.X, .Y = A.Y * B.Y, .Z = A.Z * B.Z, .W = A.W * B.W};
}

f32_v4 F32V4Add(f32_v4 A, f32_v4 B) {
	return (f32_v4) {.X = A.X + B.X, .Y = A.Y + B.Y, .Z = A.Z + B.Z, .W = A.W + B.W};
}

f32_v4 F32V4Sub(f32_v4 A, f32_v4 B) {
	return (f32_v4) {.X = A.X - B.X, .Y = A.Y - B.Y, .Z = A.Z - B.Z, .W = A.W - B.W};
}

f32 F32V4Dot(f32_v4 A, f32_v4 B) {
	return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
}

f32_v4 F32V4Norm(f32_v4 V) {
	return F32V4Multiply(V, 1 / Sqrt(F32V4Dot(V, V)));
}

f32_v4 F32V4Average(f32_v4 A, f32_v4 B) {
	return (f32_v4) {.X = (A.X + B.X)/2.0f, .Y = (A.Y + B.Y)/2.0f, .Z = (A.Z + B.Z)/2.0f, .W = (A.W + B.W)/2.0f};
}

f32_v3 F32M3x3VMultiply(f32_m3x3 M, f32_v3 V) {
	return F32V3(F32V3Dot(M.Rows[0], V), F32V3Dot(M.Rows[1], V), F32V3Dot(M.Rows[2], V));
}

f32_m3x3 F32M3x3Multiply(f32_m3x3 A, f32_m3x3 B) {
	f32_v3 Col1 = F32V3(A.E[0], A.E[3], A.E[6]);
	f32_v3 Col2 = F32V3(A.E[1], A.E[4], A.E[7]);
	f32_v3 Col3 = F32V3(A.E[2], A.E[5], A.E[6]);

	f32_v3 Row1 = B.Rows[0];
	f32_v3 Row2 = B.Rows[1];
	f32_v3 Row3 = B.Rows[2];
	
	return (f32_m3x3) { .E = {
		F32V3Dot(Col1, Row1), F32V3Dot(Col2, Row1), F32V3Dot(Col3, Row1),
		F32V3Dot(Col1, Row2), F32V3Dot(Col2, Row2), F32V3Dot(Col3, Row2),
		F32V3Dot(Col1, Row3), F32V3Dot(Col2, Row3), F32V3Dot(Col3, Row3)
	} };
}

f32_m3x3 F32M3x3Identity() {
	return (f32_m3x3) { .E = { 
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	} };
}

f32_m3x3 F32M3x3ProjScale(f32 X, f32 Y) {
	return (f32_m3x3) { .E = { 
		   X, 0.0f, 0.0f,
		0.0f,    Y, 0.0f,
		0.0f, 0.0f, 1.0f
	} };
}

f32_m3x3 F32M3x3ProjRotation(f32 Theta) {
	return (f32_m3x3) { .E = { 
		cos(Theta), -sin(Theta), 0.0f,
		sin(Theta),  cos(Theta), 0.0f,
		0.0f, 0.0f, 1.0f
	} };
}

f32_m3x3 F32M3x3ProjTranslate(f32 X, f32 Y) {
	return (f32_m3x3) { .E = { 
		1.0f, 0.0f, X,
		0.0f, 1.0f, Y,
		0.0f, 0.0f, 1.0f
	} };
}

f32_v2 F32V2(f32 X, f32 Y) {
	return (f32_v2) {X, Y};
}

f32_v2 F32V2Multiply(f32_v2 V, f32 S) {
	return (f32_v2) {V.X * S, V.Y * S};
}

f32_v2 F32V2Hadamard(f32_v2 A, f32_v2 B) {
	return (f32_v2) {A.X * B.X, A.Y * B.Y};
}

f32_v2 F32V2HadamardDiv(f32_v2 A, f32_v2 B) {
	return (f32_v2) {A.X / B.X, A.Y / B.Y};
}

f32_v2 F32V2Add(f32_v2 A, f32_v2 B) {
	return (f32_v2) {A.X + B.X, A.Y + B.Y};
}

f32_v2 F32V2Sub(f32_v2 A, f32_v2 B) {
	return (f32_v2) {A.X - B.X, A.Y - B.Y};
}

f32 F32V2Dot(f32_v2 A, f32_v2 B) {
	return A.X * B.X + A.Y * B.Y;
}

f32_v2 F32V2Norm(f32_v2 V) {
	return F32V2Multiply(V, 1 / Sqrt(F32V2Dot(V, V)));
}

f32_v2 F32V2Average(f32_v2 A, f32_v2 B) {
	return (f32_v2) {(A.X + B.X)/2.0f, (A.Y + B.Y)/2.0f};
}

f32_v2 F32V2Rotate(f32_v2 V, float Rotation)
{
	return F32V2(cos(Rotation) * V.X - sin(Rotation) * V.Y, sin(Rotation) * V.X + cos(Rotation) * V.Y);
}

f32_v2 F32V2Polar(float R, float Theta)
{
	return F32V2(cos(Theta) * R, sin(Theta) * R);
}

f32_v2 F32V2FromI32V2(i32_v2 V) {
	return (f32_v2) {(float) V.X, (float) V.Y};
}