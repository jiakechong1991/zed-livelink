// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_ZEDLiveLinkPose.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimTrace.h"
#include "Features/IModularFeatures.h"
#include "Math/Quat.h"
#include "Math/UnrealMathUtility.h"
#include "ZEDSkeletonRetarget.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include <chrono>

// 构造函数，初始化 属性变量 到指定默认值
FAnimNode_ZEDLiveLinkPose::FAnimNode_ZEDLiveLinkPose()
    : InputPose()
    , LiveLinkSubjectName()
    , SkeletalMesh(nullptr)
    , BoneNameMap34()
    , BoneNameMap38()
    , CurBoneNameMap(nullptr)
    , bMirrorOnZAxis(false)
    , ManualHeightOffset(-1.f)
    , bStickAvatarOnFloor(false)
    , bEnableScaling(false)
    , LiveLinkClient_AnyThread(nullptr)
    , CachedDeltaTime(0.0f)
    , NbKeypoints(-1)
    , DurationOffsetErrorThreshold(3.0f)
    , DurationOffsetError(0.f)
    , PreviousTS_ms(0)
    , DistanceToFloorThreshold(0.f)
    , AutomaticHeightOffset(0.f)
{
}

/*
计算 动捕数据的尺度和UE骨架对比，获得缩放因子
*/
float FAnimNode_ZEDLiveLinkPose::ComputeRootTranslationFactor(FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames, const FLiveLinkAnimationFrameData* InFrameData)
{
    float avatarTotalTranslation = 0.f;
    float SDKTotalTranslation = 0.f;

    if (NbKeypoints <= 34)
    {
        for (int32 i = 22; i < 24; i++)
        {
            FTransform BoneTransform = InFrameData->Transforms[i];
            FCompactPoseBoneIndex CPIndex = GetCPIndex(i, OutPose, TransformedBoneNames);
            if (CPIndex != INDEX_NONE)
            {
                avatarTotalTranslation += OutPose[CPIndex].GetTranslation().Size();
                SDKTotalTranslation += BoneTransform.GetTranslation().Size();
            }
        }
    }
    else
    {
        for (int32 i = 19; i < 23; i++)
        {
            FTransform BoneTransform = InFrameData->Transforms[i];
            FCompactPoseBoneIndex CPIndex = GetCPIndex(i, OutPose, TransformedBoneNames);
            if (CPIndex != INDEX_NONE)
            {
                avatarTotalTranslation += OutPose[CPIndex].GetTranslation().Size();
                SDKTotalTranslation += BoneTransform.GetTranslation().Size();
            }
        }
    }

    float factor = avatarTotalTranslation / SDKTotalTranslation;
    float scale = 1.f;
    FCompactPoseBoneIndex CPIndexRoot = GetCPIndex(0, OutPose, TransformedBoneNames);
    if (CPIndexRoot != INDEX_NONE)
        scale = OutPose[CPIndexRoot].GetScale3D().Z;
    return FMath::Abs(scale * factor);
}

FCompactPoseBoneIndex FAnimNode_ZEDLiveLinkPose::GetCPIndex(int32 idx, FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames) {
    FName BoneName = TransformedBoneNames[idx];
    const int32 MeshIndex = OutPose.GetBoneContainer().GetPoseBoneIndexForBoneName(BoneName);
    if (MeshIndex != INDEX_NONE)
    {
        FCompactPoseBoneIndex CPIndex = OutPose.GetBoneContainer().MakeCompactPoseIndex(
            FMeshPoseBoneIndex(MeshIndex));
        return CPIndex;
    }
    return (FCompactPoseBoneIndex)INDEX_NONE;
}

/*将所有骨骼设置为它们的参考姿势（RefPose），这是骨骼的默认或休息姿势*/
void FAnimNode_ZEDLiveLinkPose::PutInRefPose(FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames) {
    for (int32 i = 0; i < TransformedBoneNames.Num(); i++)
    {
        FCompactPoseBoneIndex CPIndex = GetCPIndex(i, OutPose, TransformedBoneNames);
        if (CPIndex != INDEX_NONE)
        {
            auto RefBoneTransform = OutPose.GetRefPose(CPIndex);
            OutPose[CPIndex].SetRotation(RefBoneTransform.GetRotation());
        }
    }
}

