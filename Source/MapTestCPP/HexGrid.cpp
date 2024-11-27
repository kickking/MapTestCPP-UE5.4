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
#include <EnhancedInputComponent.h>
#include <EnhancedInputSubsystems.h>
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
	HexInstMesh->SetMobility(EComponentMobility::Static);
	HexInstMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MouseOverInstMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MouseOverInstMesh"));
	MouseOverInstMesh->SetupAttachment(RootComponent);
	MouseOverInstMesh->SetMobility(EComponentMobility::Static);
	MouseOverInstMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BindDelegate();
}

// Called when the game starts or when spawned
void AHexGrid::BeginPlay()
{
	Super::BeginPlay();
	EnablePlayer();
	AddInputMappingContext();
	BindEnchancedInputAction();

	WorkflowState = Enum_HexGridWorkflowState::InitWorkflow;
	CreateHexGridFlow();
	StartCheckMouseOver();
}

void AHexGrid::BindDelegate()
{
	WorkflowDelegate.BindUFunction(Cast<UObject>(this), TEXT("CreateHexGridFlow"));
	CheckMouseOverDelegate.BindUFunction(Cast<UObject>(this), TEXT("CheckMouseOver"));
}

void AHexGrid::EnablePlayer()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController) {
		EnableInput(PlayerController);
		Controller = PlayerController;
	}
}

void AHexGrid::AddInputMappingContext()
{
	if (Controller) {
		if (ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer()) {
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
				if (InputMapping != nullptr) {
					InputSystem->AddMappingContext(InputMapping, 0);
				}
				else {
					UE_LOG(HexGrid, Error, TEXT("No setting Input Mapping Context."));
				}
			}
		}
	}
}

void AHexGrid::BindEnchancedInputAction()
{
	if (InputComponent) {
		if (IncMouseOverRadiusAction != nullptr 
			&& DecMouseOverRadiusAction != nullptr) {
			UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent);
			EnhancedInputComp->BindAction(IncMouseOverRadiusAction, ETriggerEvent::Triggered, this,
				&AHexGrid::OnIncMouseOverRadius);
			EnhancedInputComp->BindAction(DecMouseOverRadiusAction, ETriggerEvent::Triggered, this,
				&AHexGrid::OnDecMouseOverRadius);
		}
		else {
			UE_LOG(HexGrid, Error, TEXT("No setting Input Action."));
		}
	}
}

void AHexGrid::OnIncMouseOverRadius()
{
	if (IsWorkFlowDone()) {
		int32 max = NeighborRange;
		int value = MouseOverShowRadius + 1;
		value = value < max ? value : max;
		MouseOverShowRadius = value;
	}
}

void AHexGrid::OnDecMouseOverRadius()
{
	if (IsWorkFlowDone()) {
		int32 min = 0;
		int value = MouseOverShowRadius - 1;
		value = value > min ? value : min;
		MouseOverShowRadius = value;
	}
}

void AHexGrid::StartCheckMouseOver()
{
	GetWorldTimerManager().SetTimer(CheckTimerHandle, CheckMouseOverDelegate, CheckTimerRate, true);
	UE_LOG(HexGrid, Log, TEXT("Start checking mouse over hex grid!"));
}

void AHexGrid::StopCheckMouseOver()
{
	GetWorldTimerManager().ClearTimer(CheckTimerHandle);
	UE_LOG(HexGrid, Log, TEXT("Stop checking mouse over hex grid!"));
}

void AHexGrid::CheckMouseOver()
{
	if (IsWorkFlowDone()) {
		FVector MousePos = Terrain->GetMousePosition();
		MouseOverGrid(FVector2D(MousePos.X, MousePos.Y));
	}
}

