#pragma once

#include "StructDefine.generated.h"

USTRUCT(BlueprintType)
struct FStructLoopData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 LoopCountLimit = 3000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Rate = 0.01f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 LoopDepthLimit = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<int32> IndexSaved = {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsInitialized = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Count = 0;

};

USTRUCT(BlueprintType)
struct FStructHexTileNeighbors
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Radius;

	UPROPERTY()
	TArray<FIntPoint> Tiles;
};

USTRUCT(BlueprintType)
struct FStructHexTileData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FIntPoint AxialCoord;

	UPROPERTY(BlueprintReadOnly)
	FVector2D Position2D;

	UPROPERTY(BlueprintReadOnly)
	float PositionZ;

	UPROPERTY(BlueprintReadOnly)
	float AvgPositionZ;

	UPROPERTY(BlueprintReadOnly)
	TArray<FStructHexTileNeighbors> Neighbors;

	UPROPERTY(BlueprintReadOnly)
	int32 TerrainLowBlockLevel = -1; // -1 means no block

	UPROPERTY(BlueprintReadOnly)
	int32 TerrainHighBlockLevel = -1; // -1 means no block
};

USTRUCT(BlueprintType)
struct FStructHeightMapping
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float RangeMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float RangeMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MappingMin;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MappingMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float RangeMinOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float RangeMaxOffset;

};