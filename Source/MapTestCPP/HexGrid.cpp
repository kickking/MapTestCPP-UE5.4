// Fill out your copyright notice in the Description page of Project Settings.


#include "HexGrid.h"
#include "FlowControlUtility.h"
#include "Terrain.h"

#include <Math/UnrealMathUtility.h>
#include <kismet/KismetStringLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>
#include <ProceduralMeshComponent.h>
#include <Components/InstancedStaticMeshComponent.h>
#include <TimerManager.h>

#include <string>

DEFINE_LOG_CATEGORY(HexGrid);

// Sets default values
AHexGrid::AHexGrid()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	HexInstMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HexInstMesh"));
	this->SetRootComponent(HexInstMesh);
	HexInstMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MouseOverMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MouseOverMesh"));
	MouseOverMesh->SetupAttachment(RootComponent);
	MouseOverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BindDelegate();
}

// Called when the game starts or when spawned
void AHexGrid::BeginPlay()
{
	Super::BeginPlay();

	WorkflowState = Enum_HexGridWorkflowState::InitWorkflow;
	CreateHexGridFlow();
}

void AHexGrid::BindDelegate()
{
	WorkflowDelegate.BindUFunction(Cast<UObject>(this), TEXT("CreateHexGridFlow"));
}

void AHexGrid::CreateHexGridFlow()
{
	switch (WorkflowState)
	{
	case Enum_HexGridWorkflowState::InitWorkflow:
		InitWorkflow();
		break;
	case Enum_HexGridWorkflowState::WaitTerrainInit:
		WaitTerrain();
		break;
	case Enum_HexGridWorkflowState::LoadParams:
		LoadParamsFromFile();
		break;
	case Enum_HexGridWorkflowState::LoadTileIndices:
		LoadTileIndicesFromFile();
		break;
	case Enum_HexGridWorkflowState::LoadTiles:
		LoadTilesFromFile();
		break;
	case Enum_HexGridWorkflowState::CreateTilesVertices:
		CreateTilesVertices();
		break;
	case Enum_HexGridWorkflowState::SetTilesPosZ:
		SetTilesPosZ();
		break;
	case Enum_HexGridWorkflowState::CalTilesNormal:
		CalTilesNormal();
		break;
	case Enum_HexGridWorkflowState::SetTilesLowBlockLevel:
		SetTilesLowBlockLevel();
		break;
	case Enum_HexGridWorkflowState::InitCheckTerrainConnection:
	case Enum_HexGridWorkflowState::BreakMaxLowBlockTilesToChunk:
	case Enum_HexGridWorkflowState::CheckChunksConnection:
		CheckTerrainConnection();
		break;
	case Enum_HexGridWorkflowState::FindTilesIsland:
		FindTilesIsland();
		break;
	case Enum_HexGridWorkflowState::SetTilesPlainLevel:
		SetTilesPlainLevel();
		break;
	case Enum_HexGridWorkflowState::DrawMesh:
		AddTilesInstance();
		break;
	case Enum_HexGridWorkflowState::Done:
		break;
	case Enum_HexGridWorkflowState::Error:
		UE_LOG(HexGrid, Warning, TEXT("CreateHexGridFlow Error!"));
		break;
	default:
		break;
	}
}

void AHexGrid::InitWorkflow()
{
	InitLoopData();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::WaitTerrainInit;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGrid, Log, TEXT("Init workflow done!"));
}

void AHexGrid::InitLoopData()
{
	FlowControlUtility::InitLoopData(LoadTileIndicesLoopData);
	FlowControlUtility::InitLoopData(LoadTilesLoopData);
	FlowControlUtility::InitLoopData(CreateTilesVerticesLoopData);
	FlowControlUtility::InitLoopData(SetTilesPosZLoopData);
	FlowControlUtility::InitLoopData(CalTilesNormalLoopData);
	FlowControlUtility::InitLoopData(SetTilesLowBlockLevelLoopData);

	FlowControlUtility::InitLoopData(BreakMaxLowBlockTilesToChunkLoopData);
	FlowControlUtility::InitLoopData(CheckChunksConnectionLoopData);

	FlowControlUtility::InitLoopData(FindTilesIslandLoopData);
	FlowControlUtility::InitLoopData(SetTilesPlainLevelLoopData);
	FlowControlUtility::InitLoopData(AddTilesInstanceLoopData);
}

