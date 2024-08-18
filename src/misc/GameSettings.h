#pragma once
#include "../Vec.h"
#include "Enums.h"

struct GameSettings
{
	real flySpeed = 6;
	real gamma = 1.3;
	real nearPlaneZ = -1;
	real fovMult = 1;

	real camAngAdjustmentSpeed_Mouse = 1e-3;
	real camAngAdjustmentSpeed_Keyboard = 3e-2;

	real fogIntensity = 600;
	FogEffectVersion fogEffectVersion = FogEffectVersion::LINEAR_WITH_CLAMP;

	WheelAdjustmentMode wheelAdjMod = WheelAdjustmentMode::FLY_SPEED;
	SkyRenderingMode skyRenderingMode = SkyRenderingMode::SPHERE;

	bool fogEnabled = false;
	bool mouseCaptured = false;
	bool wireframeEnabled = false;
	bool backfaceCullingEnabled = false;
	bool bufferCleaningEnabled = false;
	bool performanceMonitorDisplayEnabled = true;
	bool ditheringEnabled = true;

	int ssaaMult;
};