// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructDefine.h"

#include <FastNoiseWrapper.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(Terrain, Log, All);

UENUM(BlueprintType)
enum class Enum_TerrainWorkflowState: uint8
{
	InitWorkflow,
	CreateVerticesAndUVs,
	CreateTriangles,
	CalNormalsInit,
	CalNormalsAcc,
	NormalizeNormals,
	DrawLandMesh,
	CreateWater,
	CreateTree,
	Done,
	Error
};

UCLASS()
class MAPTESTCPP_API ATerrain : public AActor
{
	GENERATED_BODY()

private:
	//noise param for high mountain
	UFastNoiseWrapper* NWHighMountain;
	EFastNoise_NoiseType NWHighMountain_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWHighMountain_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWHighMountain_FractalType = EFastNoise_FractalType::RigidMulti;
	int32 NWHighMountain_Octaves = 6;
	float NWHighMountain_Lacunarity = 2.0;
	float NWHighMountain_Gain = 0.5;
	float NWHighMountain_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWHighMountain_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWHighMountain_CRT = EFastNoise_CellularReturnType::CellValue;
	
	//noise param for low mountain
	UFastNoiseWrapper* NWLowMountain;
	EFastNoise_NoiseType NWLowMountain_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWLowMountain_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWLowMountain_FractalType = EFastNoise_FractalType::RigidMulti;
	int32 NWLowMountain_Octaves = 6;
	float NWLowMountain_Lacunarity = 2.0;
	float NWLowMountain_Gain = 0.5;
	float NWLowMountain_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWLowMountain_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWLowMountain_CRT = EFastNoise_CellularReturnType::CellValue;

	//noise param for water
	UFastNoiseWrapper* NWWater;
	EFastNoise_NoiseType NWWater_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWWater_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWWater_FractalType = EFastNoise_FractalType::FBM;
	int32 NWWater_Octaves = 2;
	float NWWater_Lacunarity = 2.0;
	float NWWater_Gain = 0.5;
	float NWWater_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWWater_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWWater_CRT = EFastNoise_CellularReturnType::CellValue;

	//noise param for moisture
	UFastNoiseWrapper* NWMoisture;
	EFastNoise_NoiseType NWMoisture_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWMoisture_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWMoisture_FractalType = EFastNoise_FractalType::FBM;
	int32 NWMoisture_Octaves = 3;
	float NWMoisture_Lacunarity = 2.0;
	float NWMoisture_Gain = 0.5;
	float NWMoisture_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWMoisture_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWMoisture_CRT = EFastNoise_CellularReturnType::CellValue;

	//noise param for temperature
	UFastNoiseWrapper* NWTemperature;
	EFastNoise_NoiseType NWTemperature_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWTemperature_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWTemperature_FractalType = EFastNoise_FractalType::FBM;
	int32 NWTemperature_Octaves = 3;
	float NWTemperature_Lacunarity = 2.0;
	float NWTemperature_Gain = 0.5;
	float NWTemperature_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWTemperature_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWTemperature_CRT = EFastNoise_CellularReturnType::CellValue;

	//noise param for biomes
	UFastNoiseWrapper* NWBiomes;
	EFastNoise_NoiseType NWBiomes_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWBiomes_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWBiomes_FractalType = EFastNoise_FractalType::FBM;
	int32 NWBiomes_Octaves = 3;
	float NWBiomes_Lacunarity = 2.0;
	float NWBiomes_Gain = 0.5;
	float NWBiomes_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWBiomes_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWBiomes_CRT = EFastNoise_CellularReturnType::CellValue;

	//noise param for tree
	UFastNoiseWrapper* NWTree;
	EFastNoise_NoiseType NWTree_NoiseType = EFastNoise_NoiseType::PerlinFractal;
	EFastNoise_Interp NWTree_Interp = EFastNoise_Interp::Quintic;
	EFastNoise_FractalType NWTree_FractalType = EFastNoise_FractalType::FBM;
	int32 NWTree_Octaves = 6;
	float NWTree_Lacunarity = 2.0;
	float NWTree_Gain = 0.5;
	float NWTree_CellularJitter = 0.45;
	EFastNoise_CellularDistanceFunction NWTree_CDF = EFastNoise_CellularDistanceFunction::Euclidean;
	EFastNoise_CellularReturnType NWTree_CRT = EFastNoise_CellularReturnType::CellValue;

	//delegate
	FTimerDynamicDelegate WorkflowDelegate;

	//For normal calculate
	TArray<FVector> NormalsAcc;

	//water param
	float WaterBase;

