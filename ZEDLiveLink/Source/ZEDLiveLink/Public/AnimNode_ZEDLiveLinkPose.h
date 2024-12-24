// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNodeBase.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "LiveLinkTypes.h"
#include "ILiveLinkClient.h"

#include "CoreMinimal.h"
#include <deque>
#include <algorithm>

#include "AnimNode_ZEDLiveLinkPose.generated.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS  //禁用 编译器的弃用警告

USTRUCT(BlueprintType)
struct ZEDLIVELINK_API FAnimNode_ZEDLiveLinkPose: public FAnimNode_Base
{
	GENERATED_BODY()

public:  // 下面配置了 该node的输入属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input)
	FPoseLink InputPose;  //输入姿势的引用

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	FLiveLinkSubjectName LiveLinkSubjectName;  //livelink主题名称

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh; // 骨骼网格体 指针

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	TMap<FName, FName> BoneNameMap34;  //34joint骨骼映射

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	TMap<FName, FName> BoneNameMap38;  // 38joint骨骼映射

	TMap<FName, FName>* CurBoneNameMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	bool bMirrorOnZAxis;  // 是否在Z轴上镜像

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	float ManualHeightOffset; // 手动的高度偏移

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	bool bStickAvatarOnFloor;  // 是否将avatar粘在地板上

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	bool bEnableScaling; // 是否启用缩放

public:
	FAnimNode_ZEDLiveLinkPose();

	//~ FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext & Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext & Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual bool HasPreUpdate() const { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	//~ End of FAnimNode_Base interface

	bool Serialize(FArchive& Ar);  // 序列化 -- 用于保存和加载node状态，目前就是returne false
protected:
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;

private: //私有函数
	void PropagateRestPoseRotations(int32 parentIdx, FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames, TArray<int32> SourceBoneParents, FQuat restPoseRot, bool inverse);

	void BuildPoseFromZEDAnimationData(float DeltaTime, const FLiveLinkSkeletonStaticData* InSkeletonData,
		const FLiveLinkAnimationFrameData* InFrameData,
		FCompactPose& OutPose);
	void PutInRefPose(FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames);
	FCompactPoseBoneIndex GetCPIndex(int32 idx, FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames);
	float ComputeRootTranslationFactor(FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames, const FLiveLinkAnimationFrameData* InFrameData);

	// This is the bone we will apply position translation to.
	// The root in our case is the pelvis (0)
	// root-joint的名称
	FName GetTargetRootName() const { return "PELVIS"; }

	ILiveLinkClient* LiveLinkClient_AnyThread;

	// Delta time from update so that it can be passed to retargeter
	float CachedDeltaTime; // 缓存的 deltaTime，用于传递给重定位器
 
    int NbKeypoints = -1;  // 关键点数量
    TMap<int, FName> Keypoints;  // 关键点
    TMap<int, FName> KeypointsMirrored;
    TArray<int> ParentsIdx; // 父节点索引

	//FBoneContainer& RequiredBones;

    float DurationOffsetErrorThreshold = 3.0f;  // 时间偏移误差阈值
    float DurationOffsetError = 0.0f; // 时间偏移误差
    long long PreviousTS_ms = 0; // 上一次的时间戳

    float DistanceToFloorThreshold = 1.0f; // cm 距离地面的阈值

	float AutomaticHeightOffset = 0;  // 根据动作数据，计算的 角色脚部到地面的距离
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS  // 重新开启 编译器 的 弃用警告

// 指导UE对于FAnimNode_ZEDLiveLinkPose类型的序列化
template<> struct TStructOpsTypeTraits<FAnimNode_ZEDLiveLinkPose> : public TStructOpsTypeTraitsBase2<FAnimNode_ZEDLiveLinkPose>
{
	enum
	{
		WithSerializer = true
	};
};


