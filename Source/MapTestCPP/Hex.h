// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class MAPTESTCPP_API HexCoord
{
public:
	float q = 0.0f;
	float r = 0.0f;
	float s = 0.0f;

	int32 Q = 0;
	int32 R = 0;
	int32 S = 0;

	HexCoord();
	~HexCoord();

	void Set(const HexCoord& Coord);

};

/**
 * 
 */
class MAPTESTCPP_API Hex
{
private:
	HexCoord Coord;

private:
	void UpdateCoordFloat(float a, float b);
	void UpdateCoordInt(int32 A, int32 B);

public:
	Hex();
	Hex(FVector Cube);
	Hex(FVector2D Axial);
	Hex(FIntVector CubeInt);
	Hex(FIntPoint AxialInt);

	void SetCube(const FVector Cube);
	void SetAxial(const FVector2D Axial);
	void SetCubeInt(const FIntVector CubeInt);
	void SetAxialInt(const FIntPoint AxialInt);
	void SetHex(const Hex& hex);

	HexCoord GetCoord();

	static Hex Round(const Hex& InHex);
	static Hex Subtract(const Hex& InHexA, const Hex& InHexB);
	static float Distance(const Hex& InHexA, const Hex& InHexB);
	static Hex PosToHex(const FVector2D& Point, float Size);


	FORCEINLINE bool operator==(const Hex& InHex) const
	{
		return (Coord.Q == InHex.Coord.Q && Coord.R == InHex.Coord.R);
	}

	FORCEINLINE FIntPoint ToIntPoint()
	{
		return FIntPoint(Coord.Q, Coord.R);
	}

	~Hex();
};

