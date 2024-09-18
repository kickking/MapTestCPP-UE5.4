// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructDefine.h"
#include "Hex.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include <iostream>
#include <fstream>

#include "HexGrid.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(HexGrid, Log, All);

class ATerrain;

UENUM(BlueprintType)
enum class Enum_HexGridWorkflowState : uint8
{
	InitWorkflow,
	WaitTerrainInit,
	LoadParams,
	LoadTiles,
	LoadTileIndices,
	LoadVertices,
	LoadTriangles,
	SetTilesPosZ,
	SetVerticesPosZ,
	SetTerrainLowBlockLevel,
	SetVertexColors,
	DrawMesh,
	Done,
	Error
};

UENUM(BlueprintType)
enum class Enum_BlockMode : uint8
{
	LowBlock,
	MediumBlock,
	HighBlock
};

UCLASS()
class MAPTESTCPP_API AHexGrid : public AActor
{
	GENERATED_BODY()
	
private:
	//Delimiter
	FString PipeDelim = FString(TEXT("|"));
	FString CommaDelim = FString(TEXT(","));
	FString SpaceDelim = FString(TEXT(" "));
	FString ColonDelim = FString(TEXT(":"));

	//Delegate
	FTimerDynamicDelegate WorkflowDelegate;

	//ifstream for data load
	std::ifstream DataLoadStream;

	//Terrain
	ATerrain* Terrain;

	//Mouse over
	Hex MouseOverHex;

protected:
	//Mesh
	UPROPERTY(BlueprintReadOnly)
	class UProceduralMeshComponent* HexGridMesh;
	UPROPERTY(BlueprintReadOnly)
	class UProceduralMeshComponent* MouseOverMesh;

	//Material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* HexGridLineMaterialIns;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* HexGridLineVCMaterialIns;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* MouseOverMaterialIns;

	//Vetex Colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|VextexColors")
	bool bUseBlockVextexColors = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|VextexColors")
	FLinearColor LineVertexColor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|VextexColors")
	Enum_BlockMode VertexColorsShowMode = Enum_BlockMode::LowBlock;

	//Timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Timer")
	float DefaultTimerRate = 0.01f;

	//Path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString ParamsDataPath = FString(TEXT("Data/Params.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TilesDataPath = FString(TEXT("Data/Tiles.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TileIndicesDataPath = FString(TEXT("Data/TileIndices.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString VerticesDataPath = FString(TEXT("Data/Vertices.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TrianglesDataPath = FString(TEXT("Data/Triangles.data"));

	//Altitude
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Altitude")
	bool bFitAltitude = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Altitude")
	float AltitudeOffset = 100.0f;

	//Loop BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTilesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTileIndicesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadVerticesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTrianglesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesPosZLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetVerticesPosZLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTerrainLowBlockLevelLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetVertexColorsLoopData;

	//Load from data file
	UPROPERTY(BlueprintReadOnly)
	TArray<FStructHexTileData> Tiles;
	UPROPERTY(BlueprintReadOnly)
	TMap<FIntPoint, int32> TileIndices;

	//Render variables (Load from data file)
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> GridVertices;
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> GridTriangles;
	UPROPERTY(BlueprintReadOnly)
	TArray<FLinearColor> GridVertexColors;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> MouseOverVertices;
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> MouseOverTriangles;

	//Workflow
	UPROPERTY(BlueprintReadOnly)
	Enum_HexGridWorkflowState WorkflowState = Enum_HexGridWorkflowState::InitWorkflow;

	//Progress
	UPROPERTY(BlueprintReadOnly)
	int32 ProgressTarget = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 ProgressCurrent = 0;

	//common
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Common")
	bool bShowGrid = false;

	//Params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params")
	int32 ParamNum = 4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	float TileSize = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	int32 GridRange = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	float GridLineRatio = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	int32 NeighborRange = 10;

	//MouseOver
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|MouseOver", meta = (ClampMin = "0"))
	int32 MouseOverShowRadius = 1;

	//Block
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block")
	float TerrainLowBlockPosZ = 0.0;
	
public:	
	// Sets default values for this actor's properties
	AHexGrid();

private:

	//Timer delegate
	void BindDelegate();

	//Create Workflow
	UFUNCTION()
		void CreateHexGridFlow();

	//Init workflow
	void InitWorkflow();
	void InitLoopData();

	//Wait terrain noise
	void WaitTerrain();

	//Read file func
	bool GetValidFilePath(const FString& RelPath, FString& FullPath);

	//Load param
	void LoadParamsFromFile();
	void LoadParams(std::ifstream& ifs);
	bool ParseParams(const FString& line);

	//Load tiles data
	void LoadTilesFromFile();
	void LoadTiles(std::ifstream& ifs);
	void ParseTileLine(const FString& line, FStructHexTileData& Data);
	void ParseAxialCoord(const FString& Str, FStructHexTileData& Data);
	void ParsePosition2D(const FString& Str, FStructHexTileData& Data);
	void ParseNeighbors(const FString& Str, FStructHexTileData& Data);
	void ParseNeighborInfo(const FString& Str, FStructHexTileNeighbors& Neighbors, int32 Radius);
	
	//Load tiles indices data
	void LoadTileIndicesFromFile();
	void LoadTileIndices(std::ifstream& ifs);
	void ParseTileIndexLine(const FString& line);
	
	//Load vertices data
	void LoadVerticesFromFile();
	void LoadVertices(std::ifstream& ifs);
	void ParseVerticesLine(const FString& line);

	//Load triangles data
	void LoadTrianglesFromFile();
	void LoadTriangles(std::ifstream& ifs);
	void ParseTrianglesLine(const FString& line);

	//Parse string to other data type
	void ParseIntPoint(const FString& Str, FIntPoint& Point);
	void ParseVector2D(const FString& Str, FVector2D& Vec2D);
	void ParseVector(const FString& Str, FVector& Vec);
	void ParseInt(const FString& Str, int32& value);

	//Set tiles PosZ
	void SetTilesPosZ();
	void SetTilePosZ(FStructHexTileData& Data);

	//Set Vertices PosZ
	void SetVerticesPosZ();
	void SetVertexPosZ(FVector& Vertex);

	//Set block level
	void SetTerrainLowBlockLevel();
	void SetTileLowBlockLevel(FStructHexTileData& Data);
	void SetLowBlockLevelByNeighbors(FStructHexTileData& Data, int32 Index);

	//Set Vertex Colors
	void SetVertexColors();
	void SetVertexColor(const FLinearColor& Color);
	void SetVertexColorByBlockMode(int32 Index);
	void SetVertexColorByLowBlock(int32 Index);

	//Mesh create
	void CreateHexGridLineMesh();
	void SetHexGridLineMaterial();

	//Mouse over
	Hex PosToHex(const FVector2D& Point, float Size);
	void SetMouseOverVertices();
	void SetHexVerticesByIndex(int32 Index);
	void SetMouseOverTriangles();
	void CreateMouseOverMesh();
	void SetMouseOverMaterial();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Mouse over grid
	UFUNCTION(BlueprintCallable)
		void MouseOverGrid(const FVector2D& MousePos);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsWorkFlowDone()
	{
		return WorkflowState == Enum_HexGridWorkflowState::Done;
	}

};
