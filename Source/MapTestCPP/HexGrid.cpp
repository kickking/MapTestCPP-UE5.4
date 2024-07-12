// Fill out your copyright notice in the Description page of Project Settings.


#include "HexGrid.h"
#include "FlowControlUtility.h"
#include "Terrain.h"

#include <Math/UnrealMathUtility.h>
#include <kismet/KismetStringLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>
#include <ProceduralMeshComponent.h>
#include <TimerManager.h>

#include <string>

DEFINE_LOG_CATEGORY(HexGrid);

// Sets default values
AHexGrid::AHexGrid()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	HexGridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexGridMesh"));
	this->SetRootComponent(HexGridMesh);
	HexGridMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
	case Enum_HexGridWorkflowState::WaitTerrainNoise:
		WaitTerrainNoise();
		break;
	case Enum_HexGridWorkflowState::LoadParams:
		LoadParamsFromFile();
		break;
	case Enum_HexGridWorkflowState::LoadTiles:
		LoadTilesFromFile();
		break;
	case Enum_HexGridWorkflowState::LoadTileIndices:
		LoadTileIndicesFromFile();
		break;
	case Enum_HexGridWorkflowState::LoadVertices:
		LoadVerticesFromFile();
		break;
	case Enum_HexGridWorkflowState::LoadTriangles:
		LoadTrianglesFromFile();
		break;
	case Enum_HexGridWorkflowState::SetTilesPosZ:
		SetTilesPosZ();
		break;
	case Enum_HexGridWorkflowState::SetVerticesPosZ:
		SetVerticesPosZ();
		break;
	case Enum_HexGridWorkflowState::SetTerrainLowBlockLevel:
		SetTerrainLowBlockLevel();
		break;
	case Enum_HexGridWorkflowState::SetVertexColors:
		SetVertexColors();
		break;
	case Enum_HexGridWorkflowState::DrawMesh:
		CreateHexGridLineMesh();
		SetHexGridLineMaterial();
		WorkflowState = Enum_HexGridWorkflowState::Done;
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
	WorkflowState = Enum_HexGridWorkflowState::WaitTerrainNoise;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGrid, Log, TEXT("Init workflow done!"));
}

void AHexGrid::InitLoopData()
{
	FlowControlUtility::InitLoopData(LoadTilesLoopData);
	FlowControlUtility::InitLoopData(LoadTileIndicesLoopData);
	FlowControlUtility::InitLoopData(LoadVerticesLoopData);
	FlowControlUtility::InitLoopData(LoadTrianglesLoopData);
	FlowControlUtility::InitLoopData(SetTilesPosZLoopData);
	FlowControlUtility::InitLoopData(SetVerticesPosZLoopData);
	FlowControlUtility::InitLoopData(SetTerrainLowBlockLevelLoopData);
	FlowControlUtility::InitLoopData(SetVertexColorsLoopData);
}