/*
递归地将休息姿势的旋转应用到骨骼的子结构上，确保整个骨骼链保持正确的相对旋转
*/
void FAnimNode_ZEDLiveLinkPose::PropagateRestPoseRotations(int32 parentIdx, FCompactPose& OutPose, TArray<FName, TMemStackAllocator<>> TransformedBoneNames, TArray<int32> SourceBoneParents, FQuat restPoseRot, bool inverse) {
    for (int32 i = 0; i < SourceBoneParents.Num(); i++) {
        FCompactPoseBoneIndex CPIndex = GetCPIndex(i, OutPose, TransformedBoneNames);
        if (SourceBoneParents[i] == parentIdx)
        {
            FQuat restPoseRotChild;
            if (CPIndex != INDEX_NONE)
            {
                FQuat jointRotation = restPoseRot * OutPose[CPIndex].GetRotation();
                OutPose[CPIndex].SetRotation(jointRotation);
                if (!inverse)
                    restPoseRotChild = restPoseRot * OutPose.GetRefPose(CPIndex).GetRotation();
                else
                    restPoseRotChild = OutPose.GetRefPose(CPIndex).GetRotation().Inverse() * restPoseRot;
            }
            else
                restPoseRotChild = restPoseRot; // in case the parent bone doesn't appear in 
            // the Unreal avatar, we propagate its own restposerot to 
            // its children 
            PropagateRestPoseRotations(i, OutPose, TransformedBoneNames, SourceBoneParents, restPoseRotChild, inverse);
        }
    }
}

