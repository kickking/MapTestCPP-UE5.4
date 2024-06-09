// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructDefine.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(Terrain, Log, All);

class UFastNoiseWrapper;
class AHexGrid;

UENUM(BlueprintType)
enum class Enum_TerrainWorkflowState: uint8
{
	InitWorkflow,
	CreateVerticesAndUVs,
	CreateTriangles,
	CalNormalsInit,
	CalNormalsAcc,
	NormalizeNormals,
	DrawMesh,
	Done,
	Error
};

UCLASS()
class MAPTESTCPP_API ATerrain : public AActor
{
	GENERATED_BODY()

private:
	//noise param
	UFastNoiseWrapper* NoiseWrapper;
	bool CreateNoiseDone = false;
	int32 Octaves = 8;
	float Lacunarity = 2.0;
	float Gain = 0.5;
	float CellularJitter = 0.45;

	//delegate
	FTimerDynamicDelegate WorkflowDelegate;

	//For normal calculate
	TArray<FVector> NormalsAcc;

protected:
	//Mesh
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
		class UProceduralMeshComponent* TerrainMesh;

	//Noise variables BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise")
		int32 NoiseSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Noise", meta = (ClampMin = "0.0"))
		float NoiseFrequency = 0.006;

	//Tile variables BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0"))
		int32 NumRows = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0"))
		int32 NumColumns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0.0"))
		float TileScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0.0"))
		float TileSize = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0.0"))
		float TileHeight = 10000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0.0"))
		float UVScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Tile|Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
		float Horizon = 0.5;

	float TileSizeMultiplier;
	float TileHeightMultiplier;

	//Material BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Material")
		class UMaterialInstance* TerrainMaterialIns;

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
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render")
		TArray<FVector> Vertices;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render")
		TArray<FVector2D> UVs;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render")
		TArray<int32> Triangles;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Custom|Render")
		TArray<FVector> Normals;

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
		AHexGrid* HexGrid;
	
public:	
	// Sets default values for this actor's properties
	ATerrain();

	bool isCreateNoiseDone();
	float GetAltitudeByPos2D(const FVector2D Pos2D, AActor* Caller);

private:
	//Timer delegate
	void BindDelegate();

	//Init workflow
	void InitWorkflow();
	void CreateNoise();
	void InitTileParameter();
	void InitLoopData();
	void InitHexGrid();

	//create Workflow
	UFUNCTION()
		void CreateTerrainFlow();

	//Vertices create
	void CreateVertices();
	float GetAltitude(UFastNoiseWrapper* NoiseWP, float X, float Y, float InHorizon, float Multiplier);
	void CreateVertex(float X, float Y);
	void CreateUV(float X, float Y);

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
	
	void ResetProgress();

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

};
