// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain.h"
#include "FlowControlUtility.h"
#include "HexGrid.h"

#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMaterialLibrary.h>
#include <Kismet/KismetMathLibrary.h>
#include <Math/UnrealMathUtility.h>
#include <TimerManager.h>
#include <ProceduralMeshComponent.h>
#include <EnhancedInputComponent.h>

DEFINE_LOG_CATEGORY(Terrain);

// Sets default values
ATerrain::ATerrain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	this->SetRootComponent(TerrainMesh);
	TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	WaterMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetupAttachment(TerrainMesh);
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterMesh->SetWorldLocation(FVector(0.0, 0.0, -1.0));

	BindDelegate();

}

// Called when the game starts or when spawned
void ATerrain::BeginPlay()
{
	Super::BeginPlay();
	EnablePlayer();
	BindEnchancedInputAction();

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

void ATerrain::EnablePlayer()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController) {
		EnableInput(PlayerController);
		Controller = PlayerController;
	}
}

void ATerrain::BindEnchancedInputAction()
{
	if (InputComponent) {
		if (MouseLeftHoldAction != nullptr && MouseRightHoldAction != nullptr) {
			UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent);
			EnhancedInputComp->BindAction(MouseLeftHoldAction, ETriggerEvent::Started, this,
				&ATerrain::OnLeftHoldStarted);
			EnhancedInputComp->BindAction(MouseLeftHoldAction, ETriggerEvent::Completed, this,
				&ATerrain::OnLeftHoldCompleted);
			EnhancedInputComp->BindAction(MouseRightHoldAction, ETriggerEvent::Started, this,
				&ATerrain::OnRightHoldStarted);
			EnhancedInputComp->BindAction(MouseRightHoldAction, ETriggerEvent::Completed, this,
				&ATerrain::OnRightHoldCompleted);
		}
		else {
			UE_LOG(Terrain, Error, TEXT("No setting Input Action."));
		}
	}
}

bool ATerrain::IsMouseClickTraceHit()
{
	FVector location, direction;
	Controller->DeprojectMousePositionToWorld(location, direction);

	FHitResult result;
	FCollisionQueryParams params;
	bool isHit = TerrainMesh->LineTraceComponent(result, location, HoldTraceLength * direction + location,
		params);
	return isHit;
}

void ATerrain::OnLeftHoldStarted(const FInputActionValue& Value)
{
	if (IsMouseClickTraceHit()) {
		LeftHold = true;
	}
}

void ATerrain::OnLeftHoldCompleted(const FInputActionValue& Value)
{
	if (IsMouseClickTraceHit()) {
		LeftHold = false;
	}
}

void ATerrain::OnRightHoldStarted(const FInputActionValue& Value)
{
	if (IsMouseClickTraceHit()) {
		RightHold = true;
	}
}

void ATerrain::OnRightHoldCompleted(const FInputActionValue& Value)
{
	if (IsMouseClickTraceHit()) {
		RightHold = false;
	}
}

bool ATerrain::IsLeftHold()
{
	return LeftHold;
}

bool ATerrain::IsRightHold()
{
	return RightHold;
}

