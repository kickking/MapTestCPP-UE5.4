// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain.h"
#include "FlowControlUtility.h"
#include "HexGrid.h"

#include <FastNoiseWrapper.h>
#include <Kismet/GameplayStatics.h>
#include <Math/UnrealMathUtility.h>
#include <TimerManager.h>
#include <ProceduralMeshComponent.h>

DEFINE_LOG_CATEGORY(Terrain);

// Sets default values
ATerrain::ATerrain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	this->SetRootComponent(TerrainMesh);

	TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	BindDelegate();


}

// Called when the game starts or when spawned
void ATerrain::BeginPlay()
{
	Super::BeginPlay();

	WorkflowState = Enum_TerrainWorkflowState::InitWorkflow;
	CreateTerrainFlow();
}

// Called every frame
void ATerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATerrain::BindDelegate()
{
	WorkflowDelegate.BindUFunction(Cast<UObject>(this), TEXT("CreateTerrainFlow"));
}

void ATerrain::CreateNoise()
{
	NoiseWrapper = NewObject<UFastNoiseWrapper>(this);

	if (NoiseWrapper != nullptr) {
		NoiseWrapper->SetupFastNoise(EFastNoise_NoiseType::PerlinFractal, 
			NoiseSeed, 
			NoiseFrequency, 
			EFastNoise_Interp::Quintic, 
			EFastNoise_FractalType::RigidMulti, 
			Octaves, 
			Lacunarity, 
			Gain, 
			CellularJitter, 
			EFastNoise_CellularDistanceFunction::Euclidean, 
			EFastNoise_CellularReturnType::CellValue);

		CreateNoiseDone = true;
		UE_LOG(Terrain, Log, TEXT("Create and set NoiseWrapper."));
	}
}

bool ATerrain::isCreateNoiseDone()
{
	return CreateNoiseDone;
}

void ATerrain::InitTileParameter()
{
	TileSizeMultiplier = TileSize * TileScale;
	TileHeightMultiplier = TileHeight * TileScale;
}

void ATerrain::InitLoopData()
{
	FlowControlUtility::InitLoopData(CreateVerticesLoopData);
	FlowControlUtility::InitLoopData(CreateTrianglesLoopData);
	FlowControlUtility::InitLoopData(CalNormalsInitLoopData);
	FlowControlUtility::InitLoopData(CalNormalsAccLoopData);
	FlowControlUtility::InitLoopData(NormalizeNormalsLoopData);
}

void ATerrain::InitHexGrid()
{
	FTimerHandle TimerHandle;
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHexGrid::StaticClass(), OutActors);
	if (OutActors.Num() == 1) {
		HexGrid = (AHexGrid*)OutActors[0];
	}
	else {
		WorkflowState = Enum_TerrainWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		UE_LOG(Terrain, Warning, TEXT("More than one HexGrid!"));
		return;
	}
}

void ATerrain::InitWorkflow()
{
	CreateNoise();
	InitTileParameter();
	InitLoopData();
	InitHexGrid();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_TerrainWorkflowState::CreateVerticesAndUVs;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(Terrain, Log, TEXT("Init workflow done!"));

}

//bind to delegate
void ATerrain::CreateTerrainFlow()
{
	switch (WorkflowState)
	{
	case Enum_TerrainWorkflowState::InitWorkflow:
		InitWorkflow();
		break;
	case Enum_TerrainWorkflowState::CreateVerticesAndUVs:
		CreateVertices();
		break;
	case Enum_TerrainWorkflowState::CreateTriangles:
		CreateTriangles();
		break;
	case Enum_TerrainWorkflowState::CalNormalsInit:
	case Enum_TerrainWorkflowState::CalNormalsAcc:
	case Enum_TerrainWorkflowState::NormalizeNormals:
		CreateNormals();
		break;
	case Enum_TerrainWorkflowState::DrawMesh:
		CreateTerrainMesh();
		SetTerrainMaterial();
		WorkflowState = Enum_TerrainWorkflowState::Done;
		break;
	case Enum_TerrainWorkflowState::Done:
		break;
	case Enum_TerrainWorkflowState::Error:
		UE_LOG(Terrain, Warning, TEXT("CreateTerrainFlow Error!"));
		break;
	default:
		break;
	}
}

void ATerrain::ResetProgress()
{
	ProgressTarget = 0;
	ProgressCurrent = 0;
}

void ATerrain::GetProgress(float& Out_Progress)
{
	float Rate;
	if (ProgressTarget == 0.0) {
		Out_Progress = 0.0;
	}
	else {
		Rate = float(ProgressCurrent) / float(ProgressTarget);
		Rate = Rate > 1.0 ? 1.0 : Rate;
		Out_Progress = Rate;
	}
}