void AHexGrid::CreateHexGridFlow()
{
	switch (WorkflowState)
	{
	case Enum_HexGridWorkflowState::InitWorkflow:
		InitWorkflow();
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
	case Enum_HexGridWorkflowState::LoadNeighbors:
		LoadNeighborsFromFile();
		break;
	case Enum_HexGridWorkflowState::CreateTilesVertices:
		CreateTilesVertices();
		break;
	case Enum_HexGridWorkflowState::WaitTerrain:
		WaitTerrain();
		break;
	case Enum_HexGridWorkflowState::SetTilesPosZ:
		SetTilesPosZ();
		break;
	case Enum_HexGridWorkflowState::CalTilesNormal:
		CalTilesNormal();
		break;
	case Enum_HexGridWorkflowState::SetTilesWalkingBlockLevel:
		SetTilesWalkingBlockLevel();
		break;
	case Enum_HexGridWorkflowState::SetTilesWalkingBlockLevelEx:
		SetTilesWalkingBlockLevelEx();
		break;
	case Enum_HexGridWorkflowState::InitCheckTerrainWalkingConnection:
	case Enum_HexGridWorkflowState::BreakMaxWalkingBlockTilesToChunk:
	case Enum_HexGridWorkflowState::CheckChunksWalkingConnection:
		CheckTerrainWalkingConnection();
		break;
	case Enum_HexGridWorkflowState::FindTilesIsland:
		FindTilesIsland();
		break;
	case Enum_HexGridWorkflowState::SetTilesBuildingBlockLevel:
		SetTilesBuildingBlockLevel();
		break;
	case Enum_HexGridWorkflowState::SetTilesBuildingBlockLevelEx:
		SetTilesBuildingBlockLevelEx();
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
	WorkflowState = Enum_HexGridWorkflowState::LoadParams;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGrid, Log, TEXT("Init workflow done!"));
}

void AHexGrid::InitLoopData()
{
	FlowControlUtility::InitLoopData(LoadTileIndicesLoopData);
	FlowControlUtility::InitLoopData(LoadTilesLoopData);
	FlowControlUtility::InitLoopData(LoadNeighborsLoopData);
	LoadNeighborsLoopData.IndexSaved[0] = 1;
	FlowControlUtility::InitLoopData(CreateTilesVerticesLoopData);

	FlowControlUtility::InitLoopData(SetTilesPosZLoopData);
	FlowControlUtility::InitLoopData(CalTilesNormalLoopData);
	FlowControlUtility::InitLoopData(SetTilesWalkingBlockLevelLoopData);
	FlowControlUtility::InitLoopData(SetTilesWalkingBlockLevelExLoopData);
	FlowControlUtility::InitLoopData(BreakMaxWalkingBlockTilesToChunkLoopData);
	FlowControlUtility::InitLoopData(FindTilesIslandLoopData);
	FlowControlUtility::InitLoopData(SetTilesBuildingBlockLevelLoopData);
	FlowControlUtility::InitLoopData(SetTilesBuildingBlockLevelExLoopData);

	FlowControlUtility::InitLoopData(AddTilesInstanceLoopData);
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

	if (!LoadTileIndicesLoopData.HasInitialized) {
		LoadTileIndicesLoopData.HasInitialized = true;
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

	if (!LoadTilesLoopData.HasInitialized) {
		LoadTilesLoopData.HasInitialized = true;
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
	WorkflowState = Enum_HexGridWorkflowState::LoadNeighbors;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadTilesLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load tiles done!"));
}

void AHexGrid::ParseTileLine(const FString& line, FStructHexTileData& Data)
{
	TArray<FString> StrArr;
	line.ParseIntoArray(StrArr, *PipeDelim, true);
	ParseAxialCoord(StrArr[0], Data);
	ParsePosition2D(StrArr[1], Data);
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

void AHexGrid::LoadNeighborsFromFile()
{
	int32 i = LoadNeighborsLoopData.IndexSaved[0];
	FTimerHandle TimerHandle;

	for (; i <= NeighborRange; i++)
	{
		FString NeighborPath;
		FString FullPath;
		CreateNeighborPath(NeighborPath, i);

		if (!GetValidFilePath(NeighborPath, FullPath)) {
			UE_LOG(HexGrid, Warning, TEXT("Neighbor path file %s not exist!"), *NeighborPath);
			WorkflowState = Enum_HexGridWorkflowState::Error;
			GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadNeighborsLoopData.Rate, false);
			return;
		}

		if (!LoadNeighborsLoopData.HasInitialized) {
			LoadNeighborsLoopData.HasInitialized = true;
			DataLoadStream.open(*FullPath, std::ios::in);
		}

		if (!DataLoadStream || !DataLoadStream.is_open()) {
			UE_LOG(HexGrid, Warning, TEXT("Open file %s failed!"), *FullPath);
			WorkflowState = Enum_HexGridWorkflowState::Error;
			GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadNeighborsLoopData.Rate, false);
			return;
		}
		if (!LoadNeighbors(DataLoadStream, i)) {
			return;
		}
	}

	WorkflowState = Enum_HexGridWorkflowState::CreateTilesVertices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoadNeighborsLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("Load neighbors done!"));
}