/*
将ZED LiveLink系统的动作捕捉数据应用到Unreal的骨骼上
*/
// Take Live Link data and apply it to skeleton bones in UE.
void FAnimNode_ZEDLiveLinkPose::BuildPoseFromZEDAnimationData(
    float DeltaTime, // 自上次更新以来经过的时间
    const FLiveLinkSkeletonStaticData* InSkeletonData, // 包含骨骼的静态数据，如骨骼名称和父骨骼索引
    const FLiveLinkAnimationFrameData* InFrameData, // 当前采集到的动作数据
    FCompactPose& OutPose  // 用于输出最终骨骼姿势的对象
    )
{
    const TArray<FName>& SourceBoneNames = InSkeletonData->BoneNames;  // 骨骼名称
    const TArray<int32>& SourceBoneParents = InSkeletonData->BoneParents;  // 父索引数组

    TArray<FName, TMemStackAllocator<>> TransformedBoneNames;
    TransformedBoneNames.Reserve(SourceBoneNames.Num());


    if (NbKeypoints < 0) // only the first time. 类初始化时，设置为了-1，所以只运行一次
    {
        // Check the size of the input data to know which body format is used.
        // 根据livelink的数据帧，选择骨架
        if (InFrameData->Transforms.Num() == Keypoints38.Num() * 2) // body_38
        {
            NbKeypoints = 38;
            Keypoints = Keypoints38;
            ParentsIdx = Parents38Idx;
            CurBoneNameMap = &BoneNameMap38;
        }
        else if (InFrameData->Transforms.Num() == Keypoints34.Num() * 2)// BODY_34
        {
            NbKeypoints = 34;
            Keypoints = Keypoints34;
            ParentsIdx = Parents34Idx;
            CurBoneNameMap = &BoneNameMap34;
        }
        else
        {
            NbKeypoints = 38;
            Keypoints = Keypoints38;
            ParentsIdx = Parents38Idx;
        }
    }

    // Find remapped bone names and cache them for fast subsequent retrieval.
    for (const FName& SrcBoneName : SourceBoneNames)
    {
        if (!SrcBoneName.ToString().ToLower().Contains("conf") && !Keypoints.FindKey(SrcBoneName))
        {
            UE_LOG(LogTemp, Fatal, TEXT("Bone names mismatch between remap asset and live link sender. %s"), *SrcBoneName.ToString());
        }

        FName* TargetBoneName = CurBoneNameMap->Find(SrcBoneName);
        if (!SrcBoneName.ToString().ToLower().Contains("conf") && TargetBoneName == nullptr)
        {
            UE_LOG(LogTemp, Fatal, TEXT("Error in remap asset. Do not find remapped name of %s"), *SrcBoneName.ToString());
        }
        else if (TargetBoneName)
        {
            TransformedBoneNames.Add(*TargetBoneName);
        }
        else
            TransformedBoneNames.Add("");

    }

    // Apply an offset to put the feet of the ground and offset "floating" avatars.
    /* 如果启用了bStickAvatarOnFloor选项，计算角色 脚部到地面的距离
    */
    if (bStickAvatarOnFloor && InFrameData->Transforms[NbKeypoints / 2 + *Keypoints.FindKey(FName("LEFT_ANKLE"))].GetLocation().X > 0 && InFrameData->Transforms[NbKeypoints / 2 + *Keypoints.FindKey(FName("RIGHT_ANKLE"))].GetLocation().X > 0) { //if both foot are visible/detected
        if (SkeletalMesh) {

            FVector LeftFootPosition;
            FVector RightFootPosition;

            if (Keypoints.Num() == 34) // body 34
            {
                LeftFootPosition = SkeletalMesh->GetBoneLocation(TransformedBoneNames[21]);
                RightFootPosition = SkeletalMesh->GetBoneLocation(TransformedBoneNames[25]);
            }
            else // body 38
            {
                LeftFootPosition = SkeletalMesh->GetBoneLocation(TransformedBoneNames[24]);
                RightFootPosition = SkeletalMesh->GetBoneLocation(TransformedBoneNames[25]);
            }

            FHitResult HitLeftFoot;
            bool RaycastLeftFoot = SkeletalMesh->GetWorld()->LineTraceSingleByObjectType(OUT HitLeftFoot, LeftFootPosition + FVector(0, 0, 200), LeftFootPosition - FVector(0, 0, 200),
                FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic));

            FHitResult HitRightFoot;
            bool RaycastRightFoot = SkeletalMesh->GetWorld()->LineTraceSingleByObjectType(OUT HitRightFoot, RightFootPosition + FVector(0, 0, 200), RightFootPosition - FVector(0, 0, 200),
                FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic));

            float LeftFootFloorDistance = 0;
            float RightFootFloorDistance = 0;

            // Compute the distance between one foot and the ground (the first static object found by the ray cast).
            if (RaycastLeftFoot)
            {
                LeftFootFloorDistance = (LeftFootPosition + FVector(0, 0, AutomaticHeightOffset) - HitLeftFoot.ImpactPoint).Z;
            }

            if (RaycastRightFoot)
            {
                RightFootFloorDistance = (RightFootPosition + FVector(0, 0, AutomaticHeightOffset) - HitRightFoot.ImpactPoint).Z;
            }

            if (abs(fminf(LeftFootFloorDistance, RightFootFloorDistance)) <= DistanceToFloorThreshold)
            {
                // Reset counter 
                DurationOffsetError = 0;
            }
            else
            {
                auto NowTS_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                DurationOffsetError += (NowTS_ms - PreviousTS_ms) / 1000.0f;
                PreviousTS_ms = NowTS_ms;

                if (DurationOffsetError > DurationOffsetErrorThreshold)
                {
                    AutomaticHeightOffset = fmin(LeftFootFloorDistance, RightFootFloorDistance);
                    DurationOffsetError = 0;

                    //UE_LOG(LogTemp, Warning, TEXT("Recomputing offset ... %f"), AutomaticHeightOffset);
                }
            }
        }
    }
    else
    {
        AutomaticHeightOffset = 0;
    }

    // 将骨骼 设置成 参考姿势
    PutInRefPose(OutPose, TransformedBoneNames);
    
    // 读取root骨骼，如果root有效，则对上面的子-joint应用 pose旋转和偏移
    FCompactPoseBoneIndex CPIndexRoot = GetCPIndex(0, OutPose, TransformedBoneNames);
    if (CPIndexRoot != INDEX_NONE)
        PropagateRestPoseRotations(0, OutPose, TransformedBoneNames, SourceBoneParents, OutPose.GetRefPose(CPIndexRoot).GetRotation(), false);

    // Iterate over remapped bone names, find the index of that bone on the skeleton, and apply the Live Link pose data.
    // 从root-joint开始，应用正向运动学，扭转子-joint的位姿
    for (int32 i = 0; i < TransformedBoneNames.Num(); i++)
    {
        FName BoneName = TransformedBoneNames[i];

        if (!BoneName.ToString().ToLower().Contains("conf")) { // ignore kp confidence stored as kp
            FTransform BoneTransform = InFrameData->Transforms[i];
            FCompactPoseBoneIndex CPIndex = GetCPIndex(i, OutPose, TransformedBoneNames);
            if (CPIndex != INDEX_NONE)
            {
                FQuat Rotation;
                FVector Translation;

                // Only use position + rotation data for root. For all other bones, set rotation only.
                if (BoneName == (*CurBoneNameMap)[GetTargetRootName()]) // 如果是root-joint
                {   // 骨骼是根骨骼，则计算位置和旋转，并应用缩放因子
                    float rootScaleFactor = ComputeRootTranslationFactor(OutPose, TransformedBoneNames, InFrameData);

                    FVector RootPosition = BoneTransform.GetTranslation();
                    FCompactPoseBoneIndex leftUpLegIndex = GetCPIndex(*Keypoints.FindKey(FName("LEFT_HIP")), OutPose, TransformedBoneNames);
                    float HipOffset = FMath::Abs(OutPose[leftUpLegIndex].GetTranslation().Z) * OutPose[CPIndexRoot].GetScale3D().Z;

                    RootPosition.Z += HipOffset; // The position of the root in UE and in the SDK are slightly different. This offset compensates it.
                    RootPosition.Z += ManualHeightOffset;
                    RootPosition.Z -= AutomaticHeightOffset;
                    //RootPosition.Z *= rootScaleFactor;

                    Translation = RootPosition;

                    Rotation = BoneTransform.GetRotation();
                }
                else  
                {
                    // 非根骨骼，只设置旋转
                    Rotation = BoneTransform.GetRotation();
                    Translation = OutPose[CPIndex].GetTranslation();
                }

                // Retrieves the default reference pose for the skeleton. Live Link data contains relative transforms from the default pose.
                FQuat FinalRotation = Rotation * OutPose[CPIndex].GetRotation();
                OutPose[CPIndex].SetRotation(FinalRotation);
                OutPose[CPIndex].SetTranslation(Translation);
            }
        }
    }

    if (CPIndexRoot != INDEX_NONE)
        PropagateRestPoseRotations(0, OutPose, TransformedBoneNames, SourceBoneParents, OutPose.GetRefPose(CPIndexRoot).GetRotation().Inverse(), true);
}