void ATerrain::CreateNoise()
{
	NWHighMountain = NewObject<UFastNoiseWrapper>(this);
	NWLowMountain = NewObject<UFastNoiseWrapper>(this);
	NWWater = NewObject<UFastNoiseWrapper>(this);
	NWMoisture = NewObject<UFastNoiseWrapper>(this);
	NWTemperature = NewObject<UFastNoiseWrapper>(this);
	NWBiomes = NewObject<UFastNoiseWrapper>(this);
	NWTree = NewObject<UFastNoiseWrapper>(this);

	if (NWHighMountain != nullptr && NWLowMountain != nullptr && NWWater != nullptr && 
		NWMoisture != nullptr && NWTemperature != nullptr && NWBiomes!=nullptr && 
		NWTree != nullptr) {
		NWHighMountain->SetupFastNoise(NWHighMountain_NoiseType,
			NWHighMountain_NoiseSeed,
			NWHighMountain_NoiseFrequency,
			NWHighMountain_Interp,
			NWHighMountain_FractalType,
			NWHighMountain_Octaves,
			NWHighMountain_Lacunarity,
			NWHighMountain_Gain,
			NWHighMountain_CellularJitter,
			NWHighMountain_CDF,
			NWHighMountain_CRT);

		NWLowMountain->SetupFastNoise(NWLowMountain_NoiseType,
			NWLowMountain_NoiseSeed,
			NWLowMountain_NoiseFrequency,
			NWLowMountain_Interp,
			NWLowMountain_FractalType,
			NWLowMountain_Octaves,
			NWLowMountain_Lacunarity,
			NWLowMountain_Gain,
			NWLowMountain_CellularJitter,
			NWLowMountain_CDF,
			NWLowMountain_CRT);

		NWWater->SetupFastNoise(NWWater_NoiseType,
			NWWater_NoiseSeed,
			NWWater_NoiseFrequency,
			NWWater_Interp,
			NWWater_FractalType,
			NWWater_Octaves,
			NWWater_Lacunarity,
			NWWater_Gain,
			NWWater_CellularJitter,
			NWWater_CDF,
			NWWater_CRT);

		NWMoisture->SetupFastNoise(NWMoisture_NoiseType,
			NWMoisture_NoiseSeed,
			NWMoisture_NoiseFrequency,
			NWMoisture_Interp,
			NWMoisture_FractalType,
			NWMoisture_Octaves,
			NWMoisture_Lacunarity,
			NWMoisture_Gain,
			NWMoisture_CellularJitter,
			NWMoisture_CDF,
			NWMoisture_CRT);

		NWTemperature->SetupFastNoise(NWTemperature_NoiseType,
			NWTemperature_NoiseSeed,
			NWTemperature_NoiseFrequency,
			NWTemperature_Interp,
			NWTemperature_FractalType,
			NWTemperature_Octaves,
			NWTemperature_Lacunarity,
			NWTemperature_Gain,
			NWTemperature_CellularJitter,
			NWTemperature_CDF,
			NWTemperature_CRT);

		NWBiomes->SetupFastNoise(NWBiomes_NoiseType,
			NWBiomes_NoiseSeed,
			NWBiomes_NoiseFrequency,
			NWBiomes_Interp,
			NWBiomes_FractalType,
			NWBiomes_Octaves,
			NWBiomes_Lacunarity,
			NWBiomes_Gain,
			NWBiomes_CellularJitter,
			NWBiomes_CDF,
			NWBiomes_CRT);

		NWTree->SetupFastNoise(NWTree_NoiseType,
			NWTree_NoiseSeed,
			NWTree_NoiseFrequency,
			NWTree_Interp,
			NWTree_FractalType,
			NWTree_Octaves,
			NWTree_Lacunarity,
			NWTree_Gain,
			NWTree_CellularJitter,
			NWTree_CDF,
			NWTree_CRT);

		//CreateNoiseDone = true;
		UE_LOG(Terrain, Log, TEXT("Create and set Noise."));
	}
}

bool ATerrain::IsWorkFlowStepDone(Enum_TerrainWorkflowState state)
{
	return WorkflowState > state;
}

void ATerrain::InitTileParameter()
{
	TileSizeMultiplier = TileSize * TileScale;
	TileAltitudeMultiplier = TileAltitudeMax * TileScale;

	TerrainWidth = NumRows * TileSizeMultiplier;
	TerrainHeight = NumColumns * TileSizeMultiplier;

	TileNumRowRatio = (float)StdNumRows / (float)NumRows;
	TileNumColumnRatio = (float)StdNumColumns / (float)NumColumns;

}

void ATerrain::InitReceiveDecal()
{
	TerrainMesh->SetReceivesDecals(true);
	WaterMesh->SetReceivesDecals(false);
}

void ATerrain::InitTreeParam()
{
	TreeAreaScaleA = FMath::Pow(TreeAreaScale, 0.2);
	TreeAreaScaleA = TreeAreaScaleA > 0.0 ? TreeAreaScaleA : 1.0;
	OneMinTAS = 1.0 - TreeAreaScaleA;
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

void ATerrain::InitTerrainFormBaseRatio()
{
	if (HasWater) {
		UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("LavaBaseRatio"),
			LavaBaseRatio);
	}
	else {
		UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("LavaBaseRatio"),
			-2.0);
	}
	UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("DesertBaseRatio"),
		DesertBaseRatio);
	UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("SwampBaseRatio"),
		SwampBaseRatio);

}

