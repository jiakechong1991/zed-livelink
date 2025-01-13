#include "FrameData.h"
#include "ZEDSkeletonRetarget.h"

FrameData::FrameData(FString frameData)
{
    // Create a reader for the JSON data
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(frameData);

	// Deserialize the JSON data
	Deserialize(Reader);
}

void FrameData::Deserialize(TSharedRef<TJsonReader<>> Reader)
{
    // Create a variable to store the parsed JSON data
    TSharedPtr<FJsonObject> JsonObject;  // 解析后的就保存在这个json中

    bIsValid = false;
    Timestamp = 0;
    UE_LOG(LogTemp, Warning, TEXT("i will parse data!!!"));
    UE_LOG(LogTemp, Warning, TEXT("即将解析从UDP读取到的动捕数据!!!"));
    // Try to deserialize the JSON data
    // 这里应该就是解析UDP收到 动捕消息 的位置， 解析动捕消息
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // Successfully parsed the JSON data

        int role = -1;
        if (!JsonObject->TryGetNumberField(FString("role"), role))
        {
            bIsValid = false;
            return;
        }

        if (!JsonObject->TryGetNumberField(FString("timestamp"), Timestamp))
        {
            bIsValid = false;
            return;
        }

        if (!JsonObject->TryGetNumberField(FString("frame_id"), FrameID))
        {
            bIsValid = false;
            return;
        }

        int value = 0;
        if (!JsonObject->TryGetNumberField(FString("coordinate_system"), value))
        {
            bIsValid = false;
            return;
        }
        CoordinateSystem = (EZEDCoordinateSystem)value;

        if (!JsonObject->TryGetNumberField(FString("coordinate_unit"), value))
        {
            bIsValid = false;
            return;
        }
        CoordinateUnit = (EZEDCoordinateUnit)value;

        // camera类型 数据
        if (role == (int)EZEDLiveLinkRole::Camera)
        {
            int64 SerialNumber = -1;
            if (!JsonObject->TryGetNumberField(FString("serial_number"), SerialNumber))
            {
                bIsValid = false;
                return;
            }

            Subject = "SN" + FString::FromInt(SerialNumber);
            SubjectRole = ULiveLinkCameraRole::StaticClass();

            // 解析camera_position数据
            const TSharedPtr<FJsonObject>* CameraPositionArray;
            if (!JsonObject->TryGetObjectField(FString("camera_position"), CameraPositionArray))
            {
                bIsValid = false;
                return;
            }
            FVector CameraPosition = ConvertCoordinateUnitToUE(CoordinateUnit, FVector(CameraPositionArray->Get()->GetNumberField(FString("x")), CameraPositionArray->Get()->GetNumberField(FString("y")), CameraPositionArray->Get()->GetNumberField(FString("z"))));

            // 解析camera_orientation数据
            const TSharedPtr<FJsonObject>* CameraOrientationArray;
            if (!JsonObject->TryGetObjectField(FString("camera_orientation"), CameraOrientationArray))
            {
                bIsValid = false;
                return;
            }
            // 四元数形式
            FQuat CameraOrientation = FQuat(CameraOrientationArray->Get()->GetNumberField(FString("x")), CameraOrientationArray->Get()->GetNumberField(FString("y")),
                CameraOrientationArray->Get()->GetNumberField(FString("z")), CameraOrientationArray->Get()->GetNumberField(FString("w")));

            CameraTransform.SetLocation(CameraPosition);
            CameraTransform.SetRotation(CameraOrientation);

            // 获得camera的位姿数据，并赋值类的属性变量
            CameraTransform = ConvertCoordinateSystemToUE(CoordinateSystem, CameraTransform);

            bIsValid = true;
        }
        else if (role == (int)EZEDLiveLinkRole::Animation)  // 动画数据类型
        {
            int DetectionID = -1;

            if (!JsonObject->TryGetNumberField(FString("id"), DetectionID))
            {
                bIsValid = false;
                return;
            }

            Subject = FString::FromInt(DetectionID);
            SubjectRole = ULiveLinkAnimationRole::StaticClass();

            EZEDBodyFormat ZEDBodyFormat = EZEDBodyFormat::Body_38;

            if (!JsonObject->TryGetNumberField(FString("body_format"), value))
            {
                bIsValid = false;
                return;
            }
            ZEDBodyFormat = (EZEDBodyFormat)value;

            if (!JsonObject->TryGetNumberField(FString("tracking_state"), value))
            {
                bIsValid = false;
                return;
            }
            BodyTrackingState = (EZEDTrackingState)value;


            if (ZEDBodyFormat == EZEDBodyFormat::Body_34)// BODY_34
            {
                NbKeypoints = 34;
                Keypoints = Keypoints34;
                ParentsIdx = Parents34Idx;
                TargetBones = TargetBone34;
            }
            else
            {
                NbKeypoints = 38;
                Keypoints = Keypoints38;
                ParentsIdx = Parents38Idx;
                TargetBones = TargetBone38; 
            }

            // 解析 Root transform of the skeleton
            auto rootPositionJsonValue = JsonObject->GetObjectField(FString("global_root_posititon"));
            // 缩放了0.1
            FVector rootPosition = ConvertCoordinateUnitToUE(CoordinateUnit, FVector(rootPositionJsonValue->GetNumberField(FString("x")), rootPositionJsonValue->GetNumberField(FString("y")), rootPositionJsonValue->GetNumberField(FString("z"))));

            auto rootOrientationJsonValue = JsonObject->GetObjectField(FString("global_root_orientation"));
            FQuat rootOrientation = FQuat(
                rootOrientationJsonValue->GetNumberField(FString("x")), 
                rootOrientationJsonValue->GetNumberField(FString("y")), 
                rootOrientationJsonValue->GetNumberField(FString("z")),
                rootOrientationJsonValue->GetNumberField(FString("w"))
            );

            if (rootPosition.ContainsNaN())
            {
                rootPosition = FVector::ZeroVector;
                UE_LOG(LogTemp, Warning, TEXT("--bbb-我到这里了吗？11"));
            }
            else{
                UE_LOG(LogTemp, Warning, TEXT("--bbb-打印一下: %s"), *rootPosition.ToString());
            }
            if (rootOrientation.ContainsNaN())
            {
                rootOrientation = FQuat::Identity;
            }
            // 解析骨骼的local 姿态:  位置 和朝向
            TArray< TSharedPtr<FJsonValue>> LocalPositions = JsonObject->GetArrayField(
                FString("local_position_per_joint"));
            TArray< TSharedPtr<FJsonValue>> LocalOrientations = JsonObject->GetArrayField(
                FString("local_orientation_per_joint"));

            FTransform RootTransform;
            rootOrientation.Normalize();
            RootTransform.SetRotation(rootOrientation);
            RootTransform.SetLocation(rootPosition);
            
            // 将root骨骼点的位姿推到 boneTransform中
            RootTransform = ConvertCoordinateSystemToUE(CoordinateSystem, RootTransform);
            BoneTransform.Push(RootTransform); // 0-joint
            UE_LOG(LogTemp, Warning, TEXT("--bbb---%s"), *RootTransform.ToString());

            // Local position and rotation of each keypoint
            // NbKeypoints=38（或者34），所以这里是1～37个joint (0是前面的root)
            for (int i = 1; i < NbKeypoints; i++)  // 逐个关节，把他们的位姿 push到 BoneTransform中
            {
                FQuat Orientation = FQuat(
                    LocalOrientations[i]->AsObject()->GetNumberField(FString("x")), 
                    LocalOrientations[i]->AsObject()->GetNumberField(FString("y")), 
                    LocalOrientations[i]->AsObject()->GetNumberField(FString("z")),
                    LocalOrientations[i]->AsObject()->GetNumberField(FString("w")));
                
                FVector Position = ConvertCoordinateUnitToUE(
                    CoordinateUnit, 
                    FVector(LocalPositions[i]->AsObject()->GetNumberField(FString("x")), 
                    LocalPositions[i]->AsObject()->GetNumberField(FString("y")),
                    LocalPositions[i]->AsObject()->GetNumberField(FString("z"))));

                if (Position.ContainsNaN())
                {
                    Position = FVector::ZeroVector;
                }
                if (Orientation.ContainsNaN())
                {
                    Orientation = FQuat::Identity;
                }

                FTransform Transform;
                Orientation.Normalize();  // 执行标准化
                Transform.SetRotation(Orientation);
                Transform.SetLocation(Position);

                UE_LOG(LogTemp, Warning, TEXT("--bbb-%d---%s"), i,*Transform.ToString());
                Transform = ConvertCoordinateSystemToUE(CoordinateSystem, Transform);

                BoneTransform.Push(Transform);
            }

            // 将每个追踪点的 置信度读取到
            TArray< TSharedPtr<FJsonValue>> KeypointConfidence = JsonObject->GetArrayField(FString("keypoint_confidence"));

            // NbKeypoints=38（或者34），所以这里是0～37，0是root
            for (int i = 0; i < NbKeypoints; i++)
            {
                float Confidence = KeypointConfidence[i]->AsNumber();

                FVector VecConf(Confidence, Confidence, Confidence);
                FTransform TfConf;
                TfConf.SetLocation(VecConf);
                BoneTransform.Push(TfConf);
            }
            bIsValid = true;
        }
        else
        {
            bIsValid = false;
        }
    }
    else
    {
        // Failed to parse JSON data
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON data"));
        bIsValid = false;
    }
}

