#pragma once

#include "UObject/ObjectMacros.h"
#include "Math/RandomStream.h"
#include "Distributions/DistributionVector.h"
//#include "Particles/Orientation/ParticleModuleOrientationAxisLock.h"
#include "Particles/TypeData/ParticleModuleTypeDataBase.h"

class UParticleEmitter;
class UParticleSystemComponent;
struct FParticleEmitterInstance;
class UStaticMesh;

//UENUM()
enum EMeshScreenAlignment : int
{
    PSMA_MeshFaceCameraWithRoll,
    PSMA_MeshFaceCameraWithSpin,
    PSMA_MeshFaceCameraWithLockedAxis,
    PSMA_MAX,
};

//UENUM()
enum EMeshCameraFacingUpAxis : int
{
    CameraFacing_NoneUP,
    CameraFacing_ZUp,
    CameraFacing_NegativeZUp,
    CameraFacing_YUp,
    CameraFacing_NegativeYUp,
    CameraFacing_MAX,
};

//UENUM()
enum EMeshCameraFacingOptions : int
{
    XAxisFacing_NoUp, 
    XAxisFacing_ZUp, 
    XAxisFacing_NegativeZUp,
    XAxisFacing_YUp, 
    XAxisFacing_NegativeYUp,

    LockedAxis_ZAxisFacing, 
    LockedAxis_NegativeZAxisFacing,
    LockedAxis_YAxisFacing, 
    LockedAxis_NegativeYAxisFacing,

    VelocityAligned_ZAxisFacing, 
    VelocityAligned_NegativeZAxisFacing,
    VelocityAligned_YAxisFacing, 
    VelocityAligned_NegativeYAxisFacing,

    EMeshCameraFacingOptions_MAX,
};

//UCLASS(editinlinenew, hidecategories = Object, MinimalAPI, meta = (DisplayName = "Mesh Data"))
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
    DECLARE_CLASS(UParticleModuleTypeDataMesh, UParticleModuleTypeDataBase)