void ATerrain::CreateVertices()
{
	int32 HalfRow = NumRows * 0.5;
	int32 HalfColumn = NumColumns * 0.5;

	bool OnceLoopCol = true;
	int32 Count = 0;
	TArray<int32> Indices = {0, 0};
	bool SaveLoopFlag = false;

	if (!CreateVerticesLoopData.IsInitialized) {
		CreateVerticesLoopData.IsInitialized = true;
		ProgressTarget = (NumRows + 1) * (NumColumns + 1);
	}

	int32 i = CreateVerticesLoopData.IndexSaved[0];
	int32 j;
	for ( ; i <= NumRows; i++) {
		Indices[0] = i;
		float X = i - HalfRow;
		j = OnceLoopCol ? CreateVerticesLoopData.IndexSaved[1] : 0;
		for (; j <= NumColumns; j++) {
			Indices[1] = j;
			float Y = j - HalfColumn;
			FlowControlUtility::SaveLoopData(this, CreateVerticesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
			if (SaveLoopFlag) {
				return;
			}
			CreateVertex(X, Y);
			CreateUV(X, Y);
			ProgressCurrent = CreateVerticesLoopData.Count;
			Count++;
		}
		OnceLoopCol = false;
	}
	ResetProgress();

	WorkflowState = Enum_TerrainWorkflowState::CreateTriangles;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CreateVerticesLoopData.Rate, false);
	UE_LOG(Terrain, Log, TEXT("Create vertices and UVs done."));
}

float ATerrain::GetAltitude(UFastNoiseWrapper* NoiseWP, float X, float Y, float InHorizon, float Multiplier)
{
	float Value = 0.0;
	if (NoiseWP != nullptr) {
		Value = NoiseWP->GetNoise2D(X, Y);
		float H = FMath::Clamp<float>(InHorizon, 0.0, 1.0);
		Value = (Value + 1.0) * 0.5 - H;
		Value = FMath::Clamp<float>(Value, 0.0, 1.0) * Multiplier / (1.0 - H);

	}
	return Value;
}

float ATerrain::GetAltitudeByPos2D(const FVector2D Pos2D, AActor* Caller)
{
	float X = Pos2D.X / TileSizeMultiplier;
	float Y = Pos2D.Y / TileSizeMultiplier;
	float Z = GetAltitude(NoiseWrapper, X, Y, Horizon, TileHeightMultiplier);
	return Z;
}

void ATerrain::CreateVertex(float X, float Y)
{
	float VX = X * TileSizeMultiplier;
	float VY = Y * TileSizeMultiplier;
	float VZ = GetAltitude(NoiseWrapper, X, Y, Horizon, TileHeightMultiplier);
	Vertices.Add(FVector(VX, VY, VZ));
}

void ATerrain::CreateUV(float X, float Y)
{
	float UVx = X * UVScale;
	float UVy = Y * UVScale;
	UVs.Add(FVector2D(UVx, UVy));
}

void ATerrain::CreateTriangles()
{
	int32 ColumnVertexNum = NumColumns + 1;
	int32 RowMOne = NumRows - 1;
	int32 ColumnMOne = NumColumns - 1;

	int32 RowVertex;
	int32 RowPlusOneVertex;

	bool OnceLoopCol = true;
	int32 Count = 0;
	TArray<int32> Indices = { 0, 0 };
	bool SaveLoopFlag = false;

	if (!CreateTrianglesLoopData.IsInitialized) {
		CreateTrianglesLoopData.IsInitialized = true;
		ProgressTarget = NumRows * NumColumns;
	}

	int32 i = CreateTrianglesLoopData.IndexSaved[0];
	int32 j;

	for ( ; i <= RowMOne; i++) {
		Indices[0] = i;
		RowVertex = i * ColumnVertexNum;
		RowPlusOneVertex = (i + 1) * ColumnVertexNum;
		j = OnceLoopCol ? CreateTrianglesLoopData.IndexSaved[1] : 0;
		for (; j <= ColumnMOne; j++) {
			Indices[1] = j;
			FlowControlUtility::SaveLoopData(this, CreateTrianglesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
			if (SaveLoopFlag) {
				return;
			}
			CreatePairTriangles(j, RowVertex, RowPlusOneVertex);
			ProgressCurrent = CreateTrianglesLoopData.Count;
			Count++;
		}
		OnceLoopCol = false;
	}
	ResetProgress();

	WorkflowState = Enum_TerrainWorkflowState::CalNormalsInit;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CreateTrianglesLoopData.Rate, false);
	UE_LOG(Terrain, Log, TEXT("Create triangles done."));
}

