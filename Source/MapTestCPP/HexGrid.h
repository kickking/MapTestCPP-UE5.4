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
	LoadParams,
	LoadTileIndices,
	LoadTiles,
	LoadNeighbors,
	CreateTilesVertices,
	WaitTerrain,
	SetTilesPosZ,
	CalTilesNormal,
	SetTilesWalkingBlockLevel,
	SetTilesWalkingBlockLevelEx,
	InitCheckTerrainWalkingConnection,
	BreakMaxWalkingBlockTilesToChunk,
	CheckChunksWalkingConnection,
	FindTilesIsland,
	SetTilesBuildingBlockLevel,
	SetTilesBuildingBlockLevelEx,
	DrawMesh,
	Done,
	Error
};

UENUM(BlueprintType)
enum class Enum_BlockMode : uint8
{
	BuildingBlock,
	WalkingBlock,
	FlyingBlock,
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
	//FString ColonDelim = FString(TEXT(":"));

	//Delegate
	FTimerDynamicDelegate WorkflowDelegate;
	FTimerDynamicDelegate CheckMouseOverDelegate;

	//Timer handle
	FTimerHandle CheckTimerHandle;

	//ifstream for data load
	std::ifstream DataLoadStream;

	//Terrain
	ATerrain* Terrain;

	//Mouse over
	Hex MouseOverHex;

	//Block data
	int32 WalkingBlockLevelMax = 0;
	TSet<int32> MaxWalkingBlockTileIndices;

	//Create tiles vertices tmp data
	TArray<FVector> TileVerticesVectors;

	//Hex ISM mesh
	float HexInstanceScale = 1.0;
	FVector HexInstMeshUpVec = FVector(0.f, 0.f, 1.0);

	//BuildingBlock data
	int32 BuildingBlockLevelMax = 0;

	//Check Terrain connection data
	TSet<int32> CheckWalkingConnectionReached;
	TArray<TSet<int32>> MaxWalkingBlockTileChunks;
	TQueue<int32> CheckWalkingConnectionFrontier;

	APlayerController* Controller;

protected:
	//Mesh
	UPROPERTY(VisibleAnywhere, Category = "Custom|HexInstMesh")
	class UInstancedStaticMeshComponent* HexInstMesh;
	UPROPERTY(VisibleAnywhere, Category = "Custom|HexInstMesh")
	class UInstancedStaticMeshComponent* MouseOverInstMesh;

	//Material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* MouseOverMaterialIns;

	//Timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Timer")
	float DefaultTimerRate = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Timer")
	float CheckTimerRate = 0.02f;

	//Path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString ParamsDataPath = FString(TEXT("Data/Params.data"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TileIndicesDataPath = FString(TEXT("Data/TileIndices.data"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString TilesDataPath = FString(TEXT("Data/Tiles.data"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
	FString NeighborsDataPathPrefix = FString(TEXT("Data/N"));

	//Loop BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTileIndicesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadTilesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData LoadNeighborsLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CreateTilesVerticesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesPosZLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CalTilesNormalLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesWalkingBlockLevelLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesWalkingBlockLevelExLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData BreakMaxWalkingBlockTilesToChunkLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData FindTilesIslandLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesBuildingBlockLevelLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData SetTilesBuildingBlockLevelExLoopData;
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


	//common
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Common")
	bool bShowGrid = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Common")
	Enum_BlockMode GridShowMode = Enum_BlockMode::WalkingBlock;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom|MouseOver")
	float MouseOverInstMeshOffsetZ = 2.0;

	//Block
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block|Walking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkingBlockAltitudeRatio = 0.3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block|Walking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WalkingBlockSlopeRatio = 0.3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block|Building", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BuildingBlockAltitudeRatio = 0.3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Block|Building", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BuildingBlockSlopeRatio = 0.1;

	//Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom|Input")
	class UInputMappingContext* InputMapping;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Input")
	class UInputAction* IncMouseOverRadiusAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Input")
	class UInputAction* DecMouseOverRadiusAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Input")
	float TraceLength = 1000000;

public:	
	// Sets default values for this actor's properties
	AHexGrid();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	//Timer delegate
	void BindDelegate();

	//Input
	void EnablePlayer();
	void AddInputMappingContext();
	void BindEnchancedInputAction();
	void OnIncMouseOverRadius();
	void OnDecMouseOverRadius();