public:
    UParticleModuleTypeDataMesh();
    ~UParticleModuleTypeDataMesh() = default;

    /** The static mesh to render at the particle positions */
    //UPROPERTY(EditAnywhere, Category = Mesh)
    UStaticMesh* Mesh;

    /** Random stream for the initial rotation distribution */
    FRandomStream RandomStream;

    /** use the static mesh's LOD setup and switch LODs based on largest particle's screen size*/
    //UPROPERTY(EditAnywhere, Category = Mesh)
    float LODSizeScale;

    /** use the static mesh's LOD setup and switch LODs based on largest particle's screen size*/
    //UPROPERTY(EditAnywhere, Category = Mesh)
    uint8 bUseStaticMeshLODs : 1;

    /** If true, has the meshes cast shadows */
    //UPROPERTY()
    uint8 CastShadows : 1;

    /** UNUSED (the collision module dictates doing collisions) */
    //UPROPERTY()
    uint8 DoCollisions : 1;

    /**
     *	The alignment to use on the meshes emitted.
     *	The RequiredModule->ScreenAlignment MUST be set to PSA_TypeSpecific to use.
     *	One of the following:
     *	PSMA_MeshFaceCameraWithRoll
     *		Face the camera allowing for rotation around the mesh-to-camera FVector
     *		(amount provided by the standard particle sprite rotation).
     *	PSMA_MeshFaceCameraWithSpin
     *		Face the camera allowing for the mesh to rotate about the tangential axis.
     *	PSMA_MeshFaceCameraWithLockedAxis
     *		Face the camera while maintaining the up FVector as the locked direction.
     */
    //UPROPERTY(EditAnywhere, Category = Mesh)
    EMeshScreenAlignment MeshAlignment;

    /**
     *	If true, use the emitter material when rendering rather than the one applied
     *	to the static mesh model.
     */
    //UPROPERTY(EditAnywhere, Category = Mesh)
    uint8 bOverrideMaterial : 1;

    //UPROPERTY(EditAnywhere, Category = Mesh)
    uint8 bOverrideDefaultMotionBlurSettings : 1;

    //UPROPERTY(EditAnywhere, Category = Mesh, meta = (EditCondition = "bOverrideDefaultMotionBlurSettings"))
    uint8 bEnableMotionBlur : 1;

    /** The 'pre' rotation pitch (in degrees) to apply to the static mesh used. */
    //UPROPERTY(EditAnywhere, Category = Orientation)
    FRawDistributionVector RollPitchYawRange;

    /**
     *	The axis to lock the mesh on. This overrides TypeSpecific mesh alignment as well as the LockAxis module.
     *		EPAL_NONE		 -	No locking to an axis.
     *		EPAL_X			 -	Lock the mesh X-axis facing towards +X.
     *		EPAL_Y			 -	Lock the mesh X-axis facing towards +Y.
     *		EPAL_Z			 -	Lock the mesh X-axis facing towards +Z.
     *		EPAL_NEGATIVE_X	 -	Lock the mesh X-axis facing towards -X.
     *		EPAL_NEGATIVE_Y	 -	Lock the mesh X-axis facing towards -Y.
     *		EPAL_NEGATIVE_Z	 -	Lock the mesh X-axis facing towards -Z.
     *		EPAL_ROTATE_X	 -	Ignored for mesh emitters. Treated as EPAL_NONE.
     *		EPAL_ROTATE_Y	 -	Ignored for mesh emitters. Treated as EPAL_NONE.
     *		EPAL_ROTATE_Z	 -	Ignored for mesh emitters. Treated as EPAL_NONE.
     */
    //UPROPERTY(EditAnywhere, Category = Orientation)
    EParticleAxisLock AxisLockOption;

    /**
     *	If true, then point the X-axis of the mesh towards the camera.
     *	When set, AxisLockOption as well as all other locked axis/screen alignment settings are ignored.
     */
    //UPROPERTY(EditAnywhere, Category = CameraFacing)
    uint8 bCameraFacing : 1;

    /**
     *	The axis of the mesh to point up when camera facing the X-axis.
     *		CameraFacing_NoneUP			No attempt to face an axis up or down.
     *		CameraFacing_ZUp			Z-axis of the mesh should attempt to point up.
     *		CameraFacing_NegativeZUp	Z-axis of the mesh should attempt to point down.
     *		CameraFacing_YUp			Y-axis of the mesh should attempt to point up.
     *		CameraFacing_NegativeYUp	Y-axis of the mesh should attempt to point down.
     */
    //UPROPERTY()
    EMeshCameraFacingUpAxis CameraFacingUpAxisOption_DEPRECATED;

    /**
     *	The camera facing option to use:
     *	All camera facing options without locked axis assume X-axis will be facing the camera.
     *		XAxisFacing_NoUp				- X-axis camera facing, no attempt to face an axis up or down.
     *		XAxisFacing_ZUp					- X-axis camera facing, Z-axis of the mesh should attempt to point up.
     *		XAxisFacing_NegativeZUp			- X-axis camera facing, Z-axis of the mesh should attempt to point down.
     *		XAxisFacing_YUp					- X-axis camera facing, Y-axis of the mesh should attempt to point up.
     *		XAxisFacing_NegativeYUp			- X-axis camera facing, Y-axis of the mesh should attempt to point down.
     *	All axis-locked camera facing options assume the AxisLockOption is set. EPAL_NONE will be treated as EPAL_X.
     *		LockedAxis_ZAxisFacing			- X-axis locked on AxisLockOption axis, rotate Z-axis of the mesh to face towards camera.
     *		LockedAxis_NegativeZAxisFacing	- X-axis locked on AxisLockOption axis, rotate Z-axis of the mesh to face away from camera.
     *		LockedAxis_YAxisFacing			- X-axis locked on AxisLockOption axis, rotate Y-axis of the mesh to face towards camera.
     *		LockedAxis_NegativeYAxisFacing	- X-axis locked on AxisLockOption axis, rotate Y-axis of the mesh to face away from camera.
     *	All velocity-aligned options do NOT require the ScreenAlignment be set to PSA_Velocity.
     *	Doing so will result in additional work being performed... (it will orient the mesh twice).
     *		VelocityAligned_ZAxisFacing         - X-axis aligned to the velocity, rotate the Z-axis of the mesh to face towards camera.
     *		VelocityAligned_NegativeZAxisFacing - X-axis aligned to the velocity, rotate the Z-axis of the mesh to face away from camera.
     *		VelocityAligned_YAxisFacing         - X-axis aligned to the velocity, rotate the Y-axis of the mesh to face towards camera.
     *		VelocityAligned_NegativeYAxisFacing - X-axis aligned to the velocity, rotate the Y-axis of the mesh to face away from camera.
     */
    //UPROPERTY(EditAnywhere, Category = CameraFacing)
    EMeshCameraFacingOptions CameraFacingOption;

    /**
     *	If true, apply 'sprite' particle rotation about the orientation axis (direction mesh is pointing).
     *	If false, apply 'sprite' particle rotation about the camera facing axis.
     */
    //UPROPERTY(EditAnywhere, Category = CameraFacing)
    uint8 bApplyParticleRotationAsSpin : 1;

    /**
    *	If true, all camera facing options will point the mesh against the camera's view direction rather than pointing at the cameras location.
    *	If false, the camera facing will point to the cameras position as normal.
    */
    //UPROPERTY(EditAnywhere, Category = CameraFacing)
    uint8 bFaceCameraDirectionRatherThanPosition : 1;

    /**
    *	If true, all collisions for mesh particle on this emitter will take the particle size into account.
    *	If false, particle size will be ignored in collision checks.
    */
    //UPROPERTY(EditAnywhere, Category = Collision)
    uint8 bCollisionsConsiderPartilceSize : 1;

    //virtual void PostLoad();

    //~ Begin UObject Interface
    //virtual void	Serialize(FArchive& Ar) override;

    //~ End UObject Interface

    void CreateDistribution();

    //~ Begin UParticleModule Interface
    //virtual void	SetToSensibleDefaults(UParticleEmitter* Owner) override;
    //~ End UParticleModule Interface

    //~ Begin UParticleModuleTypeDataBase Interface
    virtual FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) override;
    virtual bool	SupportsSpecificScreenAlignmentFlags() const override { return true; }
    virtual bool	SupportsSubUV() const override { return true; }
    virtual bool	IsAMeshEmitter() const override { return true; }
};