void AHexGrid::WaitTerrainNoise()
{
	FTimerHandle TimerHandle;
	TArray<AActor*> Out_Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATerrain::StaticClass(), Out_Actors);
	if (Out_Actors.Num() == 1) {
		Terrain = (ATerrain*)Out_Actors[0];
		if (Terrain->IsCreateNoiseDone()) {
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
	WorkflowState = Enum_HexGridWorkflowState::LoadTiles;
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
	GridLineRatio = UKismetStringLibrary::Conv_StringToFloat(StrArr[2]);
	NeighborRange = UKismetStringLibrary::Conv_StringToInt(StrArr[3]);
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
	WorkflowState = Enum_HexGridWorkflowState::LoadTileIndices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTilesLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load tiles done!"));
}

void AHexGrid::ParseTileLine(const FString& line, FStructHexTileData& Data)
{
	TArray<FString> StrArr;
	line.ParseIntoArray(StrArr, *PipeDelim, true);
	ParseAxialCoord(StrArr[0], Data);
	ParsePosition2D(StrArr[1], Data);
	ParseNeighbors(StrArr[2], Data);
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
	TArray<FString> StrArr;
	Str.ParseIntoArray(StrArr, *SpaceDelim, true);
	for (int32 i = 0; i < StrArr.Num(); i++)
	{
		FIntPoint Point;
		ParseIntPoint(StrArr[i], Point);
		Neighbors.Tiles.Add(Point);
	}
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
	WorkflowState = Enum_HexGridWorkflowState::LoadVertices;
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

void AHexGrid::LoadVerticesFromFile()
{
	FTimerHandle TimerHandle;
	FString FullPath;
	if (!GetValidFilePath(VerticesDataPath, FullPath)) {
		UE_LOG(HexGrid, Warning, TEXT("Vertices data file %s not exist!"), *VerticesDataPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadVerticesLoopData.Rate, false);
		return;
	}

	if (!LoadVerticesLoopData.IsInitialized) {
		LoadVerticesLoopData.IsInitialized = true;
		DataLoadStream.open(*FullPath, std::ios::in);
	}

	if (!DataLoadStream || !DataLoadStream.is_open()) {
		UE_LOG(HexGrid, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadVerticesLoopData.Rate, false);
		return;
	}

	LoadVertices(DataLoadStream);
}

void AHexGrid::LoadVertices(std::ifstream& ifs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	std::string line;
	while (std::getline(ifs, line))
	{
		FString fline(line.c_str());
		ParseVerticesLine(fline);
		Count++;

		FlowControlUtility::SaveLoopData(this, LoadVerticesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
	}

	ifs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::LoadTriangles;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadVerticesLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load vertices done!"));
}

void AHexGrid::ParseVerticesLine(const FString& line)
{
	TArray<FString> StrArr;
	line.ParseIntoArray(StrArr, *PipeDelim, true);
	for (int32 i = 0; i < StrArr.Num(); i++)
	{
		FVector Vec;
		ParseVector(StrArr[i], Vec);
		GridVertices.Add(Vec);
	}
}

void AHexGrid::LoadTrianglesFromFile()
{
	FTimerHandle TimerHandle;
	FString FullPath;
	if (!GetValidFilePath(TrianglesDataPath, FullPath)) {
		UE_LOG(HexGrid, Warning, TEXT("Triangles data file %s not exist!"), *TrianglesDataPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTrianglesLoopData.Rate, false);
		return;
	}

	if (!LoadTrianglesLoopData.IsInitialized) {
		LoadTrianglesLoopData.IsInitialized = true;
		DataLoadStream.open(*FullPath, std::ios::in);
	}

	if (!DataLoadStream || !DataLoadStream.is_open()) {
		UE_LOG(HexGrid, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTrianglesLoopData.Rate, false);
		return;
	}

	LoadTriangles(DataLoadStream);
}

void AHexGrid::LoadTriangles(std::ifstream& ifs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	std::string line;
	while (std::getline(ifs, line))
	{
		FString fline(line.c_str());
		ParseTrianglesLine(fline);
		Count++;

		FlowControlUtility::SaveLoopData(this, LoadTrianglesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
	}

	ifs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::SetTilesPosZ;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTrianglesLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load triangles done!"));
}

void AHexGrid::ParseTrianglesLine(const FString& line)
{
	TArray<FString> StrArr;
	line.ParseIntoArray(StrArr, *CommaDelim, true);
	for (int32 i = 0; i < StrArr.Num(); i++)
	{
		int32 Value;
		ParseInt(StrArr[i], Value);
		GridTriangles.Add(Value);
	}
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

void AHexGrid::SetTilesPosZ()
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = SetTilesPosZLoopData.IndexSaved[0];
	for ( ; i < Tiles.Num(); i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, SetTilesPosZLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}

		SetTilePosZ(Tiles[i]);
		Count++;
	}
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::SetVerticesPosZ;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SetTilesPosZLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Set tiles pos z done!"));

}

void AHexGrid::SetTilePosZ(FStructHexTileData& Data)
{
	Data.PositionZ = Terrain->GetAltitudeByPos2D(Data.Position2D, this);
}

void AHexGrid::SetVerticesPosZ()
{
	FTimerHandle TimerHandle;
	if (!bFitAltitude) {
		UKismetSystemLibrary::PrintString(this, TEXT("Do not fit altitude!"), false, true);
		WorkflowState = Enum_HexGridWorkflowState::SetVertexColors;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SetVerticesPosZLoopData.Rate, false);
		return;
	}

	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = SetVerticesPosZLoopData.IndexSaved[0];
	for ( ; i < GridVertices.Num(); i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, SetVerticesPosZLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}

		SetVertexPosZ(GridVertices[i]);
		Count++;
	}
	WorkflowState = Enum_HexGridWorkflowState::SetTerrainLowBlockLevel;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SetVerticesPosZLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Set vertices pos z done!"));

}

void AHexGrid::SetVertexPosZ(FVector& Vertex)
{
	Vertex.Z = Terrain->GetAltitudeByPos2D(FVector2D(Vertex), this) + AltitudeOffset;
}

void AHexGrid::SetTerrainLowBlockLevel()
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = SetTerrainLowBlockLevelLoopData.IndexSaved[0];
	for (; i < Tiles.Num(); i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, SetTerrainLowBlockLevelLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		SetTileLowBlockLevel(Tiles[i]);
		Count++;
	}
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::SetVertexColors;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SetTerrainLowBlockLevelLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Set terrain low block level done!"));

}

