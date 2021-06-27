#include "Math/Vector.isph"
#include "Math/Matrix.isph"
#include "Math/Quat.isph"
#include "Math/Transform.isph"
#include "Math/WideVector.isph"

// NOTE:
// uniform FVector v = InV[Index];
// varying FVector v = VectorLoad(&InV[extract(Index, 0)]);

static inline bool VectorNormalize(FVector& V)
{
	const float SquareSum = VectorSizeSquared(V);
	if(SquareSum > SMALL_NUMBER)
	{
		const float Scale = InvSqrt(SquareSum);
		V = V * Scale;
		return true;
	}
	return false;
}

static inline uniform FVector VectorSafeClamp(uniform FVector& Value,
                                              const uniform FVector& Min,
                                              const uniform FVector& Max)
{
    uniform FVector mask = VectorCompareGE(Value, Min);
    Value = VectorSelect(mask, Value, Min);

    mask = VectorCompareGE(Max, Value);
    Value = VectorSelect(mask, Value, Max);
    return Value;
}

static inline uniform FVector checkMapRange(const uniform FVector& InMapSize,
                                            const uniform FVector& InCurrentPosition,
                                            const uniform FVector& InCurrentVelocity)
{
    uniform FVector newVelocity = InCurrentVelocity;
    if (InCurrentPosition.V[0] > InMapSize.V[0] || InCurrentPosition.V[0] < -InMapSize.V[0]) {
       newVelocity.V[0] *= -1.f;
    }

    if (InCurrentPosition.V[1] > InMapSize.V[1] || InCurrentPosition.V[1] < -InMapSize.V[1]) {
       newVelocity.V[1] *= -1.f;
    }

    if (InCurrentPosition.V[2] > InMapSize.V[2] || InCurrentPosition.V[2] < -InMapSize.V[2]) {
       newVelocity.V[2] *= -1.f;
    }
    return newVelocity;
}

export void UpdateFishAgentState(uniform FVector CurX[],
                                 const uniform FVector PrevX[],

                                 uniform FVector CurV[],
                                 const uniform FVector PrevV[],

                                 uniform FVector CurA[],
                                 const uniform FVector PrevA[],

                                 const uniform int32 FishesCount,
                                 const uniform float DeltaTime,
                                 const uniform float kCoh,
                                 const uniform float kSep,
                                 const uniform float kAlign,

                                 const uniform float rCohesion,
                                 const uniform float rSeparation,
                                 const uniform float rAlignment,

                                 const uniform FVector& maxAccel,
                                 const uniform FVector& maxVel,
                                 const uniform FVector& mapSize)
{
    // (snote) [foreach] is used only for function that returns value and receive purely const params!
    //foreach(Index = 0 ... FishesCount)
    for(uniform int32 Index = 0; Index < FishesCount; Index++)
    {
        uniform FVector cohesion = ZeroVector;
        uniform FVector separation = ZeroVector;
        uniform FVector alignment = ZeroVector;

        uniform int32 cohesionCnt = 0;
        uniform int32 separationCnt = 0;
        uniform int32 alignmentCnt = 0;

        // Nested "foreach" statements are currently illegal.
        // SequentialFor
        for (uniform int32 i = 0; i < FishesCount; ++i) {
            if (i != Index) {
                const uniform FVector pairDistance = PrevX[i] - PrevX[Index];
                const uniform float distance = sqrt(VectorSizeSquared(pairDistance));
                if (distance > 0) {
                    if (distance < rCohesion) {
                        cohesion = cohesion + PrevX[i];
                        cohesionCnt++;
                    }
                    if (distance < rSeparation) {
                        separation = separation + pairDistance;
                        separationCnt++;
                    }
                    if (distance < rAlignment) {
                        alignment = alignment + PrevX[i];
                        alignmentCnt++;
                    }
                }
            }
        }

        if (cohesionCnt != 0) {
            const uniform float fCohesionCnt = 1.f/cohesionCnt;
            cohesion = cohesion * fCohesionCnt;
            cohesion = cohesion - PrevX[Index];
            cohesion = VectorGetSafeNormal(cohesion);
        }

        if (separationCnt != 0) {
            const uniform float fSeparationCnt = -1.f/separationCnt;
            separation = separation * fSeparationCnt;
            separation = VectorGetSafeNormal(separation);
        }

        if (alignmentCnt != 0) {
            const uniform float fAlignmentCnt = 1.f/alignmentCnt;
            alignment = alignment * fAlignmentCnt;
            alignment = VectorGetSafeNormal(alignment);
        }

        uniform FVector accel = (cohesion * kCoh + separation * kSep + alignment * kAlign);
        uniform FVector tmpCurX = CurX[Index];
        uniform FVector tmpCurV = CurV[Index];
        uniform FVector tmpCurA = CurA[Index];
        tmpCurA = VectorSafeClamp(accel, ZeroVector, maxAccel);
        tmpCurA.V[2] = 0;

        // V += A*dt
        tmpCurV = tmpCurV + tmpCurA * DeltaTime;
        tmpCurV = VectorSafeClamp(tmpCurV, ZeroVector, maxVel);

        // X += V*dt
        tmpCurX = tmpCurX + tmpCurV * DeltaTime;
        tmpCurV = checkMapRange(mapSize, tmpCurX, tmpCurV);

        // Write the results
        {
            CurX[Index] = tmpCurX;
            CurV[Index] = tmpCurV;
            CurA[Index] = tmpCurA;
        }
    }
}