void AHexGrid::WaitTerrain()
{
	FTimerHandle TimerHandle;
	TArray<AActor*> Out_Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATerrain::StaticClass(), Out_Actors);
	if (Out_Actors.Num() == 1) {
		Terrain = (ATerrain*)Out_Actors[0];
		if (Terrain->IsWorkFlowStepDone(Enum_TerrainWorkflowState::InitWorkflow)) {
			WorkflowState = Enum_HexGridWorkflowState::LoadParams;
			GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
			UE_LOG(HexGrid, Log, TEXT("Wait terrain noise done!"));
			return;
		}
	}
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	return;
}

bool AHexGrid::GetValidFilePath(const FString& RelPath, FString& FullPath)
{
	bool flag = false;
	FullPath = FPaths::ProjectDir().Append(RelPath);
	if (FPaths::FileExists(FullPath)) {
		flag = true;
	}
	return flag;
}

void AHexGrid::LoadParamsFromFile()
{
	FTimerHandle TimerHandle;
	FString FullPath;
	if (!GetValidFilePath(ParamsDataPath, FullPath)) {
		UE_LOG(HexGrid, Warning, TEXT("Params data file %s not exist!"), *ParamsDataPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		return;
	}

	DataLoadStream.open(*FullPath, std::ios::in);

	if (!DataLoadStream || !DataLoadStream.is_open()) {
		UE_LOG(HexGrid, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		return;
	}
	LoadParams(DataLoadStream);
}

void AHexGrid::LoadParams(std::ifstream& ifs)
{
	FTimerHandle TimerHandle;
	std::string line;
	std::getline(ifs, line);
	FString fline(line.c_str());
	if (!ParseParams(fline)) {
		ifs.close();
		UE_LOG(HexGrid, Warning, TEXT("Parse Parameters error!"));
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		return;
	}

	ifs.close();
	WorkflowState = Enum_HexGridWorkflowState::LoadTileIndices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGrid, Log, TEXT("Load params done!"));
}

bool AHexGrid::ParseParams(const FString& line)
{
	TArray<FString> StrArr;
	line.ParseIntoArray(StrArr, *PipeDelim, true);

	if (StrArr.Num() != ParamNum) {
		return false;
	}

	TileSize = UKismetStringLibrary::Conv_StringToFloat(StrArr[0]);
	GridRange = UKismetStringLibrary::Conv_StringToInt(StrArr[1]);
	NeighborRange = UKismetStringLibrary::Conv_StringToInt(StrArr[2]);
	return true;
}

void AHexGrid::LoadTilesFromFile()
{
	FTimerHandle TimerHandle;
	FString FullPath;
	if (!GetValidFilePath(TilesDataPath, FullPath)) {
		UE_LOG(HexGrid, Warning, TEXT("Tiles data file %s not exist!"), *TilesDataPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTilesLoopData.Rate, false);
		return;
	}

	if (!LoadTilesLoopData.IsInitialized) {
		LoadTilesLoopData.IsInitialized = true;
		DataLoadStream.open(*FullPath, std::ios::in);
	}

	if (!DataLoadStream || !DataLoadStream.is_open()) {
		UE_LOG(HexGrid, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTilesLoopData.Rate, false);
		return;
	}

	LoadTiles(DataLoadStream);
}

void AHexGrid::LoadTiles(std::ifstream& ifs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	std::string line;
	while (std::getline(ifs, line))
	{
		FString fline(line.c_str());
		FStructHexTileData Data;
		ParseTileLine(fline, Data);
		Tiles.Add(Data);
		Count++;

		FlowControlUtility::SaveLoopData(this, LoadTilesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
	}

	ifs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::CreateTilesVertices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTilesLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load tiles done!"));
}

void AHexGrid::ParseTileLine(const FString& line, FStructHexTileData& Data)
{
	TArray<FString> StrArr;
	line.ParseIntoArray(StrArr, *PipeDelim, true);
	/*ParseIndex(StrArr[0], StrArr[1]);
	ParseAxialCoord(StrArr[1], Data);
	ParsePosition2D(StrArr[2], Data);
	ParseNeighbors(StrArr[3], Data);*/

	ParseAxialCoord(StrArr[0], Data);
	ParsePosition2D(StrArr[1], Data);
	ParseNeighbors(StrArr[2], Data);
}

void AHexGrid::ParseIndex(const FString& ValueStr, const FString& KeyStr)
{
	FIntPoint key;
	int32 value;
	ParseIntPoint(KeyStr, key);
	ParseInt(ValueStr, value);
	TileIndices.Add(key, value);
}


void AHexGrid::ParseAxialCoord(const FString& Str, FStructHexTileData& Data)
{
	ParseIntPoint(Str, Data.AxialCoord);
}

void AHexGrid::ParsePosition2D(const FString& Str, FStructHexTileData& Data)
{
	ParseVector2D(Str, Data.Position2D);
}

void AHexGrid::ParseNeighbors(const FString& Str, FStructHexTileData& Data)
{
	TArray<FString> StrArr;
	Str.ParseIntoArray(StrArr, *ColonDelim, true);
	for (int32 i = 0; i < StrArr.Num(); i++)
	{
		FStructHexTileNeighbors Neighbors;
		ParseNeighborInfo(StrArr[i], Neighbors, i + 1);
		Data.Neighbors.Add(Neighbors);
	}
}

void AHexGrid::ParseNeighborInfo(const FString& Str, FStructHexTileNeighbors& Neighbors, int32 Radius)
{
	Neighbors.Radius = Radius;
	int32 Count = 0;
	TArray<FString> StrArr;
	Str.ParseIntoArray(StrArr, *SpaceDelim, true);
	for (int32 i = 0; i < StrArr.Num(); i++)
	{
		FIntPoint Point;
		ParseIntPoint(StrArr[i], Point);
		if (TileIndices.Contains(Point)) {
			Neighbors.Tiles.Add(Point);
			Count++;
		}
	}
	Neighbors.Count = Count;
}

void AHexGrid::LoadTileIndicesFromFile()
{
	FTimerHandle TimerHandle;
	FString FullPath;
	if (!GetValidFilePath(TileIndicesDataPath, FullPath)) {
		UE_LOG(HexGrid, Warning, TEXT("TileIndices data file %s not exist!"), *TileIndicesDataPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTileIndicesLoopData.Rate, false);
		return;
	}

	if (!LoadTileIndicesLoopData.IsInitialized) {
		LoadTileIndicesLoopData.IsInitialized = true;
		DataLoadStream.open(*FullPath, std::ios::in);
	}

	if (!DataLoadStream || !DataLoadStream.is_open()) {
		UE_LOG(HexGrid, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTileIndicesLoopData.Rate, false);
		return;
	}

	LoadTileIndices(DataLoadStream);
}

void AHexGrid::LoadTileIndices(std::ifstream& ifs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	std::string line;
	while (std::getline(ifs, line))
	{
		FString fline(line.c_str());
		ParseTileIndexLine(fline);
		Count++;

		FlowControlUtility::SaveLoopData(this, LoadTileIndicesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
	}

	ifs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::LoadTiles;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTileIndicesLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load tile indices done!"));
}

void AHexGrid::ParseTileIndexLine(const FString& line)
{
	TArray<FString> StrArr;
	FIntPoint key;
	int32 value;
	line.ParseIntoArray(StrArr, *PipeDelim, true);
	ParseIntPoint(StrArr[0], key);
	ParseInt(StrArr[1], value);
	TileIndices.Add(key, value);

}

void AHexGrid::ParseIntPoint(const FString& Str, FIntPoint& Point)
{
	TArray<FString> StrArr;
	Str.ParseIntoArray(StrArr, *CommaDelim, true);
	Point.X = UKismetStringLibrary::Conv_StringToInt(StrArr[0]);
	Point.Y = UKismetStringLibrary::Conv_StringToInt(StrArr[1]);
}

void AHexGrid::ParseVector2D(const FString& Str, FVector2D& Vec2D)
{
	TArray<FString> StrArr;
	Str.ParseIntoArray(StrArr, *CommaDelim, true);
	Vec2D.X = UKismetStringLibrary::Conv_StringToFloat(StrArr[0]);
	Vec2D.Y = UKismetStringLibrary::Conv_StringToFloat(StrArr[1]);
}

void AHexGrid::ParseVector(const FString& Str, FVector& Vec)
{
	TArray<FString> StrArr;
	Str.ParseIntoArray(StrArr, *CommaDelim, true);
	Vec.X = UKismetStringLibrary::Conv_StringToFloat(StrArr[0]);
	Vec.Y = UKismetStringLibrary::Conv_StringToFloat(StrArr[1]);
	Vec.Z = UKismetStringLibrary::Conv_StringToFloat(StrArr[2]);
}

void AHexGrid::ParseInt(const FString& Str, int32& value)
{
	value = UKismetStringLibrary::Conv_StringToInt(Str);
}

void AHexGrid::TilesLoopFunction(TFunction<void()> InitFunc, TFunction<void(int32 LoopIndex)> LoopFunc,
	FStructLoopData& LoopData, Enum_HexGridWorkflowState State)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	if (InitFunc) {
		InitFunc();
	}

	int32 i = LoopData.IndexSaved[0];
	for (; i < Tiles.Num(); i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, LoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		LoopFunc(i);
		Count++;
	}

	FTimerHandle TimerHandle;
	WorkflowState = State;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoopData.Rate, false);
}

void AHexGrid::CreateTilesVertices()
{
	TilesLoopFunction([this]() { InitTileVerticesVertors(); }, [this](int32 i) { CreateTileVertices(i); }, 
		CreateTilesVerticesLoopData, Enum_HexGridWorkflowState::SetTilesPosZ);
	UE_LOG(HexGrid, Log, TEXT("Parse tiles vertices done!"));
}

void AHexGrid::InitTileVerticesVertors()
{
	FVector Vec(1.0, 0.0, 0.0);
	FVector ZAxis(0.0, 0.0, 1.0);
	for (int32 i = 0; i <= 5; i++)
	{
		FVector TileVector = Vec.RotateAngleAxis(i * 60, ZAxis) * TileSize;
		TileVerticesVectors.Add(TileVector);
	}
}

void AHexGrid::CreateTileVertices(int32 Index)
{
	FVector Center(Tiles[Index].Position2D.X, Tiles[Index].Position2D.Y, 0);
	for (int32 i = 0; i <= 5; i++) {
		FVector Vertex = Center + TileVerticesVectors[i];
		Tiles[Index].VerticesPostion2D.Add(FVector2D(Vertex.X, Vertex.Y));
	}
}

void AHexGrid::SetTilesPosZ()
{
	TilesLoopFunction(nullptr, [this](int32 i) { SetTilePosZ(i); },
		SetTilesPosZLoopData, Enum_HexGridWorkflowState::CalTilesNormal);
	UE_LOG(HexGrid, Log, TEXT("Set tiles pos z done!"));

}

void AHexGrid::SetTilePosZ(int32 Index)
{
	SetTileCenterPosZ(Tiles[Index]);
	SetTileVerticesPosZ(Tiles[Index]);
}

void AHexGrid::SetTileCenterPosZ(FStructHexTileData& Data)
{
	Data.PositionZ = Terrain->GetAltitudeByPos2D(Data.Position2D, this);
}

void AHexGrid::SetTileVerticesPosZ(FStructHexTileData& Data)
{
	float Sum = 0.0;
	for (int32 i = 0; i <= 5; i++) {
		float z = Terrain->GetAltitudeByPos2D(Data.VerticesPostion2D[i], this);
		Data.VerticesPositionZ.Add(z);
		Sum += z;
	}
	Data.AvgPositionZ = Sum / 6.0;
}

void AHexGrid::CalTilesNormal()
{
	TilesLoopFunction([this]() { InitCalTilesNormal(); }, [this](int32 i) { CalTileNormal(i); },
		CalTilesNormalLoopData, Enum_HexGridWorkflowState::SetTilesLowBlockLevel);

	UE_LOG(HexGrid, Log, TEXT("Calculate tiles normal done!"));
}

void AHexGrid::InitCalTilesNormal()
{
	HexInstMeshUpVec = HexInstMesh->GetUpVector();
}

void AHexGrid::CalTileNormal(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];

	FVector TileNormal(0, 0, 0);
	for (int32 i = 0; i < 2; i++) {
		FVector v0(Data.VerticesPostion2D[i].X, Data.VerticesPostion2D[i].Y, Data.VerticesPositionZ[i]);
		FVector v1(Data.VerticesPostion2D[2 + i].X, Data.VerticesPostion2D[2 + i].Y, Data.VerticesPositionZ[2 + i]);
		FVector v2(Data.VerticesPostion2D[4 + i].X, Data.VerticesPostion2D[4 + i].Y, Data.VerticesPositionZ[4 + i]);
		TileNormal += FVector::CrossProduct(v2 - v0, v2 - v1);
	}
	TileNormal.Normalize();
	Data.Normal = TileNormal;

	float DotProduct = FVector::DotProduct(HexInstMeshUpVec, TileNormal);
	Data.AngleToUp = acosf(DotProduct);

}