void AHexGrid::CreateNeighborPath(FString& NeighborPath, int32 Radius)
{
	NeighborPath.Append(NeighborsDataPathPrefix).Append(FString::FromInt(Radius)).Append(FString(TEXT(".data")));
}

bool AHexGrid::LoadNeighbors(std::ifstream& ifs, int32 Radius)
{
	int32 Count = 0;
	TArray<int32> Indices = { Radius, 0 };
	bool SaveLoopFlag = false;

	int32 i = LoadNeighborsLoopData.IndexSaved[1];
	std::string line;
	while (std::getline(ifs, line))
	{
		FString fline(line.c_str());
		ParseNeighborsLine(fline, i, Radius);
		i++;
		Count++;
		Indices[1] = i;
		FlowControlUtility::SaveLoopData(this, LoadNeighborsLoopData, Count * Radius, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return false;
		}
	}

	ifs.close();
	FlowControlUtility::InitLoopData(LoadNeighborsLoopData);
	LoadNeighborsLoopData.IndexSaved[0] = Radius;
	UE_LOG(HexGrid, Log, TEXT("Load neighbor N%d done!"), Radius);
	return true;
}

void AHexGrid::ParseNeighborsLine(const FString& Str, int32 Index, int32 Radius)
{
	ParseNeighbors(Str, Index, Radius);
}

void AHexGrid::ParseNeighbors(const FString& Str, int32 Index, int32 Radius)
{
	FStructHexTileNeighbors Neighbors;
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
	Tiles[Index].Neighbors.Add(Neighbors);
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

bool AHexGrid::TilesLoopFunction(TFunction<void()> InitFunc, TFunction<void(int32 LoopIndex)> LoopFunc,
	FStructLoopData& LoopData, Enum_HexGridWorkflowState State)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	if (InitFunc && !LoopData.HasInitialized) {
		LoopData.HasInitialized = true;
		InitFunc();
	}

	int32 i = LoopData.IndexSaved[0];
	for (; i < Tiles.Num(); i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, LoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return false;
		}
		LoopFunc(i);
		Count++;
	}

	FTimerHandle TimerHandle;
	WorkflowState = State;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, LoopData.Rate, false);
	return true;
}

void AHexGrid::CreateTilesVertices()
{
	if (TilesLoopFunction([this]() { InitTileVerticesVertors(); }, [this](int32 i) { CreateTileVertices(i); },
		CreateTilesVerticesLoopData, Enum_HexGridWorkflowState::WaitTerrain)) {
		UE_LOG(HexGrid, Log, TEXT("Parse tiles vertices done!"));
	}
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

void AHexGrid::WaitTerrain()
{
	FTimerHandle TimerHandle;
	TArray<AActor*> Out_Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATerrain::StaticClass(), Out_Actors);
	if (Out_Actors.Num() == 1) {
		Terrain = (ATerrain*)Out_Actors[0];
		if (Terrain->IsWorkFlowStepDone(Enum_TerrainWorkflowState::InitWorkflow)) {
			WorkflowState = Enum_HexGridWorkflowState::SetTilesPosZ;
			GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
			UE_LOG(HexGrid, Log, TEXT("Wait terrain noise done!"));
			return;
		}
	}
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	return;
}