void FAnimNode_ZEDLiveLinkPose::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	Super::OnInitializeAnimInstance(InProxy, InAnimInstance);
}

void FAnimNode_ZEDLiveLinkPose::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	InputPose.Initialize(Context);

    PreviousTS_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (bStickAvatarOnFloor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Automatic offset of avatar and Foot IK can not be enabled together. FootIK will be ignored"));
    }
}

void FAnimNode_ZEDLiveLinkPose::PreUpdate(const UAnimInstance* InAnimInstance)
{
    ILiveLinkClient* ThisFrameClient = nullptr;
    IModularFeatures& ModularFeatures = IModularFeatures::Get();
    if (ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
    {
        ThisFrameClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
    }
    LiveLinkClient_AnyThread = ThisFrameClient;
}

void FAnimNode_ZEDLiveLinkPose::Update_AnyThread(const FAnimationUpdateContext& Context)
{
    InputPose.Update(Context);

    GetEvaluateGraphExposedInputs().Execute(Context);

    // Accumulate Delta time from update
    CachedDeltaTime += Context.GetDeltaTime();

    TRACE_ANIM_NODE_VALUE(Context, TEXT("SubjectName"), LiveLinkSubjectName.Name);
}

void FAnimNode_ZEDLiveLinkPose::Evaluate_AnyThread(FPoseContext& Output)
{
    InputPose.Evaluate(Output);

    if (!LiveLinkClient_AnyThread)
    {
        return;
    }

    FLiveLinkSubjectFrameData SubjectFrameData;
    TSubclassOf<ULiveLinkRole> SubjectRole = LiveLinkClient_AnyThread->GetSubjectRole_AnyThread(LiveLinkSubjectName);

    if (SubjectRole)
    {
        if (LiveLinkClient_AnyThread->DoesSubjectSupportsRole_AnyThread(LiveLinkSubjectName, ULiveLinkAnimationRole::StaticClass()))
        {
            //Process animation data if the subject is from that type
            if (LiveLinkClient_AnyThread->EvaluateFrame_AnyThread(LiveLinkSubjectName, ULiveLinkAnimationRole::StaticClass(), SubjectFrameData))
            {
                FLiveLinkSkeletonStaticData* SkeletonData = SubjectFrameData.StaticData.Cast<FLiveLinkSkeletonStaticData>();
                FLiveLinkAnimationFrameData* FrameData = SubjectFrameData.FrameData.Cast<FLiveLinkAnimationFrameData>();
                check(SkeletonData);
                check(FrameData);

                BuildPoseFromZEDAnimationData(CachedDeltaTime, SkeletonData, FrameData, Output.Pose);

                CachedDeltaTime = 0.f; // Reset so that if we evaluate again we don't "create" time inside of the retargeter
            }
        }
    }
}


void FAnimNode_ZEDLiveLinkPose::CacheBones_AnyThread(const FAnimationCacheBonesContext & Context)
{
	Super::CacheBones_AnyThread(Context);

	InputPose.CacheBones(Context);
}

bool FAnimNode_ZEDLiveLinkPose::Serialize(FArchive& Ar)
{
    return false;
}