void FrameData::GetCoordinateTransform(EZEDCoordinateSystem coord_system, FMatrix& coordinateMatrix) {
    coordinateMatrix.SetIdentity();

    // set desired coordinate system
    switch (coord_system) {
    case EZEDCoordinateSystem::Image:
        coordinateMatrix.SetIdentity();
        break;
    case EZEDCoordinateSystem::Left_Handed_Y_Up:
        coordinateMatrix.M[1][1] = -1;
        break;
    case EZEDCoordinateSystem::Right_Handed_Y_Up:
        coordinateMatrix.M[1][1] = -1;
        coordinateMatrix.M[2][2] = -1;
        break;
    case EZEDCoordinateSystem::Right_Handed_Z_Up:
        coordinateMatrix.M[1][1] = 0;
        coordinateMatrix.M[1][2] = -1;
        coordinateMatrix.M[2][1] = 1;
        coordinateMatrix.M[2][2] = 0;
        break;
    case EZEDCoordinateSystem::Right_Handed_Z_Up_X_Fwd:
        coordinateMatrix.M[0][0] = 0;
        coordinateMatrix.M[1][1] = 0;
        coordinateMatrix.M[2][2] = 0;
        coordinateMatrix.M[1][2] = -1;
        coordinateMatrix.M[0][1] = -1;
        coordinateMatrix.M[2][0] = 1;

        break;
    case EZEDCoordinateSystem::Left_Handed_Z_Up:
        coordinateMatrix.M[0][0] = 0;
        coordinateMatrix.M[0][1] = 1;
        coordinateMatrix.M[1][0] = 0;
        coordinateMatrix.M[1][1] = 0;
        coordinateMatrix.M[1][2] = -1;
        coordinateMatrix.M[2][0] = 1;
        coordinateMatrix.M[2][1] = 0;
        coordinateMatrix.M[2][2] = 0;
        break;
    default:
        break;
    }
}