void AHexGrid::SetTilesPosZ()
{
	if (TilesLoopFunction(nullptr, [this](int32 i) { SetTilePosZ(i); },
		SetTilesPosZLoopData, Enum_HexGridWorkflowState::CalTilesNormal)) {
		UE_LOG(HexGrid, Log, TEXT("Set tiles pos z done!"));
	}
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
	if (TilesLoopFunction([this]() { InitCalTilesNormal(); }, [this](int32 i) { CalTileNormal(i); },
		CalTilesNormalLoopData, Enum_HexGridWorkflowState::SetTilesWalkingBlockLevel)) {
		UE_LOG(HexGrid, Log, TEXT("Calculate tiles normal done!"));
	}
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

void AHexGrid::SetTilesWalkingBlockLevel()
{
	if (TilesLoopFunction([this]() { InitSetTilesWalkingBlockLevel(); }, [this](int32 i) { SetTileWalkingBlockLevelByNeighbors(i); },
		SetTilesWalkingBlockLevelLoopData, Enum_HexGridWorkflowState::SetTilesWalkingBlockLevelEx)) {
		UE_LOG(HexGrid, Log, TEXT("Set tiles walking block level done!"));
	}
}

void AHexGrid::InitSetTilesWalkingBlockLevel()
{
	WalkingBlockLevelMax = NeighborRange + 1;
}

bool AHexGrid::SetTileWalkingBlock(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 BlockLevel)
{
	if (!IsInMapRange(CheckData) 
		|| CheckData.AvgPositionZ > WalkingBlockAltitudeRatio * Terrain->GetTileAltitudeMultiplier()
		|| CheckData.AvgPositionZ < Terrain->GetWaterBase()
		|| CheckData.AngleToUp > (PI * WalkingBlockSlopeRatio / 2.0)) {
		Data.TerrainWalkingBlockLevel = BlockLevel;
		return true;
	}
	return false;
}

void AHexGrid::SetTileWalkingBlockLevelByNeighbors(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (SetTileWalkingBlock(Data, Data, 0)) {
		return;
	}

	for (int32 i = 0; i < Data.Neighbors.Num(); i++)
	{
		if (SetTileWalkingBlockLevelByNeighbor(Data, i)) {
			return;
		}
	}
	Data.TerrainWalkingBlockLevel = WalkingBlockLevelMax;
}

bool AHexGrid::SetTileWalkingBlockLevelByNeighbor(FStructHexTileData& Data, int32 Index)
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
		if (SetTileWalkingBlock(Data, Tiles[TileIndex], Neighbors.Radius))
		{
			return true;
		}
	}
	return false;
}

void AHexGrid::SetTilesWalkingBlockLevelEx()
{
	if (TilesLoopFunction([this]() { InitSetTilesWalkingBlockLevelEx(); }, [this](int32 i) { SetTileWalkingBlockLevelByNeighborsEx(i); },
		SetTilesWalkingBlockLevelExLoopData, Enum_HexGridWorkflowState::InitCheckTerrainWalkingConnection)) {
		UE_LOG(HexGrid, Log, TEXT("Set tiles walking block level extension done!"));
	}
}

void AHexGrid::InitSetTilesWalkingBlockLevelEx()
{
	WalkingBlockLevelMax = NeighborRange * 2 + 1;
}

void AHexGrid::SetTileWalkingBlockLevelByNeighborsEx(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];

	if (Data.TerrainWalkingBlockLevel == (NeighborRange + 1)) {
		FStructHexTileNeighbors OutSideNeighbors = Data.Neighbors[Data.Neighbors.Num() - 1];
		int32 BlockLvMin = WalkingBlockLevelMax;
		int32 CurrentBlockLv = Data.TerrainWalkingBlockLevel;
		for (int32 i = 0; i < OutSideNeighbors.Count; i++)
		{
			CurrentBlockLv = NeighborRange + Tiles[TileIndices[OutSideNeighbors.Tiles[i]]].TerrainWalkingBlockLevel;
			if (CurrentBlockLv < BlockLvMin) {
				BlockLvMin = CurrentBlockLv;
			}
		}
		Data.TerrainWalkingBlockLevel = BlockLvMin;
		if (Data.TerrainWalkingBlockLevel == WalkingBlockLevelMax) {
			MaxWalkingBlockTileIndices.Add(Index);
		}
	}
}

void AHexGrid::CheckTerrainWalkingConnection()
{
	CheckTerrainWalkingConnectionWorkflow();
}