void AHexGrid::SetTilesLowBlockLevel()
{
	TilesLoopFunction([this]() { InitSetTilesLowBlockLevel(); }, [this](int32 i) { SetTileLowBlockLevelByNeighbors(i); },
		SetTilesLowBlockLevelLoopData, Enum_HexGridWorkflowState::InitCheckTerrainConnection);
	
	UE_LOG(HexGrid, Log, TEXT("Set tiles low block level done!"));

}

void AHexGrid::InitSetTilesLowBlockLevel()
{
	LowBlockLevelMax = NeighborRange + 1;
}

bool AHexGrid::SetTileLowBlock(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 BlockLevel)
{
	if (CheckData.AvgPositionZ > TerrainAltitudeLowBlockRatio * Terrain->GetTileAltitudeMultiplier()
		|| CheckData.AvgPositionZ < Terrain->GetWaterBase()
		|| CheckData.AngleToUp > (PI * TerrainSlopeLowBlockRatio / 2.0)) {
		Data.TerrainLowBlockLevel = BlockLevel;
		return true;
	}
	return false;
}

void AHexGrid::SetTileLowBlockLevelByNeighbors(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (SetTileLowBlock(Data, Data, 0)) {
		return;
	}

	for (int32 i = 0; i < Data.Neighbors.Num(); i++)
	{
		if (SetTileLowBlockLevelByNeighbor(Data, i)) {
			return;
		}
	}
	Data.TerrainLowBlockLevel = LowBlockLevelMax;
	MaxLowBlockTileIndices.Add(Index);
}

