// Fill out your copyright notice in the Description page of Project Settings.


#include "Hex.h"

#include <Math/UnrealMathUtility.h>
#include <Kismet/KismetMathLibrary.h>

HexCoord::HexCoord()
{
}

HexCoord::~HexCoord()
{
}

void HexCoord::Set(const HexCoord& Coord)
{
	q = Coord.q;
	r = Coord.r;
	s = Coord.s;
	Q = Coord.Q;
	R = Coord.R;
	S = Coord.S;
}

Hex::Hex()
{
}

Hex::~Hex()
{
}

Hex::Hex(FVector Cube)
{
	UpdateCoordFloat(Cube.X, Cube.Y);
}

Hex::Hex(FVector2D Axial)
{
	UpdateCoordFloat(Axial.X, Axial.Y);
}

Hex::Hex(FIntVector CubeInt)
{
	UpdateCoordInt(CubeInt.X, CubeInt.Y);
}

Hex::Hex(FIntPoint AxialInt)
{
	UpdateCoordInt(AxialInt.X, AxialInt.Y);
}

void Hex::UpdateCoordFloat(float a, float b)
{
	Coord.q = a;
	Coord.r = b;
	Coord.s = -Coord.q - Coord.r;

	Coord.Q = int32(Coord.q);
	Coord.R = int32(Coord.r);
	Coord.S = -Coord.Q - Coord.R;
}

void Hex::UpdateCoordInt(int32 A, int32 B)
{
	Coord.Q = A;
	Coord.R = B;
	Coord.S = -Coord.Q - Coord.R;

	Coord.q = float(Coord.Q);
	Coord.r = float(Coord.R);
	Coord.s = float(Coord.S);
}

void Hex::SetCube(const FVector Cube)
{
	UpdateCoordFloat(Cube.X, Cube.Y);
}

void Hex::SetAxial(const FVector2D Axial)
{
	UpdateCoordFloat(Axial.X, Axial.Y);
}

void Hex::SetCubeInt(const FIntVector CubeInt)
{
	UpdateCoordInt(CubeInt.X, CubeInt.Y);
}

void Hex::SetAxialInt(const FIntPoint AxialInt)
{
	UpdateCoordInt(AxialInt.X, AxialInt.Y);
}

void Hex::SetHex(const Hex& hex)
{
	Coord.Set(hex.Coord);
}

HexCoord Hex::GetCoord()
{
	return Coord;
}

Hex Hex::Round(const Hex& InHex)
{
	float q = FMath::RoundHalfFromZero(InHex.Coord.q);
	float r = FMath::RoundHalfFromZero(InHex.Coord.r);
	float s = FMath::RoundHalfFromZero(InHex.Coord.s);

	float q_diff = FMath::Abs(q - InHex.Coord.q);
	float r_diff = FMath::Abs(r - InHex.Coord.r);
	float s_diff = FMath::Abs(s - InHex.Coord.s);

	if (q_diff > r_diff && q_diff > s_diff) {
		q = -r - s;
	}
	else if (r_diff > s_diff) {
		r = -q - s;
	}
	else {
		s = -q - r;
	}

	return Hex(FVector(q, r, s));
}

Hex Hex::Subtract(const Hex& InHexA, const Hex& InHexB)
{
	return Hex(FVector2D(InHexA.Coord.q - InHexB.Coord.q, InHexA.Coord.r - InHexB.Coord.r));
}

float Hex::Distance(const Hex& InHexA, const Hex& InHexB)
{
	Hex hex = Subtract(InHexA, InHexB);
	return (FMath::Abs(hex.Coord.q) + FMath::Abs(hex.Coord.r) + FMath::Abs(hex.Coord.s)) / 2.0f;
}

Hex Hex::PosToHex(const FVector2D& Point, float Size)
{
	float q = (2.0 / 3.0 * Point.X) / Size;
	float r = (-1.0 / 3.0 * Point.X + FMath::Sqrt(3.0) / 3.0 * Point.Y) / Size;

	return Round(Hex(FVector2D(q, r)));
}