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
	LoadTileIndices,
	LoadTiles,
	CreateTilesVertices,
	SetTilesPosZ,
	CalTilesNormal,
	SetTilesLowBlockLevel,
	InitCheckTerrainConnection,
	BreakMaxLowBlockTilesToChunk,
	CheckChunksConnection,
	FindTilesIsland,
	SetTilesPlainLevel,
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

	//Block data
	int32 LowBlockLevelMax = 0;
	TSet<int32> MaxLowBlockTileIndices;

	//Create tiles vertices tmp data
	TArray<FVector> TileVerticesVectors;

	//Hex ISM mesh
	float HexInstanceScale = 1.0;
	FVector HexInstMeshUpVec;

	//Plain data
	int32 PlainLevelMax = 0;

	//Check Terrain connection data
	TSet<int32> CheckConnectionReached;
	TArray<TSet<int32>> MaxLowBlockTileChunks;
	TQueue<int32> CheckConnectionFrontier;

protected:
	//Mesh
	UPROPERTY(VisibleAnywhere, Category = "Custom|HexInstMesh")
	class UInstancedStaticMeshComponent* HexInstMesh;
	UPROPERTY(BlueprintReadOnly)
	class UProceduralMeshComponent* MouseOverMesh;

	//Material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* MouseOverMaterialIns;

	//Timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Timer")
	float DefaultTimerRate = 0.01f;

	//Path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString ParamsDataPath = FString(TEXT("Data/Params.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TileIndicesDataPath = FString(TEXT("Data/TileIndices.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TilesDataPath = FString(TEXT("Data/Tiles.data"));

	//Loop BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTileIndicesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTilesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CreateTilesVerticesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesPosZLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CalTilesNormalLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesLowBlockLevelLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData BreakMaxLowBlockTilesToChunkLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CheckChunksConnectionLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData FindTilesIslandLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesPlainLevelLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData AddTilesInstanceLoopData;

	//Load from data file
	UPROPERTY(BlueprintReadOnly)
	TArray<FStructHexTileData> Tiles;
	UPROPERTY(BlueprintReadOnly)
	TMap<FIntPoint, int32> TileIndices;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> MouseOverVertices;
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> MouseOverTriangles;

	//Workflow
	UPROPERTY(BlueprintReadOnly)
	Enum_HexGridWorkflowState WorkflowState = Enum_HexGridWorkflowState::InitWorkflow;

	//Progress
	/*UPROPERTY(BlueprintReadOnly)
	int32 ProgressTarget = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 ProgressCurrent = 0;*/

	//common
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Common")
	bool bShowGrid = false;

	//Params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params")
	int32 ParamNum = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	float TileSize = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	int32 GridRange = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	int32 NeighborRange = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	FRotator HexInstMeshRot = FRotator(0.0, 30.0, 0.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	float HexInstMeshOffsetZ = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom|Params")
	float HexInstMeshSize = 100.0;

	//MouseOver
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|MouseOver", meta = (ClampMin = "0"))
	int32 MouseOverShowRadius = 1;

	//Block
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TerrainAltitudeLowBlockRatio = 0.3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TerrainSlopeLowBlockRatio = 0.3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TerrainSlopePlainRatio = 0.1;

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

	//Load tiles indices data
	void LoadTileIndicesFromFile();
	void LoadTileIndices(std::ifstream& ifs);
	void ParseTileIndexLine(const FString& line);

	//Load tiles data
	void LoadTilesFromFile();
	void LoadTiles(std::ifstream& ifs);
	void ParseTileLine(const FString& line, FStructHexTileData& Data);
	void ParseIndex(const FString& ValueStr, const FString& KeyStr);
	void ParseAxialCoord(const FString& Str, FStructHexTileData& Data);
	void ParsePosition2D(const FString& Str, FStructHexTileData& Data);
	void ParseNeighbors(const FString& Str, FStructHexTileData& Data);
	void ParseNeighborInfo(const FString& Str, FStructHexTileNeighbors& Neighbors, int32 Radius);
	
	//Parse string to other data type
	void ParseIntPoint(const FString& Str, FIntPoint& Point);
	void ParseVector2D(const FString& Str, FVector2D& Vec2D);
	void ParseVector(const FString& Str, FVector& Vec);
	void ParseInt(const FString& Str, int32& value);

	//Loop Function for all workflow of tiles loop
	void TilesLoopFunction(TFunction<void()> InitFunc, TFunction<void(int32 LoopIndex)> LoopFunc, 
		FStructLoopData& LoopData, Enum_HexGridWorkflowState State);

	//Parse tiles vertices
	void CreateTilesVertices();
	void InitTileVerticesVertors();
	void CreateTileVertices(int32 Index);

	//Set tiles PosZ
	void SetTilesPosZ();
	void SetTilePosZ(int32 Index);
	void SetTileCenterPosZ(FStructHexTileData& Data);
	void SetTileVerticesPosZ(FStructHexTileData& Data);

	//Calculate Normal
	void CalTilesNormal();
	void InitCalTilesNormal();
	void CalTileNormal(int32 Index);
	//void CalTileNormalByNeighbor(FStructHexTileData& Data, int32 Index, FVector& OutNormal);

	//Set low block level
	void SetTilesLowBlockLevel();
	void InitSetTilesLowBlockLevel();
	bool SetTileLowBlock(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 BlockLevel);
	void SetTileLowBlockLevelByNeighbors(int32 Index);
	bool SetTileLowBlockLevelByNeighbor(FStructHexTileData& Data, int32 Index);

	//Check Terrain connection
	void CheckTerrainConnection();
	void InitCheckTerrainConnection();
	void CheckTerrainConnectionWorkflow();
	void BreakMaxLowBlockTilesToChunk();
	void CheckChunksConnection();
	bool FindTwoChunksConnection(TSet<int32>& ChunkStart, TSet<int32>& ChunkObj);
	void CheckTerrainConnectionNotPass();

	//Find tiles Island
	void FindTilesIsland();
	void FindTileIsLand(int32 Index);
	bool Find_LBLM_By_LBL3(int32 Index);

	//Set Plain level
	void SetTilesPlainLevel();
	void InitSetTilesPlainLevel();
	bool SetTilePlain(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 PlainLevel);
	void SetTilePlainLevelByNeighbors(int32 Index);
	void SetTilePlainLevelByNeighbor(FStructHexTileData& Data, int32 Index);

	//Add Grid tiles ISM
	void AddTilesInstance();
	void InitAddTilesInstance();
	int32 AddTileInstance(FStructHexTileData Data);

	void AddTileInstanceByLowBlock(int32 Index);
	void AddTileInstanceDataByLowBlock(int32 TileIndex, int32 InstanceIndex);

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
