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

f32 F32Sqrt(f32 N) {
	return sqrtf(N);
}

f64 F64Sqrt(f64 N) {
	return sqrt(N);
}

f32 F32Sin(f32 A) {
	return sinf(A);
}

f64 F64Sin(f64 A) {
	return sin(A);
}

f32 F32Cos(f32 A) {
	return cosf(A);
}

f64 F64Cos(f64 A) {
	return cos(A);
}

f32 F32Tan(f32 A) {
	return tanf(A);
}

f64 F64Tan(f64 A) {
	return tan(A);
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

i32_v2 I32V2Rotate(i32_v2 V, f32 Rotation)
{
	return I32V2(
		(i32) (cos(Rotation) * V.X) - (i32) (sin(Rotation) * V.Y),
		(i32) (sin(Rotation) * V.X) + (i32) (cos(Rotation) * V.Y)
	);
}

i32_v2 I32V2Polar(f32 R, f32 Theta)
{
	return I32V2((i32) (cos(Theta) * R), (i32) (sin(Theta) * R));
}

i32_v2 I32V2Lerp(i32_v2 A, i32_v2 B, f32 T)
{
	return I32V2((i32) (B.X * T + A.X * (1 - T)), (i32) (B.Y * T + A.Y * (1 - T)));
}

i32_v2 I32V2FromF32V2(f32_v2 V) {
	return (i32_v2) {(i32) V.X, (i32) V.Y};
}

i32_rect I32Rect(i32 X0, i32 X1, i32 Y0, i32 Y1)
{
	return (i32_rect) { X0, X1, Y0, Y1 };
}

i32_rect I32RectFromPositionSize(i32_v2 Position, i32_v2 Size)
{
	return (i32_rect) {
		Position.X,
		Position.X + Size.X,
		Position.Y,
		Position.Y + Size.Y
	};
}

i32_rect I32RectFromCenterSize(i32_v2 Center, i32_v2 Size)
{
	i32_v2 HalfSize = I32V2Divide(Size, 2);
	return (i32_rect) {
		Center.X - HalfSize.X,
		Center.X + HalfSize.X,
		Center.Y - HalfSize.Y,
		Center.Y + HalfSize.Y
	};
}

i32 I32RectArea(i32_rect Rect)
{
	return (Rect.X1 - Rect.X0) * (Rect.Y1 - Rect.Y0);
}

i32_v2 I32RectCenter(i32_rect Rect)
{
	return I32V2(
		(Rect.X0 + Rect.X1) / 2,
		(Rect.Y0 + Rect.Y1) / 2
	);
}

i32_v2 I32RectSize(i32_rect Rect)
{
	return I32V2(
		Rect.X1 - Rect.X0,
		Rect.Y1 - Rect.Y0
	);
}

bool32 I32RectContainsPoint(i32_rect Rect, i32_v2 Point)
{
	if (Point.X < Rect.X0 || Point.X > Rect.X1) return false;
	if (Point.Y < Rect.Y0 || Point.Y > Rect.Y1) return false;
	return true;
}

i32_rect I32RectExpand(i32_rect Rect, i32 Amount)
{
	return (i32_rect) {
		Rect.X0 - Amount,
		Rect.X1 + Amount,
		Rect.Y0 - Amount,
		Rect.Y1 + Amount
	};
}

i32_rect I32RectTranslate(i32_rect Rect, i32_v2 Offset)
{
	return (i32_rect) {
		Rect.X0 + Offset.X,
		Rect.X1 + Offset.X,
		Rect.Y0 + Offset.Y,
		Rect.Y1 + Offset.Y
	};
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
	return U8V4((u8) (V.X * 255), (u8)(V.Y * 255), (u8)(V.Z * 255), (u8)(V.W * 255));
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

f32 F32V3Norm(f32_v3 V) {
	return F32Sqrt(F32V3Dot(V, V));
}

f32_v3 F32V3Normalize(f32_v3 V) {
	return F32V3Multiply(V, 1 / F32V3Norm(V));
}

f32_v3 F32V3Average(f32_v3 A, f32_v3 B) {
	return (f32_v3) {(A.X + B.X)/2.0f, (A.Y + B.Y)/2.0f, (A.Z + B.Z)/2.0f};
}

f32_v3 F32V3FromI32V3(i32_v3 V) {
	return (f32_v3) {(f32) V.X, (f32) V.Y, (f32) V.Z};
}

///////

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

f32 F32V4Norm(f32_v4 V)
{
	return F32Sqrt(F32V4Dot(V, V));
}

f32_v4 F32V4Normalize(f32_v4 V) {
	return F32V4Multiply(V, F32V4Norm(V));
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
		(f32) cos(Theta), (f32) -sin(Theta), 0.0f,
		(f32) sin(Theta), (f32)  cos(Theta), 0.0f,
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

f32 F32V2Norm(f32_v2 V) {
	return F32Sqrt(F32V2Dot(V, V));
}

f32_v2 F32V2Normalize(f32_v2 V) {
	return F32V2Multiply(V, 1 / F32V2Norm(V));
}

f32_v2 F32V2Average(f32_v2 A, f32_v2 B) {
	return (f32_v2) {(A.X + B.X)/2.0f, (A.Y + B.Y)/2.0f};
}

f32_v2 F32V2Rotate(f32_v2 V, f32 Rotation)
{
	return F32V2(
		(f32) (cos(Rotation) * V.X) - (f32) (sin(Rotation) * V.Y),
		(f32) (sin(Rotation) * V.X) + (f32) (cos(Rotation) * V.Y)
	);
}

f32_v2 F32V2Polar(f32 R, f32 Theta)
{
	return F32V2((f32) (cos(Theta) * R), (f32) (sin(Theta) * R));
}

f32_v2 F32V2FromI32V2(i32_v2 V) {
	return (f32_v2) {(f32) V.X, (f32) V.Y};
}

//////

f32_v4 F32M4x4VMultiply(f32_m4x4 M, f32_v4 V)
{
	return F32V4(F32V4Dot(M.Rows[0], V), F32V4Dot(M.Rows[1], V), F32V4Dot(M.Rows[2], V), F32V4Dot(M.Rows[3], V));
}

f32_m4x4 F32M4x4Multiply(f32_m4x4 A, f32_m4x4 B)
{
	f32_v4 Row1 = A.Rows[0];
	f32_v4 Row2 = A.Rows[1];
	f32_v4 Row3 = A.Rows[2];
	f32_v4 Row4 = A.Rows[3];

	f32_v4 Col1 = F32V4(B.E[0], B.E[4], B.E[8], B.E[12]);
	f32_v4 Col2 = F32V4(B.E[1], B.E[5], B.E[9], B.E[13]);
	f32_v4 Col3 = F32V4(B.E[2], B.E[6], B.E[10], B.E[14]);
	f32_v4 Col4 = F32V4(B.E[3], B.E[7], B.E[11], B.E[15]);

	return (f32_m4x4) { .E = {
		F32V4Dot(Col1, Row1), F32V4Dot(Col2, Row1), F32V4Dot(Col3, Row1), F32V4Dot(Col4, Row1),
		F32V4Dot(Col1, Row2), F32V4Dot(Col2, Row2), F32V4Dot(Col3, Row2), F32V4Dot(Col4, Row2),
		F32V4Dot(Col1, Row3), F32V4Dot(Col2, Row3), F32V4Dot(Col3, Row3), F32V4Dot(Col4, Row3),
		F32V4Dot(Col1, Row4), F32V4Dot(Col2, Row4), F32V4Dot(Col3, Row4), F32V4Dot(Col4, Row4),
	} };
}

f32_m4x4 F32M4x4Identity()
{
	return (f32_m4x4) { .E = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	} };
}

f32_m4x4 F32M4x4ProjScale(f32 X, f32 Y, f32 Z)
{
	return (f32_m4x4) { .E = {
		X,    0.0f, 0.0f, 0.0f,
		0.0f, Y,    0.0f, 0.0f,
		0.0f, 0.0f,    Z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	} };
}

f32_m4x4 F32M4x4ProjRotation(f32_q Q)
{
	f32 I2 = Q.I * Q.I;
	f32 J2 = Q.J * Q.J;
	f32 K2 = Q.K * Q.K;

	f32 IR = Q.I * Q.W;
	f32 JR = Q.J * Q.W;
	f32 KR = Q.K * Q.W;

	f32 IJ = Q.I * Q.J;
	f32 JK = Q.J * Q.K;
	f32 IK = Q.I * Q.K;

	return (f32_m4x4) { .E = {
		1.0f - 2.0f * (J2 + K2), 2.0f * (IJ - KR),        2.0f * (IK + JR),        0.0f,
		2.0f * (IJ + KR),        1.0f - 2.0f * (I2 + K2), 2.0f * (JK - IR),        0.0f,
		2.0f * (IK - JR),        2.0f * (JK + IR),        1.0f - 2.0f * (I2 + J2), 0.0f,
		0.0f,                    0.0f,                    0.0f,                    1.0f
	} };
}

f32_m4x4 F32M4x4ProjTranslate(f32 X, f32 Y, f32 Z)
{
	return (f32_m4x4) { .E = {
		1.0f, 0.0f, 0.0f, X,
		0.0f, 1.0f, 0.0f, Y,
		0.0f, 0.0f, 1.0f, Z,
		0.0f, 0.0f, 0.0f, 1.0f
	} };
}

f32_m4x4 F32M4x4ProjPerspective(f32 Fov, f32 Aspect, f32 Near, f32 Far)
{
	f32 F = 1.0f / F32Tan(Fov / 2.0f);
	f32 Range = Near - Far;

	return (f32_m4x4) { .E = {
		F / Aspect, 0.0f, 0.0f,                 0.0f,
		0.0f,       F,    0.0f,                 0.0f,
		0.0f,       0.0f, (Near + Far) / Range, Near * Far / Range,
		0.0f,       0.0f, -1.0f,                0.0f
	} };
}

f32_q F32QuaternionRotation(f32 Theta, f32 X, f32 Y, f32 Z)
{
	f32 Sin = F32Sin(Theta);
	f32 Cos = F32Cos(Theta);

	f32_v3 Direction = F32V3Normalize(F32V3(X, Y, Z));

	return F32QuaternionNormalize(F32Quaternion2(Cos, F32V3Multiply(Direction, Sin)));
}

f32_q F32Quaternion(f32 W, f32 I, f32 J, f32 K)
{
	return (f32_q) { .W = W, .I = I, .J = J, .K = K };
}

f32_q F32Quaternion2(f32 Real, f32_v3 Imaginary)
{
	return (f32_q) { .Real = Real, .Imaginary = Imaginary };
}

f32 F32QuaternionNorm(f32_q Q)
{
	return F32Sqrt(Q.W * Q.W + Q.I * Q.I + Q.J * Q.J + Q.K * Q.K);
}

f32_q F32QuaternionNormalize(f32_q Q)
{
	return F32QuaternionMultiplyS(Q, 1.0f / F32QuaternionNorm(Q));
}

f32_q F32QuaternionMultiply(f32_q A, f32_q B)
{
	f32 WW = A.W * B.W;
	f32 WI = A.W * B.I;
	f32 WJ = A.W * B.J;
	f32 WK = A.W * B.K;

	f32 IW = A.I * B.W;
	f32 II = A.I * B.I;
	f32 IJ = A.I * B.J;
	f32 IK = A.I * B.K;

	f32 JW = A.J * B.W;
	f32 JI = A.J * B.I;
	f32 JJ = A.J * B.J;
	f32 JK = A.J * B.K;

	f32 KW = A.K * B.W;
	f32 KI = A.K * B.I;
	f32 KJ = A.K * B.J;
	f32 KK = A.K * B.K;

	return F32Quaternion(WW - II - JJ - KK, WI + IW + JK - KJ, WJ + JW - IK + KI, WK + KW + IJ - JI);
}

f32_q F32QuaternionMultiplyS(f32_q Q, f32 S)
{
	return F32Quaternion(Q.W * S, Q.I * S, Q.J * S, Q.K * S);
}

f32_q F32QuaternionAdd(f32_q A, f32_q B)
{
	return F32Quaternion(A.W + B.W, A.I + B.I, A.J + B.J, A.K + B.K);
}