	//Tree param
	float TreeAreaScaleA = 1.0;
	float OneMinTAS = 0.0;

	//Input
	bool LeftHold = false;
	bool RightHold = false;

	APlayerController* Controller;

protected:
	//Mesh
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UProceduralMeshComponent* TerrainMesh;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UProceduralMeshComponent* WaterMesh;

	//Noise variables BP for high mountain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|HighMountain")
	int32 NWHighMountain_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|HighMountain", meta = (ClampMin = "0.0"))
	float NWHighMountain_NoiseFrequency = 0.005;
	//Noise variables BP for low mountain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|LowMountain")
	int32 NWLowMountain_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|LowMountain", meta = (ClampMin = "0.0"))
	float NWLowMountain_NoiseFrequency = 0.01;
	//Noise variables BP for water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Water")
	int32 NWWater_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Water", meta = (ClampMin = "0.0"))
	float NWWater_NoiseFrequency = 0.01;
	//Noise variables BP for moisture
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Moisture")
	int32 NWMoisture_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Moisture", meta = (ClampMin = "0.0"))
	float NWMoisture_NoiseFrequency = 0.007;
	//Noise variables BP for temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Temperature")
	int32 NWTemperature_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Temperature", meta = (ClampMin = "0.0"))
	float NWTemperature_NoiseFrequency = 0.007;
	//Noise variables BP for biomes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Biomes")
	int32 NWBiomes_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Biomes", meta = (ClampMin = "0.0"))
	float NWBiomes_NoiseFrequency = 0.007;
	//Noise variables BP for tree
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Tree")
	int32 NWTree_NoiseSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise|Tree", meta = (ClampMin = "0.0"))
	float NWTree_NoiseFrequency = 0.01;

	//Tile variables BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile", meta = (ClampMin = "0"))
	int32 NumRows = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile", meta = (ClampMin = "0"))
	int32 NumColumns = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile", meta = (ClampMin = "0.0"))
	float TileScale = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile", meta = (ClampMin = "0.0"))
	float TileSize = 100.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile", meta = (ClampMin = "0.0"))
	float TileHeight = 5000.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile", meta = (ClampMin = "0.0"))
	float UVScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HighMountainLevel = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	FStructHeightMapping HighRangeMapping = { 0.4, 0.6, 0.0, 0.7, -0.2, -0.2 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowMountainLevel = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	FStructHeightMapping LowRangeMapping = { 0.5, 1.0, 0.0, 0.3, -0.4, 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	bool HasWater = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	bool HasCaustics = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WaterLevel = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	FStructHeightMapping WaterRangeMapping = { -0.6, -0.4, -0.4, -0.1, 0.2, 0.2 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	FStructHeightMapping WaterGroundRangeMapping = { -0.4, 1.0, 0.1, 0.2, 0.2, 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "-1.0", ClampMax = "0.0"))
	float WaterBaseRatio = -0.005;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "1"))
	int32 WaterNumRows = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "1"))
	int32 WaterNumColumns = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float LavaBaseRatio = 0.02;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float DesertBaseRatio = 0.03;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SwampBaseRatio = 0.03;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain")
	float TerrainBlendOffset = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "1.0"))
	float TerrainBlendSharpness = 50.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Terrain", meta = (ClampMin = "1.0"))
	float WaterBankSharpness = 50.0;

	UPROPERTY(BlueprintReadOnly)
	float TileSizeMultiplier = 100;
	UPROPERTY(BlueprintReadOnly)
	float TileHeightMultiplier = 100;
	UPROPERTY(BlueprintReadOnly)
	float TerrainWidth;
	UPROPERTY(BlueprintReadOnly)
	float TerrainHeight;

	//Tree variables BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tree", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TreeAreaScale = 1.0;

	//Material BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* TerrainMaterialIns;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialParameterCollection* TerrainMPC;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* WaterMaterialIns;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
	class UMaterialInstance* CausticsMaterialIns;

	//Timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Timer")
	float DefaultTimerRate = 0.01f;

	//Loop BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CreateVerticesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CreateTrianglesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CalNormalsInitLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData CalNormalsAccLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
	FStructLoopData NormalizeNormalsLoopData;

	//Render variables
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Land")
	TArray<FVector> Vertices;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Land")
	TArray<FVector2D> UVs;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Land")
	TArray<int32> Triangles;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Land")
	TArray<FVector> Normals;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Land")
	TArray<FLinearColor> VertexColors;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Water")
	TArray<FVector> WaterVertices;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Water")
	TArray<FVector2D> WaterUVs;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Water")
	TArray<int32> WaterTriangles;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render|Water")
	TArray<FVector> WaterNormals;

	//Tree
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Tree")
	TArray<float> TreeValues;

	//Workflow
	UPROPERTY(BlueprintReadOnly)
	Enum_TerrainWorkflowState WorkflowState = Enum_TerrainWorkflowState::InitWorkflow;

	//Progress
	UPROPERTY(BlueprintReadOnly)
	int32 ProgressTarget = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 ProgressCurrent = 0;

	//Hex grid
	UPROPERTY(BlueprintReadOnly)
	class AHexGrid* HexGrid;
	
	//Input
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Input")
	class UInputAction* MouseLeftHoldAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Input")
	class UInputAction* MouseRightHoldAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Input")
	float HoldTraceLength = 1000000;