bool AHexGrid::SetTileLowBlockLevelByNeighbor(FStructHexTileData& Data, int32 Index)
{
	FStructHexTileNeighbors Neighbors = Data.Neighbors[Index];
	int32 TileIndex;
	for (int32 i = 0; i < Neighbors.Tiles.Num(); i++)
	{
		FIntPoint key = Neighbors.Tiles[i];
		if (!TileIndices.Contains(key)) {
			continue;
		}
		
		TileIndex = TileIndices[key];
		if (SetTileLowBlock(Data, Tiles[TileIndex], Neighbors.Radius))
		{
			return true;
		}
	}
	return false;
}

void AHexGrid::CheckTerrainConnection()
{
	CheckTerrainConnectionWorkflow();
}

void AHexGrid::CheckTerrainConnectionWorkflow()
{
	switch (WorkflowState)
	{
	case Enum_HexGridWorkflowState::InitCheckTerrainConnection:
		InitCheckTerrainConnection();
		WorkflowState = Enum_HexGridWorkflowState::BreakMaxLowBlockTilesToChunk;
	case Enum_HexGridWorkflowState::BreakMaxLowBlockTilesToChunk:
		BreakMaxLowBlockTilesToChunk();
		break;
	case Enum_HexGridWorkflowState::CheckChunksConnection:
		CheckChunksConnection();
		break;
	default:
		break;
	}
}