void AHexGrid::CheckTerrainWalkingConnectionWorkflow()
{
	switch (WorkflowState)
	{
	case Enum_HexGridWorkflowState::InitCheckTerrainWalkingConnection:
		InitCheckTerrainWalkingConnection();
		WorkflowState = Enum_HexGridWorkflowState::BreakMaxWalkingBlockTilesToChunk;
	case Enum_HexGridWorkflowState::BreakMaxWalkingBlockTilesToChunk:
		BreakMaxWalkingBlockTilesToChunk();
		break;
	case Enum_HexGridWorkflowState::CheckChunksWalkingConnection:
		CheckChunksWalkingConnection();
		break;
	default:
		break;
	}
}

void AHexGrid::InitCheckTerrainWalkingConnection()
{
	MaxWalkingBlockTileChunks.Empty();
}

void AHexGrid::BreakMaxWalkingBlockTilesToChunk()
{
	int32 Count = 0;
	TArray<int32> Indices = {};
	bool SaveLoopFlag = false;

	while (!MaxWalkingBlockTileIndices.IsEmpty()) 
	{
		if (CheckWalkingConnectionFrontier.IsEmpty()) {
			CheckWalkingConnectionReached.Empty();
			TArray<int32> arr = MaxWalkingBlockTileIndices.Array();
			CheckWalkingConnectionFrontier.Enqueue(arr[0]);
			MaxWalkingBlockTileIndices.Remove(arr[0]);

			CheckWalkingConnectionReached.Add(arr[0]);
		}
		
		while (!CheckWalkingConnectionFrontier.IsEmpty())
		{
			FlowControlUtility::SaveLoopData(this, BreakMaxWalkingBlockTilesToChunkLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
			if (SaveLoopFlag) {
				return;
			}

			int32 current;
			CheckWalkingConnectionFrontier.Dequeue(current);

			FStructHexTileNeighbors Neighbors = Tiles[current].Neighbors[0];
			for (int32 i = 0; i < Neighbors.Tiles.Num(); i++) {
				FIntPoint key = Neighbors.Tiles[i];
				if (!TileIndices.Contains(key)) {
					continue;
				}
				if (!CheckWalkingConnectionReached.Contains(TileIndices[key])
					&& MaxWalkingBlockTileIndices.Contains(TileIndices[key])) {
					CheckWalkingConnectionFrontier.Enqueue(TileIndices[key]);
					CheckWalkingConnectionReached.Add(TileIndices[key]);
					MaxWalkingBlockTileIndices.Remove(TileIndices[key]);
				}
			}

			Count++;
		}
		MaxWalkingBlockTileChunks.Add(CheckWalkingConnectionReached);
	}

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::CheckChunksWalkingConnection;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, BreakMaxWalkingBlockTilesToChunkLoopData.Rate, false);
	UE_LOG(HexGrid, Log, TEXT("MaxWalkingBlockTileChunks Num=%d"), MaxWalkingBlockTileChunks.Num());
	UE_LOG(HexGrid, Log, TEXT("Break Max Walking Block Tiles To Chunk done!"));
}

void AHexGrid::CheckChunksWalkingConnection()
{
	FTimerHandle TimerHandle;

	MaxWalkingBlockTileChunks.Sort([](const TSet<int32>& A, const TSet<int32>& B) {
		return A.Num() > B.Num();
	});
	if (MaxWalkingBlockTileChunks.IsEmpty()) {
		UE_LOG(HexGrid, Warning, TEXT("MaxWalkingBlockTileChunks is empty!"));
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		return;
	}
	TSet<int32> objChunk = MaxWalkingBlockTileChunks[0];
	
	for (int32 i = 1; i < MaxWalkingBlockTileChunks.Num(); i++) {
		if (!FindTwoChunksWalkingConnection(MaxWalkingBlockTileChunks[i], objChunk)) {
			CheckTerrainWalkingConnectionNotPass(i);
			continue;
		}
		objChunk.Append(MaxWalkingBlockTileChunks[i]);
	}
	
	WorkflowState = Enum_HexGridWorkflowState::FindTilesIsland;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGrid, Log, TEXT("Check Chunks Walking Connection done!"));
	UE_LOG(HexGrid, Log, TEXT("Check Terrain Walking Connection pass!"));
}