	/*void OnMouseOver();*/
	void StartCheckMouseOver();
	void StopCheckMouseOver();

	UFUNCTION()
	void CheckMouseOver();

	//Create Workflow
	UFUNCTION()
	void CreateHexGridFlow();

	//Init workflow
	void InitWorkflow();
	void InitLoopData();

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
	
	//Load NeighborData
	void LoadNeighborsFromFile();
	void CreateNeighborPath(FString& NeighborPath, int32 Radius);
	bool LoadNeighbors(std::ifstream& ifs, int32 Radius);
	void ParseNeighborsLine(const FString& Str, int32 Index, int32 Radius);
	void ParseNeighbors(const FString& Str, int32 Index, int32 Radius);

	//Parse string to other data type
	void ParseIntPoint(const FString& Str, FIntPoint& Point);
	void ParseVector2D(const FString& Str, FVector2D& Vec2D);
	void ParseVector(const FString& Str, FVector& Vec);
	void ParseInt(const FString& Str, int32& value);

	//Loop Function for all workflow of tiles loop
	bool TilesLoopFunction(TFunction<void()> InitFunc, TFunction<void(int32 LoopIndex)> LoopFunc, 
		FStructLoopData& LoopData, Enum_HexGridWorkflowState State);

	//Parse tiles vertices
	void CreateTilesVertices();
	void InitTileVerticesVertors();
	void CreateTileVertices(int32 Index);

	//Wait terrain noise
	void WaitTerrain();

	//Set tiles PosZ
	void SetTilesPosZ();
	void SetTilePosZ(int32 Index);
	void SetTileCenterPosZ(FStructHexTileData& Data);
	void SetTileVerticesPosZ(FStructHexTileData& Data);

	//Calculate Normal
	void CalTilesNormal();
	void InitCalTilesNormal();
	void CalTileNormal(int32 Index);

	//Set walking block level
	void SetTilesWalkingBlockLevel();
	void InitSetTilesWalkingBlockLevel();
	bool SetTileWalkingBlock(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 BlockLevel);
	void SetTileWalkingBlockLevelByNeighbors(int32 Index);
	bool SetTileWalkingBlockLevelByNeighbor(FStructHexTileData& Data, int32 Index);

	//Set walking block level extension
	void SetTilesWalkingBlockLevelEx();
	void InitSetTilesWalkingBlockLevelEx();
	void SetTileWalkingBlockLevelByNeighborsEx(int32 Index);

	//Check Terrain walking connection
	void CheckTerrainWalkingConnection();
	void InitCheckTerrainWalkingConnection();
	void CheckTerrainWalkingConnectionWorkflow();
	void BreakMaxWalkingBlockTilesToChunk();
	void CheckChunksWalkingConnection();
	bool FindTwoChunksWalkingConnection(TSet<int32>& ChunkStart, TSet<int32>& ChunkObj);
	void CheckTerrainWalkingConnectionNotPass(int32 ChunkIndex);

	//Find tiles Island
	void FindTilesIsland();
	void FindTileIsLand(int32 Index);
	bool Find_LBLM_By_LBL3(int32 Index);

	//Set Building Block level
	void SetTilesBuildingBlockLevel();
	void InitSetTilesBuildingBlockLevel();
	bool SetTileBuildingBlock(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 BuildingBlockLevel);
	void SetTileBuildingBlockLevelByNeighbors(int32 Index);
	bool SetTileBuildingBlockLevelByNeighbor(FStructHexTileData& Data, int32 Index);

	//Set Building Block level extension
	void SetTilesBuildingBlockLevelEx();
	void InitSetTilesBuildingBlockExLevel();
	void SetTileBuildingBlockLevelByNeighborsEx(int32 Index);

	//Add Grid tiles ISM
	void AddTilesInstance();
	void InitAddTilesInstance();
	int32 AddTileInstance(int32 Index);
	int32 AddISM(int32 Index, UInstancedStaticMeshComponent* ISM, float ZOffset = 0.f);

	void AddTileInstanceByWalkingBlock(int32 Index);
	void AddTileInstanceDataByWalkingBlock(int32 TileIndex, int32 InstanceIndex);

	bool IsInMapRange(int32 Index);
	bool IsInMapRange(const FStructHexTileData& Tile);

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

private:
	//Mouse over
	Hex PosToHex(const FVector2D& Point, float Size);
	void AddMouseOverTilesInstance();
	void RemoveMouseOverTilesInstance();

};