void AHexGrid::SetTileLowBlockLevel(FStructHexTileData& Data)
{
	if (Data.PositionZ > TerrainLowBlockPosZ) {
		Data.TerrainLowBlockLevel = 0;
		return;
	}

	for (int32 i = 0; i < Data.Neighbors.Num(); i++)
	{
		SetLowBlockLevelByNeighbors(Data, i);
		if (Data.TerrainLowBlockLevel != -1) {
			return;
		}
	}
}

void AHexGrid::SetLowBlockLevelByNeighbors(FStructHexTileData& Data, int32 Index)
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
		if (Tiles[TileIndex].PositionZ > TerrainLowBlockPosZ) {
			Data.TerrainLowBlockLevel = Neighbors.Radius;
			return;
		}
	}
	Data.TerrainLowBlockLevel = -1;
}

void AHexGrid::SetVertexColors()
{
	FTimerHandle TimerHandle;
	if (!bUseBlockVextexColors) {
		UE_LOG(HexGrid, Log, TEXT("Do not use vertex colors!"));
		WorkflowState = Enum_HexGridWorkflowState::DrawMesh;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SetVertexColorsLoopData.Rate, false);
		return;
	}
	
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = SetVertexColorsLoopData.IndexSaved[0];
	for (; i < GridVertices.Num(); i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, SetVertexColorsLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}

		SetVertexColorByBlockMode(i);
		Count++;
	}
	WorkflowState = Enum_HexGridWorkflowState::DrawMesh;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SetVertexColorsLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Set vertex colors done!"));
}

void AHexGrid::SetVertexColor(const FLinearColor& Color)
{
	GridVertexColors.Add(FLinearColor(Color.R, Color.G, Color.B, Color.A));
}

void AHexGrid::SetVertexColorByBlockMode(int32 Index)
{
	if (VertexColorsShowMode == Enum_BlockMode::LowBlock) {
		SetVertexColorByLowBlock(Index);
	}
	else {
		SetVertexColor(LineVertexColor);
	}
}

void AHexGrid::SetVertexColorByLowBlock(int32 Index)
{
	int32 TilesIndex = Index / 12;
	int32 Level = Tiles[TilesIndex].TerrainLowBlockLevel;

	int32 LevelMin = 0;
	int32 LevelMax = NeighborRange + 1;
	if (Level == -1) {
		Level = LevelMax;
	}

	float H = 120.0f / float(LevelMax - LevelMin + 1) * float(Level);
	FLinearColor LinearColor = UKismetMathLibrary::HSVToRGB(H, 1.0, 1.0, 1.0);
	GridVertexColors.Add(LinearColor);
}

void AHexGrid::CreateHexGridLineMesh()
{
	if (!bShowGrid) {
		FTimerHandle TimerHandle;
		WorkflowState = Enum_HexGridWorkflowState::Done;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		UE_LOG(HexGrid, Log, TEXT("Don't show grid!"));
		return;
	}
	
	if (bUseBlockVextexColors) {
		HexGridMesh->CreateMeshSection_LinearColor(0, GridVertices, GridTriangles, TArray<FVector>(), TArray<FVector2D>(), GridVertexColors, TArray<FProcMeshTangent>(), false);
	}
	else {
		HexGridMesh->CreateMeshSection(0, GridVertices, GridTriangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	}
	
}

void AHexGrid::SetHexGridLineMaterial()
{
	if (bUseBlockVextexColors) {
		HexGridMesh->SetMaterial(0, HexGridLineVCMaterialIns);
	}
	else {
		HexGridMesh->SetMaterial(0, HexGridLineMaterialIns);
	}
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
		MouseOverVertices.Add(GridVertices[Index * 12 + i * 2]);
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