void ATerrain::InitWater()
{
	SetWaterZ();
	WaterBase = WaterBaseRatio * TileAltitudeMultiplier - WaterMesh->GetComponentLocation().Z;
}

bool ATerrain::CheckMaterialSetting()
{
	bool ret = false;
	if (TerrainMaterialIns != nullptr && TerrainMPC != nullptr && WaterMaterialIns != nullptr && 
		CausticsMaterialIns != nullptr) {
		ret = true;
	}
	return ret;
}

void ATerrain::InitWorkflow()
{
	CreateNoise();
	InitTileParameter();
	InitReceiveDecal();
	InitLoopData();
	InitHexGrid();
	InitTerrainFormBaseRatio();
	InitWater();
	InitTreeParam();

	FTimerHandle TimerHandle;
	if (CheckMaterialSetting()) {
		WorkflowState = Enum_TerrainWorkflowState::CreateVerticesAndUVs;
		UE_LOG(Terrain, Log, TEXT("Init workflow done!"));
	}
	else {
		WorkflowState = Enum_TerrainWorkflowState::Error;
		UE_LOG(Terrain, Log, TEXT("CheckMaterialSetting() error!"));
	}
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
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
	case Enum_TerrainWorkflowState::DrawLandMesh:
		CreateTerrainMesh();
		SetTerrainMaterial();
		WorkflowState = Enum_TerrainWorkflowState::CreateWater;
	case Enum_TerrainWorkflowState::CreateWater:
		CreateWater();
		WorkflowState = Enum_TerrainWorkflowState::Done;
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

	float RatioStd;
	float Ratio;

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
			
			CreateVertex(X, Y, RatioStd, Ratio);
			CreateUV(X, Y);
			CreateVertexColorsForAMTA(RatioStd, X, Y);
			AddTreeValues(X, Y);

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

float ATerrain::GetAltitude(float X, float Y, float& OutRatioStd, float& OutRatio)
{
	OutRatio = GetHighMountainRatio(X, Y) + GetLowMountianRatio(X, Y);
	if (HasWater) {
		float wRatio = GetWaterRatio(X, Y);
		float alpha = 1 - wRatio / WaterBaseRatio;
		alpha = FMath::Clamp<float>(alpha, 0.0, 1.0);
		OutRatio = wRatio + FMath::Lerp<float>(wRatio, OutRatio, alpha);
	}
	OutRatioStd = OutRatio * 0.5 + 0.5;
	float z = OutRatio * TileAltitudeMultiplier;
	return z;
}

float ATerrain::MappingFromRangeToRange(float InputValue, const FStructHeightMapping& Mapping)
{
	float alpha = (Mapping.RangeMax - FMath::Clamp<float>(InputValue, Mapping.RangeMin, Mapping.RangeMax)) / (Mapping.RangeMax - Mapping.RangeMin);
	return FMath::Lerp<float>(Mapping.MappingMax, Mapping.MappingMin, alpha);
}

float ATerrain::GetHeightRatio(UFastNoiseWrapper* NWP, const FStructHeightMapping& Mapping, float X, float Y)
{
	float value = 0.0;
	if (NWP != nullptr) {
		value = NWP->GetNoise2D(X * TileNumRowRatio, Y * TileNumColumnRatio);
		return MappingFromRangeToRange(value, Mapping);
	}
	return 0.0f;
}

void ATerrain::MappingByLevel(float level, const FStructHeightMapping& InMapping, 
	FStructHeightMapping& OutMapping)
{
	OutMapping.RangeMin = InMapping.RangeMin + InMapping.RangeMinOffset * level;
	OutMapping.RangeMax = InMapping.RangeMax + InMapping.RangeMaxOffset * level;
	OutMapping.MappingMin = InMapping.MappingMin;
	OutMapping.MappingMax = InMapping.MappingMax;
	OutMapping.RangeMinOffset = InMapping.RangeMinOffset;
	OutMapping.RangeMaxOffset = InMapping.RangeMaxOffset;
}

float ATerrain::GetHighMountainRatio(float X, float Y)
{
	FStructHeightMapping mapping;
	MappingByLevel(HighMountainLevel, HighRangeMapping, mapping);
	return GetHeightRatio(NWHighMountain, mapping, X, Y);
}

float ATerrain::GetLowMountianRatio(float X, float Y)
{
	FStructHeightMapping mapping;
	MappingByLevel(LowMountainLevel, LowRangeMapping, mapping);
	return GetHeightRatio(NWLowMountain, mapping, X, Y);
}

float ATerrain::GetWaterRatio(float X, float Y)
{
	FStructHeightMapping mapping;
	FStructHeightMapping groundMapping;
	MappingByLevel(WaterLevel, WaterRangeMapping, mapping);
	MappingByLevel(WaterLevel, WaterGroundRangeMapping, groundMapping);
	float ratio = GetHeightRatio(NWWater, mapping, X, Y) + GetHeightRatio(NWWater, groundMapping, X, Y);
	ratio = ratio > 0.0 ? 0.0 : ratio;

	//cal water bank
	ratio = FMath::Abs<float>(ratio);
	float alpha = ratio * WaterBankSharpness;
	alpha = FMath::Clamp<float>(alpha, 0.0, 1.0);
	float exp = FMath::Lerp<float>(3.0, 1.0, alpha);
	ratio = -FMath::Pow(ratio, exp);

	return ratio;
}

float ATerrain::GetNoise2DStd(UFastNoiseWrapper* NWP, float X, float Y, float scale)
{
	float value = 0.0;
	if (NWP != nullptr) {
		value = NWP->GetNoise2D(X * TileNumRowRatio, Y * TileNumColumnRatio);
		value = FMath::Clamp<float>(value * scale, -1.0, 1.0);
		value = (value + 1) * 0.5;
	}
	return value;
}

float ATerrain::GetAltitudeByPos2D(const FVector2D Pos2D, AActor* Caller)
{
	float X = Pos2D.X / TileSizeMultiplier;
	float Y = Pos2D.Y / TileSizeMultiplier;
	float Out_RatioStd;
	float Out_Ratio;
	float Z = GetAltitude(X, Y, Out_RatioStd, Out_Ratio);
	return Z;
}

void ATerrain::CreateVertex(float X, float Y, float& OutRatioStd, float& OutRatio)
{
	float VX = X * TileSizeMultiplier;
	float VY = Y * TileSizeMultiplier;
	float VZ = GetAltitude(X, Y, OutRatioStd, OutRatio);
	Vertices.Add(FVector(VX, VY, VZ));
}

void ATerrain::CreateUV(float X, float Y)
{
	float UVx = X * UVScale;
	float UVy = Y * UVScale;
	UVs.Add(FVector2D(UVx, UVy));
}

//Create vertex Color(R:Altidude G:Moisture B:Temperature A:Biomes)
void ATerrain::CreateVertexColorsForAMTA(float RatioStd, float X, float Y)
{
	float Moisture = GetNoise2DStd(NWMoisture, X, Y, 3.0);
	float Temperature = GetNoise2DStd(NWTemperature, X, Y, 3.0);
	float Biomes = GetNoise2DStd(NWBiomes, X, Y, 3.0);
	VertexColors.Add(FLinearColor(RatioStd, Moisture, Temperature, Biomes));
}

void ATerrain::AddTreeValues(float X, float Y)
{
	float value = NWTree->GetNoise2D(X, Y);
	value = (value - OneMinTAS) / TreeAreaScaleA;
	value = FMath::Clamp<float>(value, 0.0, 1.0);
	TreeValues.Add(value);
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

	WorkflowState = Enum_TerrainWorkflowState::DrawLandMesh;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, NormalizeNormalsLoopData.Rate, false);
	UE_LOG(Terrain, Log, TEXT("Normalize normals done."));
}