void ATerrain::CreatePairTriangles(int32 ColumnIndex, int32 RowVertex, int32 RowPlusOneVertex)
{
	TArray<int32> VIndices = { 0, 0, 0, 0, 0, 0 };
	int32 VI0 = ColumnIndex + RowVertex;
	VIndices[0] = VI0;
	VIndices[3] = VI0;
	int32 VI1 = ColumnIndex + RowPlusOneVertex;
	VIndices[2] = VI1;
	int32 VI2 = (ColumnIndex + 1) + RowVertex;
	VIndices[4] = VI2;
	int32 VI3 = (ColumnIndex + 1) + RowPlusOneVertex;
	VIndices[1] = VI3;
	VIndices[5] = VI3;

	Triangles.Append(VIndices);

}

void ATerrain::CreateNormals()
{
	CalNormalsWorkflow();
}

void ATerrain::CalNormalsWorkflow()
{
	switch (WorkflowState)
	{
	case Enum_TerrainWorkflowState::CalNormalsInit:
		CalNormalsInit();
		break;
	case Enum_TerrainWorkflowState::CalNormalsAcc:
		CalNormalsAcc();
		break;
	case Enum_TerrainWorkflowState::NormalizeNormals:
		NormalizeNormals();
		break;
	default:
		break;
	}
}

void ATerrain::CalNormalsInit()
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	if (!CalNormalsInitLoopData.IsInitialized) {
		CalNormalsInitLoopData.IsInitialized = true;
		ProgressTarget = Vertices.Num();
	}

	int32 i = CalNormalsInitLoopData.IndexSaved[0];
	for ( ; i <= Vertices.Num() - 1; i++) {
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, CalNormalsInitLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		NormalsAcc.Add(FVector(0, 0, 0));
		ProgressCurrent = CalNormalsInitLoopData.Count;
		Count++;
	}
	ResetProgress();

	WorkflowState = Enum_TerrainWorkflowState::CalNormalsAcc;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CalNormalsInitLoopData.Rate, false);
	UE_LOG(Terrain, Log, TEXT("Calculate normals initialization done."));
}

void ATerrain::CalNormalsAcc()
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	if (!CalNormalsAccLoopData.IsInitialized) {
		CalNormalsAccLoopData.IsInitialized = true;
		ProgressTarget = Triangles.Num() / 3;
	}

	int32 i = CalNormalsAccLoopData.IndexSaved[0];
	int32 last = Triangles.Num() / 3 - 1;
	for ( ; i <= last; i++) {
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, CalNormalsAccLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		CalTriangleNormalForVertex(i);
		ProgressCurrent = CalNormalsAccLoopData.Count;
		Count++;
	}
	ResetProgress();

	WorkflowState = Enum_TerrainWorkflowState::NormalizeNormals;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CalNormalsAccLoopData.Rate, false);
	UE_LOG(Terrain, Log, TEXT("Calculate normals accumulation done."));
}

void ATerrain::CalTriangleNormalForVertex(int32 TriangleIndex)
{
	int32 Index3Times = TriangleIndex * 3;
	int32 Index1 = Triangles[Index3Times];
	int32 Index2 = Triangles[Index3Times + 1];
	int32 Index3 = Triangles[Index3Times + 2];

	FVector Normal = FVector::CrossProduct(Vertices[Index1] - Vertices[Index2], Vertices[Index3] - Vertices[Index2]);

	FVector N1 = NormalsAcc[Index1] + Normal;
	NormalsAcc[Index1] = N1;

	FVector N2 = NormalsAcc[Index2] + Normal;
	NormalsAcc[Index2] = N2;

	FVector N3 = NormalsAcc[Index3] + Normal;
	NormalsAcc[Index3] = N3;
}

void ATerrain::NormalizeNormals()
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	if (!NormalizeNormalsLoopData.IsInitialized) {
		NormalizeNormalsLoopData.IsInitialized = true;
		ProgressTarget = NormalsAcc.Num();
	}

	int32 i = NormalizeNormalsLoopData.IndexSaved[0];
	int32 last = NormalsAcc.Num() - 1;
	for ( ; i <= last; i++) {
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, NormalizeNormalsLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		NormalsAcc[i].Normalize();
		Normals.Add(NormalsAcc[i]);
		ProgressCurrent = NormalizeNormalsLoopData.Count;
		Count++;
	}
	ResetProgress();

	WorkflowState = Enum_TerrainWorkflowState::DrawMesh;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, NormalizeNormalsLoopData.Rate, false);
	UE_LOG(Terrain, Log, TEXT("Normalize normals done."));
}

void ATerrain::CreateTerrainMesh()
{
	TerrainMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true );
	TerrainMesh->bUseComplexAsSimpleCollision = true;
	TerrainMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	TerrainMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	TerrainMesh->bUseComplexAsSimpleCollision = false;
}

void ATerrain::SetTerrainMaterial()
{
	TerrainMesh->SetMaterial(0, TerrainMaterialIns);
}