//Convert Transform to UE coordinate frame (Left Z up)
FTransform FrameData::ConvertCoordinateSystemToUE(EZEDCoordinateSystem InFrame, FTransform InTransform)
{
    FMatrix tmp;
    tmp.SetIdentity();

    FMatrix coordTransf;
    coordTransf.SetIdentity();

    GetCoordinateTransform(InFrame, tmp); //src 
    GetCoordinateTransform(EZEDCoordinateSystem::Left_Handed_Z_Up, coordTransf); // dst is unreal coordinate system

    FMatrix mat;
    mat.SetIdentity();
    mat = (coordTransf.Inverse() * tmp);
    FMatrix mat_out = mat * InTransform.ToMatrixWithScale() * mat.Inverse();

    FTransform out;
    out.SetFromMatrix(mat_out);
    return out;
}

//Convert vector to UE coordinate unit (centimeter)
// 这是单位缩放（比如输入值是1，这个1单位是什么呢？）
FVector FrameData::ConvertCoordinateUnitToUE(EZEDCoordinateUnit InUnit, FVector InVector)
{
    float factor = 1;

    switch (InUnit)
    {
        case EZEDCoordinateUnit::Meter:
            factor = 100;
            break;
        case EZEDCoordinateUnit::Centimeter:
            factor = 1;
            break;
        case EZEDCoordinateUnit::Millimeter: // 将输入值 乘以0.1,相当于缩小 变为原来的1/10
            factor = 0.1f;
            break;
        case EZEDCoordinateUnit::Inch: // 放大为了 数值值的 2.54倍
            factor = 2.54f;
            break;
        case EZEDCoordinateUnit::Foot:
            factor = 30.48f;
            break;
        default:
            factor = 0.1f;
    }
    return InVector * factor;
}