void ATerrain::CreateTerrainMesh()
{
	TerrainMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, 
		TArray<FProcMeshTangent>(), true );
	TerrainMesh->bUseComplexAsSimpleCollision = true;
	TerrainMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	TerrainMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	TerrainMesh->bUseComplexAsSimpleCollision = false;
}

void ATerrain::SetTerrainMaterial()
{
	TerrainMesh->SetMaterial(0, TerrainMaterialIns);
}

void ATerrain::CreateWater()
{
	if (HasWater) {
		CreateWaterPlane();
		if (HasCaustics) {
			CreateCaustics();
		}
	}
}

void ATerrain::SetWaterZ()
{
	if (HasWater) {
		UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("WaterBaseRatio"),
			WaterBaseRatio);
	}
	else {
		UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("WaterBaseRatio"),
			-2.0);
	}
}

void ATerrain::CreateWaterPlane()
{
	CreateWaterVerticesAndUVs();
	CreateWaterTriangles();
	CreateWaterNormals();
	CreateWaterMesh();
	SetWaterMaterial();
}

void ATerrain::CreateWaterVerticesAndUVs()
{
	float HalfRow = TileSizeMultiplier * NumRows / 2.0;
	float RowLen = TileSizeMultiplier * NumRows / WaterNumRows;

	float HalfColumn = TileSizeMultiplier * NumColumns / 2.0;
	float ColLen = TileSizeMultiplier * NumColumns / WaterNumColumns;

	UKismetMaterialLibrary::SetScalarParameterValue(this, TerrainMPC, TEXT("WaterBase"),
		WaterBase);

	float UVRow = UVScale / WaterNumRows;
	float UVColumn = UVScale / WaterNumColumns;

	int32 i;
	int32 j;
	for (i = 0; i <= WaterNumRows; i++) {
		for (j = 0; j <= WaterNumColumns; j++) {
			WaterVertices.Add(FVector(RowLen * i - HalfRow, ColLen * j - HalfColumn, WaterBase));
			WaterUVs.Add(FVector2D(UVRow * i - 0.5, UVColumn * j - 0.5));
		}
	}
}

