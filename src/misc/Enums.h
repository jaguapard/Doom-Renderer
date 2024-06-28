#pragma once
enum class WheelAdjustmentMode : uint32_t
{
	FLY_SPEED,
	FOV,
	COUNT,
};

enum class SkyRenderingMode : uint32_t
{
	NONE,
	ROTATING,
	SPHERE,
	COUNT
};

enum class FogEffectVersion
{
	//DISABLED,
	LINEAR_WITH_CLAMP,
	EXPONENTIAL,
	COUNT
};