bool AHexGrid::FindTwoChunksWalkingConnection(TSet<int32>& ChunkStart, TSet<int32>& ChunkObj)
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
				&& Tiles[TileIndices[key]].TerrainWalkingBlockLevel >= 3) {
				frontier.Enqueue(TileIndices[key]);
				reached.Add(TileIndices[key]);
			}
		}
	}
	return false;
}

void AHexGrid::CheckTerrainWalkingConnectionNotPass(int32 ChunkIndex)
{
	TSet<int32> Cks = MaxWalkingBlockTileChunks[ChunkIndex];
	TArray<int32> CksArray = Cks.Array();
	for (int32 i : CksArray) {
		Tiles[i].TerrainWalkingConnection = false;
	}
}

void AHexGrid::FindTilesIsland()
{
	if (TilesLoopFunction(nullptr, [this](int32 i) { FindTileIsLand(i); },
		FindTilesIslandLoopData, Enum_HexGridWorkflowState::SetTilesBuildingBlockLevel)) {
		UE_LOG(HexGrid, Log, TEXT("Find tiles island done!"));
	}
}

void AHexGrid::FindTileIsLand(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (Data.TerrainWalkingBlockLevel == WalkingBlockLevelMax) {
		if (Data.TerrainWalkingConnection) {
			Data.TerrainIsLand = false;
		}
		else {
			Data.TerrainIsLand = true;
		}
		
	}
	else if (Data.TerrainWalkingBlockLevel < WalkingBlockLevelMax && Data.TerrainWalkingBlockLevel >= 3) {
		Data.TerrainIsLand = !Find_LBLM_By_LBL3(Index);
	}
	else if (Data.TerrainWalkingBlockLevel < 3 && Data.TerrainWalkingBlockLevel >= 1) {
		for (int32 i = Data.TerrainWalkingBlockLevel; i > 0; i--) {
			FStructHexTileNeighbors Neighbors = Data.Neighbors[2 - i];
			for (int32 j = 0; j < Neighbors.Tiles.Num(); j++) {
				FIntPoint key = Neighbors.Tiles[j];
				if (!TileIndices.Contains(key)) {
					continue;
				}
				if (Tiles[TileIndices[key]].TerrainWalkingBlockLevel == 3) {
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

/*Find WalkingBlockLevelMax tile by WalkingBlockLevel=3 tile*/
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
		if (Tiles[current].TerrainWalkingBlockLevel == WalkingBlockLevelMax) {
			if (Tiles[current].TerrainWalkingConnection) {
				return true;
			}
			else {
				return false;
			}
			
		}

		FStructHexTileNeighbors Neighbors = Tiles[current].Neighbors[0];
		for (int32 i = 0; i < Neighbors.Tiles.Num(); i++) {
			FIntPoint key = Neighbors.Tiles[i];
			if (!TileIndices.Contains(key)) {
				continue;
			}
			if (!reached.Contains(TileIndices[key]) && Tiles[TileIndices[key]].TerrainWalkingBlockLevel >=3) {
				frontier.Enqueue(TileIndices[key]);
				reached.Add(TileIndices[key]);
			}
		}
	}
	return false;
}

void AHexGrid::SetTilesBuildingBlockLevel()
{
	if (TilesLoopFunction([this]() { InitSetTilesBuildingBlockLevel(); }, [this](int32 i) { SetTileBuildingBlockLevelByNeighbors(i); },
		SetTilesBuildingBlockLevelLoopData, Enum_HexGridWorkflowState::SetTilesBuildingBlockLevelEx)) {
		UE_LOG(HexGrid, Log, TEXT("Set tiles Building Block level done!"));
	}
}

void AHexGrid::InitSetTilesBuildingBlockLevel()
{
	BuildingBlockLevelMax = NeighborRange + 1;
	BuildingBlockAltitudeRatio = BuildingBlockAltitudeRatio > WalkingBlockAltitudeRatio ? WalkingBlockAltitudeRatio : BuildingBlockAltitudeRatio;
	BuildingBlockSlopeRatio = BuildingBlockSlopeRatio > WalkingBlockSlopeRatio ? WalkingBlockSlopeRatio : BuildingBlockSlopeRatio;
}

bool AHexGrid::SetTileBuildingBlock(FStructHexTileData& Data, FStructHexTileData& CheckData, int32 BuildingBlockLevel)
{
	if (!IsInMapRange(CheckData) 
		|| CheckData.AvgPositionZ > BuildingBlockAltitudeRatio * Terrain->GetTileAltitudeMultiplier()
		|| CheckData.AvgPositionZ < Terrain->GetWaterBase()
		|| CheckData.AngleToUp >(PI * BuildingBlockSlopeRatio / 2.0)) {
		Data.TerrainBuildingBlockLevel = BuildingBlockLevel;
		return true;
	}
	return false;
}

void AHexGrid::SetTileBuildingBlockLevelByNeighbors(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (SetTileBuildingBlock(Data, Data, 0)) {
		return;
	}

	for (int32 i = 0; i < Data.Neighbors.Num(); i++)
	{
		if (SetTileBuildingBlockLevelByNeighbor(Data, i)) {
			return;
		}
	}
	Data.TerrainBuildingBlockLevel = BuildingBlockLevelMax;
}

bool AHexGrid::SetTileBuildingBlockLevelByNeighbor(FStructHexTileData& Data, int32 Index)
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
		if (SetTileBuildingBlock(Data, Tiles[TileIndex], Neighbors.Radius))
		{
			return true;
		}
	}

	return false;
}

void AHexGrid::SetTilesBuildingBlockLevelEx()
{
	if (TilesLoopFunction([this]() { InitSetTilesBuildingBlockExLevel(); }, [this](int32 i) { SetTileBuildingBlockLevelByNeighborsEx(i); },
		SetTilesBuildingBlockLevelExLoopData, Enum_HexGridWorkflowState::DrawMesh)) {
		UE_LOG(HexGrid, Log, TEXT("Set tiles Building Block level extension done!"));
	}
}

void AHexGrid::InitSetTilesBuildingBlockExLevel()
{
	BuildingBlockLevelMax = NeighborRange * 2 + 1;
}

void AHexGrid::SetTileBuildingBlockLevelByNeighborsEx(int32 Index)
{
	FStructHexTileData& Data = Tiles[Index];
	if (Data.TerrainBuildingBlockLevel == (NeighborRange + 1)) {
		FStructHexTileNeighbors OutSideNeighbors = Data.Neighbors[Data.Neighbors.Num() - 1];
		int32 BuildingBlockLvMin = BuildingBlockLevelMax;
		int32 CurrentBuildingBlockLv = Data.TerrainBuildingBlockLevel;
		for (int32 i = 0; i < OutSideNeighbors.Count; i++)
		{
			CurrentBuildingBlockLv = NeighborRange + Tiles[TileIndices[OutSideNeighbors.Tiles[i]]].TerrainBuildingBlockLevel;
			if (CurrentBuildingBlockLv < BuildingBlockLvMin) {
				BuildingBlockLvMin = CurrentBuildingBlockLv;
			}
		}
		Data.TerrainBuildingBlockLevel = BuildingBlockLvMin;
	}
}

void AHexGrid::AddTilesInstance()
{
	if (!bShowGrid) {
		InitAddTilesInstance();
		FTimerHandle TimerHandle;
		WorkflowState = Enum_HexGridWorkflowState::Done;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		UE_LOG(HexGrid, Log, TEXT("Don't show grid!"));
		return;
	}

	if (TilesLoopFunction([this]() { InitAddTilesInstance(); }, [this](int32 i) { AddTileInstanceByWalkingBlock(i); },
		AddTilesInstanceLoopData, Enum_HexGridWorkflowState::Done)) {
		UE_LOG(HexGrid, Log, TEXT("Add tiles instance done!"));
	}
}

void AHexGrid::InitAddTilesInstance()
{
	HexInstMesh->NumCustomDataFloats = 3;
	HexInstanceScale = TileSize / HexInstMeshSize;
}

int32 AHexGrid::AddTileInstance(int32 Index)
{
	return AddISM(Index, HexInstMesh, HexInstMeshOffsetZ);
}

int32 AHexGrid::AddISM(int32 Index, UInstancedStaticMeshComponent* ISM, float ZOffset)
{
	if (ISM == nullptr) {
		return -1;
	}

	FStructHexTileData tile = Tiles[Index];
	FVector HexLoc(tile.Position2D.X, tile.Position2D.Y, tile.AvgPositionZ + ZOffset);
	FVector HexScale(HexInstanceScale);

	FVector RotationAxis = FVector::CrossProduct(HexInstMeshUpVec, tile.Normal);
	RotationAxis.Normalize();

	FQuat Quat = FQuat(RotationAxis, tile.AngleToUp);
	FQuat NewQuat = Quat * HexInstMeshRot.Quaternion();

	FTransform HexTransform(NewQuat.Rotator(), HexLoc, HexScale);

	int32 InstanceIndex = ISM->AddInstance(HexTransform);
	return InstanceIndex;
}

void AHexGrid::AddTileInstanceByWalkingBlock(int32 Index)
{
	FStructHexTileData tile = Tiles[Index];
	if (tile.TerrainWalkingBlockLevel > 0 
		&& !tile.TerrainIsLand
		&& IsInMapRange(Index))
	{
		int32 InstanceIndex = AddTileInstance(Index);
		if (InstanceIndex >= 0) {
			AddTileInstanceDataByWalkingBlock(Index, InstanceIndex);
		}
	}
}

void AHexGrid::AddTileInstanceDataByWalkingBlock(int32 TileIndex, int32 InstanceIndex)
{
	float H = 0.0;
	if (GridShowMode == Enum_BlockMode::WalkingBlock) {
		if (!Tiles[TileIndex].TerrainIsLand) {
			H = 120.0f / float(WalkingBlockLevelMax) * float(Tiles[TileIndex].TerrainWalkingBlockLevel);
		}
		else {
			H = 240.0;
		}
	}
	else if (GridShowMode == Enum_BlockMode::BuildingBlock) {
		if (!Tiles[TileIndex].TerrainIsLand) {
			H = 120.0f / float(BuildingBlockLevelMax) * float(Tiles[TileIndex].TerrainBuildingBlockLevel);
		}
		else {
			H = 240.0;
		}
	}

	FLinearColor LinearColor = UKismetMathLibrary::HSVToRGB(H, 1.0, 1.0, 1.0);

	TArray<float> CustomData = { LinearColor.R, LinearColor.G, LinearColor.B };
	HexInstMesh->SetCustomData(InstanceIndex, CustomData, true);
}

bool AHexGrid::IsInMapRange(int32 Index)
{
	return IsInMapRange(Tiles[Index]);
}

bool AHexGrid::IsInMapRange(const FStructHexTileData& Tile)
{
	return (FMath::Abs<float>(Tile.Position2D.X) < Terrain->GetWidth() / 2
		&& FMath::Abs<float>(Tile.Position2D.Y) < Terrain->GetHeight() / 2);
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
	MouseOverHex.SetHex(hex);
	RemoveMouseOverTilesInstance();
	AddMouseOverTilesInstance();
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

void AHexGrid::AddMouseOverTilesInstance()
{
	int32 Index = TileIndices[MouseOverHex.ToIntPoint()];
	int32 InstanceIndex = AddISM(Index, MouseOverInstMesh, MouseOverInstMeshOffsetZ);

	MouseOverShowRadius = MouseOverShowRadius < NeighborRange ? MouseOverShowRadius : NeighborRange;
	for (int32 i = 0; i < MouseOverShowRadius; i++)
	{
		TArray<FIntPoint> RingTiles = Tiles[Index].Neighbors[i].Tiles;
		for (int32 j = 0; j < RingTiles.Num(); j++)
		{
			InstanceIndex = AddISM(TileIndices[RingTiles[j]], MouseOverInstMesh, MouseOverInstMeshOffsetZ);
		}
	}
}

void AHexGrid::RemoveMouseOverTilesInstance()
{
	MouseOverInstMesh->ClearInstances();
}