void ATerrain::CreateWaterTriangles()
{
	int32 ColumnVertexNum = WaterNumColumns + 1;
	int32 RowMinOne = WaterNumRows - 1;
	int32 ColumnMinOne = WaterNumColumns - 1;

	int32 i;
	int32 j;

	int32 RowVertex;
	int32 RowPlusOneVertex;

	for (i = 0; i <= RowMinOne; i++) {
		RowVertex = i * ColumnVertexNum;
		RowPlusOneVertex = (i + 1) * ColumnVertexNum;
		for (j = 0; j <= ColumnMinOne; j++) {
			WaterTriangles.Add(j + RowVertex);
			WaterTriangles.Add(j + 1 + RowPlusOneVertex);
			WaterTriangles.Add(j + RowPlusOneVertex);
			WaterTriangles.Add(j + RowVertex);
			WaterTriangles.Add(j + 1 + RowVertex);
			WaterTriangles.Add(j + 1 + RowPlusOneVertex);
		}
	}
}

void ATerrain::CreateWaterNormals()
{
	int32 i;
	for (i = 0; i <= WaterVertices.Num() - 1; i++) {
		WaterNormals.Add(FVector(0, 0, 1.0));
	}
}

void ATerrain::CreateWaterMesh()
{
	WaterMesh->CreateMeshSection_LinearColor(0, WaterVertices, WaterTriangles, WaterNormals, WaterUVs, 
		TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void ATerrain::SetWaterMaterial()
{
	WaterMesh->SetMaterial(0, WaterMaterialIns);
}

void ATerrain::CreateCaustics()
{
	float base = UKismetMaterialLibrary::GetScalarParameterValue(this, TerrainMPC, TEXT("WaterBase"));
	float sink = (base - WaterRangeMapping.MappingMin * TileAltitudeMultiplier) / 2.0;
	FVector size(TileSizeMultiplier * NumRows, TileSizeMultiplier * NumColumns, sink + 1);
	FVector location(0, 0, base - sink - 1);
	FRotator rotator(0.0, 0.0, 0.0);

	UGameplayStatics::SpawnDecalAtLocation(this, CausticsMaterialIns, size, location, rotator);
}