public:	
	// Sets default values for this actor's properties
	ATerrain();

	bool IsWorkFlowStepDone(Enum_TerrainWorkflowState state);
	float GetAltitudeByPos2D(const FVector2D Pos2D, AActor* Caller);

	UFUNCTION(BlueprintCallable)
	bool IsLeftHold();
	UFUNCTION(BlueprintCallable)
	bool IsRightHold();

private:
	//Timer delegate
	void BindDelegate();

	//Input
	void EnablePlayer();
	void BindEnchancedInputAction();

	UFUNCTION()
	void OnLeftHoldStarted(const FInputActionValue& Value);

	UFUNCTION()
	void OnLeftHoldCompleted(const FInputActionValue& Value);

	UFUNCTION()
	void OnRightHoldStarted(const FInputActionValue& Value);

	UFUNCTION()
	void OnRightHoldCompleted(const FInputActionValue& Value);

	//Init workflow
	void InitWorkflow();
	void CreateNoise();
	void InitTileParameter();
	void InitReceiveDecal();
	void InitLoopData();
	void InitHexGrid();
	void InitTerrainFormBaseRatio();
	void InitWater();
	void InitTreeParam();
	bool CheckMaterialSetting();

	//create Workflow
	UFUNCTION()
	void CreateTerrainFlow();

	//Vertices create
	void CreateVertices();
	float GetAltitude(float X, float Y, float& OutRatioStd, float& OutRatio);
	float MappingFromRangeToRange(float InputValue, const FStructHeightMapping& Mapping);
	float GetHeightRatio(UFastNoiseWrapper* NWP, const FStructHeightMapping& Mapping, float X, float Y);
	void MappingByLevel(float level, const FStructHeightMapping& InMapping, FStructHeightMapping& OutMapping);
	float GetHighMountainRatio(float X, float Y);
	float GetLowMountianRatio(float X, float Y);
	float GetWaterRatio(float X, float Y);


	float GetNoise2DStd(UFastNoiseWrapper* NWP, float X, float Y, float scale);
	void CreateVertex(float X, float Y, float& OutRatioStd, float& OutRatio);
	void CreateUV(float X, float Y);
	void CreateVertexColorsForAMTA(float RatioStd, float X, float Y);
	void AddTreeValues(float X, float Y);

	//Triangles create
	void CreateTriangles();
	void CreatePairTriangles(int32 ColumnIndex, int32 RowVertex, int32 RowPlusOneVertex);

	//Normals create
	void CreateNormals();
	void CalNormalsWorkflow();
	void CalNormalsInit();
	void CalNormalsAcc();
	void CalTriangleNormalForVertex(int32 Index);
	void NormalizeNormals();

	//Mesh create
	void CreateTerrainMesh();
	void SetTerrainMaterial();
	
	//Create Water
	void CreateWater();
	void SetWaterZ();

	void CreateWaterPlane();
	void CreateWaterVerticesAndUVs();
	void CreateWaterTriangles();
	void CreateWaterNormals();
	void CreateWaterMesh();
	void SetWaterMaterial();

	void CreateCaustics();

	void ResetProgress();

	//Input
	bool IsMouseClickTraceHit();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void GetProgress(float& Out_Progress);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsWorkFlowDone()
	{
		return WorkflowState == Enum_TerrainWorkflowState::Done;
	}

	FORCEINLINE bool IsWorkFlowOverStage(Enum_TerrainWorkflowState State)
	{
		return WorkflowState > State;
	}

	FORCEINLINE float GetWidth() {
		return TerrainWidth;
	}

	FORCEINLINE float GetHeight() {
		return TerrainHeight;
	}

	FORCEINLINE float GetWaterBase() {
		return WaterBase;
	}
	
};