void AHexGrid::InitCheckTerrainConnection()
{
	MaxLowBlockTileChunks.Empty();
}

void AHexGrid::BreakMaxLowBlockTilesToChunk()
{
	
	int32 Count = 0;
	TArray<int32> Indices = {};
	bool SaveLoopFlag = false;

	while (!MaxLowBlockTileIndices.IsEmpty()) 
	{
		if (CheckConnectionFrontier.IsEmpty()) {
			CheckConnectionReached.Empty();
			TArray<int32> arr = MaxLowBlockTileIndices.Array();
			CheckConnectionFrontier.Enqueue(arr[0]);
			MaxLowBlockTileIndices.Remove(arr[0]);

			CheckConnectionReached.Add(arr[0]);
		}
		
		while (!CheckConnectionFrontier.IsEmpty())
		{
			FlowControlUtility::SaveLoopData(this, BreakMaxLowBlockTilesToChunkLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
			if (SaveLoopFlag) {
				return;
			}

			int32 current;
			CheckConnectionFrontier.Dequeue(current);

			FStructHexTileNeighbors Neighbors = Tiles[current].Neighbors[0];
			for (int32 i = 0; i < Neighbors.Tiles.Num(); i++) {
				FIntPoint key = Neighbors.Tiles[i];
				if (!TileIndices.Contains(key)) {
					continue;
				}
				if (!CheckConnectionReached.Contains(TileIndices[key])
					&& Tiles[TileIndices[key]].TerrainLowBlockLevel == LowBlockLevelMax 
					&& MaxLowBlockTileIndices.Contains(TileIndices[key])) {
					CheckConnectionFrontier.Enqueue(TileIndices[key]);
					CheckConnectionReached.Add(TileIndices[key]);
					MaxLowBlockTileIndices.Remove(TileIndices[key]);
				}
			}

			Count++;
		}
		MaxLowBlockTileChunks.Add(CheckConnectionReached);
	}

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::CheckChunksConnection;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, BreakMaxLowBlockTilesToChunkLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Break Max LowBlock Tiles To Chunk done!"));
}

void AHexGrid::CheckChunksConnection()
{
	MaxLowBlockTileChunks.Sort([](const TSet<int32>& A, const TSet<int32>& B) {
		return A.Num() > B.Num();
	});
	TSet<int32> objChunk = MaxLowBlockTileChunks[0];
	
	for (int32 i = 1; i < MaxLowBlockTileChunks.Num(); i++) {
		if (!FindTwoChunksConnection(MaxLowBlockTileChunks[i], objChunk)) {
			CheckTerrainConnectionNotPass();
			return;
		}
		objChunk.Append(MaxLowBlockTileChunks[i]);
	}
	
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::FindTilesIsland;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CheckChunksConnectionLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Check Chunks Connection done!"));
	UE_LOG(HexGrid, Log, TEXT("Check Terrain Connection pass!"));
}

bool AHexGrid::FindTwoChunksConnection(TSet<int32>& ChunkStart, TSet<int32>& ChunkObj)
{
	int32 StartIndex = ChunkStart.Array()[0];
	
	TQueue<int32> frontier;
	frontier.Enqueue(StartIndex);
	TSet<int32> reached;
	reached.Add(StartIndex);

	while (!frontier.IsEmpty())
	{
		int32 current;
		frontier.Dequeue(current);
		if (ChunkObj.Contains(current)) {
			return true;
		}
		FStructHexTileNeighbors Neighbors = Tiles[current].Neighbors[0];
		for (int32 i = 0; i < Neighbors.Tiles.Num(); i++) {
			FIntPoint key = Neighbors.Tiles[i];
			if (!TileIndices.Contains(key)) {
				continue;
			}
			if (!reached.Contains(TileIndices[key]) 
				&& Tiles[TileIndices[key]].TerrainLowBlockLevel >= 3) {
				frontier.Enqueue(TileIndices[key]);
				reached.Add(TileIndices[key]);
			}
		}
	}
	return false;
}

void AHexGrid::CheckTerrainConnectionNotPass()
{
	TerrainAltitudeLowBlockRatio += 0.1;
	TerrainSlopeLowBlockRatio += 0.1;

	InitLoopData();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::SetTilesLowBlockLevel;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CheckChunksConnectionLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Check Chunks Connection done!"));
	UE_LOG(HexGrid, Warning, TEXT("Check Terrain Connection not pass!"));
}

void AHexGrid::FindTilesIsland()
{
	TilesLoopFunction(nullptr, [this](int32 i) { FindTileIsLand(i); },
		FindTilesIslandLoopData, Enum_HexGridWorkflowState::SetTilesPlainLevel);
	UE_LOG(HexGrid, Log, TEXT("Find tiles island done!"));
}

void AHexGrid::FindTileIsLand(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (Data.TerrainLowBlockLevel == LowBlockLevelMax) {
		Data.TerrainIsLand = false;
	}
	else if (Data.TerrainLowBlockLevel < LowBlockLevelMax && Data.TerrainLowBlockLevel >= 3) {
		Data.TerrainIsLand = !Find_LBLM_By_LBL3(Index);
	}
	else if (Data.TerrainLowBlockLevel < 3 && Data.TerrainLowBlockLevel >= 1) {
		for (int32 i = Data.TerrainLowBlockLevel; i > 0; i--) {
			FStructHexTileNeighbors Neighbors = Data.Neighbors[2 - i];
			for (int32 j = 0; j < Neighbors.Tiles.Num(); j++) {
				FIntPoint key = Neighbors.Tiles[j];
				if (!TileIndices.Contains(key)) {
					continue;
				}
				if (Tiles[TileIndices[key]].TerrainLowBlockLevel == 3) {
					if (Find_LBLM_By_LBL3(TileIndices[key])) {
						Data.TerrainIsLand = false;
						return;
					}
				}
			}
		}
		Data.TerrainIsLand = true;
	}
	else {
		Data.TerrainIsLand = true;
	}
	
}

/*Find LowBlockLevelMax tile by LowBlockLevel=3 tile*/
bool AHexGrid::Find_LBLM_By_LBL3(int32 Index)
{
	TQueue<int32> frontier;
	frontier.Enqueue(Index);
	TSet<int32> reached;
	reached.Add(Index);

	while (!frontier.IsEmpty())
	{
		int32 current;
		frontier.Dequeue(current);
		if (Tiles[current].TerrainLowBlockLevel == LowBlockLevelMax) {
			return true;
		}

		FStructHexTileNeighbors Neighbors = Tiles[current].Neighbors[0];
		for (int32 i = 0; i < Neighbors.Tiles.Num(); i++) {
			FIntPoint key = Neighbors.Tiles[i];
			if (!TileIndices.Contains(key)) {
				continue;
			}
			if (!reached.Contains(TileIndices[key]) && Tiles[TileIndices[key]].TerrainLowBlockLevel >=3) {
				frontier.Enqueue(TileIndices[key]);
				reached.Add(TileIndices[key]);
			}
		}
	}
	return false;
}

void AHexGrid::SetTilesPlainLevel()
{
	TilesLoopFunction([this]() { InitSetTilesPlainLevel(); }, [this](int32 i) { SetTilePlainLevelByNeighbors(i); },
		SetTilesPlainLevelLoopData, Enum_HexGridWorkflowState::DrawMesh);

	UE_LOG(HexGrid, Log, TEXT("Set tiles plain level done!"));
}

void AHexGrid::InitSetTilesPlainLevel()
{
	PlainLevelMax = NeighborRange + 1;
}

bool AHexGrid::SetTilePlain(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 PlainLevel)
{
	if (CheckData.AvgPositionZ > TerrainAltitudeLowBlockRatio * Terrain->GetTileAltitudeMultiplier()
		|| CheckData.AvgPositionZ < Terrain->GetWaterBase()
		|| CheckData.AngleToUp >(PI * TerrainSlopePlainRatio / 2.0)) {
		Data.TerrainPlainLevel = PlainLevel;
		return true;
	}
	return false;
}

void AHexGrid::SetTilePlainLevelByNeighbors(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (SetTilePlain(Data, Data, 0)) {
		return;
	}

	for (int32 i = 0; i < Data.Neighbors.Num(); i++)
	{
		SetTilePlainLevelByNeighbor(Data, i);
		if (Data.TerrainPlainLevel != PlainLevelMax) {
			return;
		}
	}
}

void AHexGrid::SetTilePlainLevelByNeighbor(FStructHexTileData& Data, int32 Index)
{
	FStructHexTileNeighbors Neighbors = Data.Neighbors[Index];
	int32 TileIndex;
	for (int32 i = 0; i < Neighbors.Tiles.Num(); i++)
	{
		FIntPoint key = Neighbors.Tiles[i];
		if (!TileIndices.Contains(key)) {
			continue;
		}

		TileIndex = TileIndices[key];
		if (SetTilePlain(Data, Tiles[TileIndex], Neighbors.Radius))
		{
			return;
		}
	}
	Data.TerrainPlainLevel = PlainLevelMax;
}

void AHexGrid::AddTilesInstance()
{
	if (!bShowGrid) {
		FTimerHandle TimerHandle;
		WorkflowState = Enum_HexGridWorkflowState::Done;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		UE_LOG(HexGrid, Log, TEXT("Don't show grid!"));
		return;
	}

	TilesLoopFunction([this]() { InitAddTilesInstance(); }, [this](int32 i) { AddTileInstanceByLowBlock(i); },
		AddTilesInstanceLoopData, Enum_HexGridWorkflowState::Done);

	UE_LOG(HexGrid, Log, TEXT("Add tiles instance done!"));

}

void AHexGrid::InitAddTilesInstance()
{
	HexInstMesh->NumCustomDataFloats = 3;
	HexInstanceScale = TileSize / HexInstMeshSize;
}

int32 AHexGrid::AddTileInstance(FStructHexTileData Data)
{
	FStructHexTileData tile = Data;
	FVector HexLoc(tile.Position2D.X, tile.Position2D.Y, tile.AvgPositionZ + HexInstMeshOffsetZ);
	FVector HexScale(HexInstanceScale);

	FVector RotationAxis = FVector::CrossProduct(HexInstMeshUpVec, tile.Normal);
	RotationAxis.Normalize();

	FQuat Quat = FQuat(RotationAxis, tile.AngleToUp);
	FQuat NewQuat = Quat * HexInstMeshRot.Quaternion();

	FTransform HexTransform(NewQuat.Rotator(), HexLoc, HexScale);

	int32 InstanceIndex = HexInstMesh->AddInstance(HexTransform);
	return InstanceIndex;
}

void AHexGrid::AddTileInstanceByLowBlock(int32 Index)
{
	FStructHexTileData tile = Tiles[Index];
	if (tile.TerrainLowBlockLevel > 0 && !tile.TerrainIsLand
		&& FMath::Abs<float>(tile.Position2D.X) < Terrain->GetWidth() / 2
		&& FMath::Abs<float>(tile.Position2D.Y) < Terrain->GetHeight() / 2)
	{
		int32 InstanceIndex = AddTileInstance(tile);
		AddTileInstanceDataByLowBlock(Index, InstanceIndex);
	}
}

void AHexGrid::AddTileInstanceDataByLowBlock(int32 TileIndex, int32 InstanceIndex)
{
	float H = 0.0;
	if (!Tiles[TileIndex].TerrainIsLand) {
		H = 120.0f / float(LowBlockLevelMax) * float(Tiles[TileIndex].TerrainLowBlockLevel);
	}
	else {
		H = 300.0;
	}

	float S = 0.5 + 0.5 / float(PlainLevelMax) * float(Tiles[TileIndex].TerrainPlainLevel);
	FLinearColor LinearColor = UKismetMathLibrary::HSVToRGB(H, S, 1.0, 1.0);

	TArray<float> CustomData = { LinearColor.R, LinearColor.G, LinearColor.B };
	HexInstMesh->SetCustomData(InstanceIndex, CustomData, true);
}

// Called every frame
void AHexGrid::Tick(float DeltaTime)
{
	//Super::Tick(DeltaTime);

}

void AHexGrid::MouseOverGrid(const FVector2D& MousePos)
{
	if (!IsWorkFlowDone() || Terrain == nullptr || !Terrain->IsWorkFlowDone()) {
		return;
	}

	Hex hex = Hex::PosToHex(MousePos, TileSize);
	if (MouseOverHex == hex) {
		return;
	}

	MouseOverHex.SetHex(hex);
	SetMouseOverVertices();
	SetMouseOverTriangles();
	CreateMouseOverMesh();
	SetMouseOverMaterial();

}

Hex AHexGrid::PosToHex(const FVector2D& Point, float Size)
{
	float q = (2.0 / 3.0 * Point.X) / Size;
	float r = (-1.0 / 3.0 * Point.X + FMath::Sqrt(3.0) / 3.0 * Point.Y) / Size;

	float qfz = FMath::RoundFromZero(q);
	float rfz = FMath::RoundFromZero(r);
	float qtz = FMath::RoundToZero(q);
	float rtz = FMath::RoundToZero(r);

	Hex hex1(FVector2D(qfz, rfz));
	Hex hex2(FVector2D(qfz, rtz));
	Hex hex3(FVector2D(qtz, rfz));
	Hex hex4(FVector2D(qtz, rtz));

	int32 index1 = TileIndices[hex1.ToIntPoint()];
	int32 index2 = TileIndices[hex2.ToIntPoint()];
	int32 index3 = TileIndices[hex3.ToIntPoint()];
	int32 index4 = TileIndices[hex4.ToIntPoint()];

	FVector2D center1 = Tiles[index1].Position2D;
	FVector2D center2 = Tiles[index2].Position2D;
	FVector2D center3 = Tiles[index3].Position2D;
	FVector2D center4 = Tiles[index4].Position2D;

	float dist1 = FVector2D::Distance(Point, center1);
	float dist2 = FVector2D::Distance(Point, center2);
	float dist3 = FVector2D::Distance(Point, center3);
	float dist4 = FVector2D::Distance(Point, center4);

	Hex OutHex;
	if (dist1 < dist2 && dist1 < dist3 && dist1 < dist4) {
		OutHex.SetHex(hex1);
	}
	else if (dist2 < dist3 && dist2 < dist4) {
		OutHex.SetHex(hex2);
	}
	else if (dist3 < dist4) {
		OutHex.SetHex(hex3);
	}
	else {
		OutHex.SetHex(hex4);
	}
	return OutHex;
}

void AHexGrid::SetMouseOverVertices()
{
	MouseOverVertices.Empty();
	int32 index = TileIndices[MouseOverHex.ToIntPoint()];
	FStructHexTileData Tile = Tiles[index];
	SetHexVerticesByIndex(index);

	MouseOverShowRadius = MouseOverShowRadius < NeighborRange ? MouseOverShowRadius : NeighborRange;
	for (int32 i = 0; i < MouseOverShowRadius; i++)
	{
		TArray<FIntPoint> RingTiles = Tile.Neighbors[i].Tiles;
		for (int32 j = 0; j < RingTiles.Num(); j++)
		{
			SetHexVerticesByIndex(TileIndices[RingTiles[j]]);
		}
	}
}

void AHexGrid::SetHexVerticesByIndex(int32 Index)
{
	for (int32 i = 0; i < 6; i++)
	{
		//MouseOverVertices.Add(GridVertices[Index * 12 + i * 2]);
	}
}

void AHexGrid::SetMouseOverTriangles()
{
	MouseOverTriangles.Empty();
	for (int32 i = 0; i < MouseOverVertices.Num(); i += 6)
	{
		for (int32 j = 0; j < 4; j++)
		{
			MouseOverTriangles.Add(i);
			MouseOverTriangles.Add(i + j + 2);
			MouseOverTriangles.Add(i + j + 1);
		}
	}
}

void AHexGrid::CreateMouseOverMesh()
{
	MouseOverMesh->CreateMeshSection(0, MouseOverVertices, MouseOverTriangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
}

void AHexGrid::SetMouseOverMaterial()
{
	MouseOverMesh->SetMaterial(0, MouseOverMaterialIns);
}