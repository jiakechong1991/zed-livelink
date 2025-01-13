#include "ZEDLiveLinkSource.h"

#include <string>

#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "Roles/LiveLinkCameraRole.h"
#include "Roles/LiveLinkCameraTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "Roles/LiveLinkTransformRole.h"
#include "Roles/LiveLinkTransformTypes.h"

#include "Async/Async.h"
#include "Common/UdpSocketBuilder.h"
#include "HAL/RunnableThread.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Containers/UnrealString.h"
#include "Misc/Char.h"
#include "Containers/Array.h"

#define LOCTEXT_NAMESPACE "ZEDLiveLinkSource"

#define RECV_BUFFER_SIZE 0

FZEDLiveLinkSource::FZEDLiveLinkSource(const FZEDLiveLinkSettings& InSettings)
: Socket(nullptr)
, bIsRunning(false)
, Thread(nullptr)
, WaitTime(FTimespan::FromMilliseconds(500))
{
	// defaults
	ZEDSettings = InSettings;

	SourceStatus = LOCTEXT("SourceStatus_NoConnection", "No Connection");
	SourceType = LOCTEXT("ZEDLiveLinkSourceType", "ZED LiveLink");
	SourceMachineName = FText::FromString(ZEDSettings.Endpoint.ToString());

	//setup socket
	if (ZEDSettings.Endpoint.Address.IsMulticastAddress())
	{
		Socket = FUdpSocketBuilder(TEXT("ZEDSOCKET"))
			.AsNonBlocking()
			.AsReusable()
			.BoundToPort(ZEDSettings.Endpoint.Port)
			.WithReceiveBufferSize(RECV_BUFFER_SIZE)

			.BoundToAddress(FIPv4Address::Any)
			.JoinedToGroup(ZEDSettings.Endpoint.Address)
			.WithMulticastLoopback()
			.WithMulticastTtl(2);
		UE_LOG(LogTemp, Warning, TEXT("Connect111 to %s : %d"), *ZEDSettings.Endpoint.Address.ToString(), ZEDSettings.Endpoint.Port);

					
	}
	else
	{
		Socket = FUdpSocketBuilder(TEXT("ZEDSOCKET"))
			.AsNonBlocking()
			.AsReusable()
			.BoundToAddress(ZEDSettings.Endpoint.Address)
			.BoundToPort(ZEDSettings.Endpoint.Port)
			.WithReceiveBufferSize(RECV_BUFFER_SIZE);

			UE_LOG(LogTemp, Warning, TEXT("Connect222 to %s : %d"), *ZEDSettings.Endpoint.Address.ToString(), ZEDSettings.Endpoint.Port);
	}

	RecvBuffer.SetNumUninitialized(RECV_BUFFER_SIZE);

	if ((Socket != nullptr) && (Socket->GetSocketType() == SOCKTYPE_Datagram))
	{
		SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

		Start();

		//SourceStatus = LOCTEXT("SourceStatus_Connecting", "Connecting");
	}
}

// 析构函数
FZEDLiveLinkSource::~FZEDLiveLinkSource()
{
	Stop();
	if (Thread != nullptr)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
	if (Socket != nullptr)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
}

void FZEDLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	Client = InClient;
	SourceGuid = InSourceGuid;
}

// 判断source是否有效
bool FZEDLiveLinkSource::IsSourceStillValid() const
{
	// Source is valid if we have a valid thread and socket
	bool bIsSourceValid = bIsRunning && Thread != nullptr && Socket != nullptr;
	return bIsSourceValid;
}

// 主动关闭source
bool FZEDLiveLinkSource::RequestSourceShutdown()
{
	Stop();

	SourceStatus = LOCTEXT("SourceStatus_Disconnected", "Disconnected");

	return true;
}
// FRunnable interface

void FZEDLiveLinkSource::Start()
{
	bIsRunning = true;

	ThreadName = "ZED UDP Receiver ";  // 线程名称
	ThreadName.AppendInt(FAsyncThreadIndex::GetNext());
	
	// 创建一个线程，这个线程就是this指向的当前类,在新线程中，将调用该对象的Run方法（FRunnable对象的Run方法）
	// 这样，FZEDLiveLinkSource对象就可以在其自己的线程中执行，而不会阻塞主线程
	/*
	this：指向当前FZEDLiveLinkSource对象的指针，这个对象实现了FRunnable接口。
	*ThreadName：上面构造的线程名称。
	0：堆栈大小，这里设置为0，表示使用默认堆栈大小。
	TPri_AboveNormal：线程的优先级，这里设置为高于正常优先级
	*/
	Thread = FRunnableThread::Create(this, *ThreadName, 0, TPri_AboveNormal);
}

void FZEDLiveLinkSource::Stop()
{
	bIsRunning = false;
}

uint32 FZEDLiveLinkSource::Run()  // 在线程中，独立运行
{
	TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();
	while (bIsRunning)
	{
		
		if (Socket->Wait(ESocketWaitConditions::WaitForRead, WaitTime))  // 等待UDP数据(最大超时500ms)
		{
			FirstConnection = false;
			SourceStatus = LOCTEXT("SourceStatus_Connected", "Connected");
			uint32 Size;
			while (Socket->HasPendingData(Size))
			{
				int32 Read = 0;
				RecvBuffer.SetNumUninitialized(FMath::Min(Size, 65507u)); // max size of UDP packet
				// 从UDP中读取 指定长度的 数据
				bool recv = Socket->RecvFrom(RecvBuffer.GetData(), RecvBuffer.Num(), Read, *Sender);
				if (recv)
				{
					if (Read > 0)
					{
						TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> ReceivedData = MakeShareable(new TArray<uint8>());
						ReceivedData->SetNumUninitialized(Read);
						memcpy(ReceivedData->GetData(), RecvBuffer.GetData(), Read);
						// 在GameThread线程中，用ProcessReceivedData函数，对 接收到的数据进行处理
						AsyncTask(ENamedThreads::GameThread, [this, ReceivedData]() { 
							ProcessReceivedData(ReceivedData); 
							});
					}
				}
			}
		}
		else
		{
			if (!FirstConnection)
			{
				SourceStatus = LOCTEXT("SourceStatus_NoData", "No Data");
				ClearSubjects(); // clear subjects if no msg received for WaitTime (1sec).
			}
		}
	}
	return 0;
}

void FZEDLiveLinkSource::ProcessReceivedData(TSharedPtr<TArray<uint8>> ReceivedData)
{
	/// CONVERTNG TO STRING
	FString recvedString;  // 原始数据在这里
	int32 Read = ReceivedData->Num();
	recvedString.Empty(ReceivedData->Num());
	for (uint8& Byte : *ReceivedData.Get())
	{
		recvedString += TCHAR(Byte);
	}

	// 这一行，对接收到 livelink数据进行解析
	FrameData frameData = FrameData(recvedString);
	FName SubjectName = FName(*frameData.Subject);

	FLiveLinkFrameDataStruct FrameData;

	FLiveLinkSubjectKey Key = FLiveLinkSubjectKey(SourceGuid, SubjectName);
	if (Client && bIsRunning)
	{
		if (frameData.bIsValid) // 解析数据有效
		{
			if (CurrentTimeStamp - frameData.Timestamp > 0) // clean all subjects when current ts > new ts. It happens if a SVO loops for example.
			{
				ClearSubjects();
			}
			else if (frameData.SubjectRole == ULiveLinkAnimationRole::StaticClass() && frameData.BodyTrackingState != EZEDTrackingState::Ok)
			{ // 动作类型，但是已经追踪失败
				Client->RemoveSubject_AnyThread(Key);
				FScopeLock Lock(&SubjectsCriticalSection);
				{
					Subjects.Remove(SubjectName);
				}
			}
			else  // 正常追踪中
			{
				TArray<FLiveLinkSubjectKey> SubjectsKey;
				FScopeLock Lock(&SubjectsCriticalSection);
				{
					SubjectsKey = Client->GetSubjects(true, false);
				}

				if (!SubjectsKey.Contains(Key))
				{
					if (!Subjects.Contains(SubjectName))
					{
						FLiveLinkSubjectPreset Preset;
						Preset.Key = Key;
						Preset.Role = frameData.SubjectRole;
						//Preset.bEnabled = true;

						if (Client->GetSources().Num() > 0)
						{ // 创建角色
							FScopeLock LockClient(&SubjectsCriticalSection);
							{
								Client->CreateSubject(Preset);
								Client->SetSubjectEnabled(Key, true);

								Subjects.Push(SubjectName);
							}

							if (frameData.SubjectRole == ULiveLinkCameraRole::StaticClass())
							{
								UpdateCameraStaticData(SubjectName, frameData.CameraTransform);
							}
							else if (frameData.SubjectRole == ULiveLinkAnimationRole::StaticClass())
							{
								UpdateAnimationStaticData(SubjectName, frameData.ParentsIdx, frameData.TargetBones);
							}
						}
					}
				}

				if (frameData.SubjectRole == ULiveLinkCameraRole::StaticClass())
				{
					FrameData = FLiveLinkCameraFrameData::StaticStruct();
					FLiveLinkCameraFrameData& CameraData = *FrameData.Cast<FLiveLinkCameraFrameData>();

					CameraData.WorldTime = FPlatformTime::Seconds();
					CameraData.Transform = frameData.CameraTransform;

				}
				else if (frameData.SubjectRole == ULiveLinkAnimationRole::StaticClass())
				{
					FrameData = FLiveLinkAnimationFrameData::StaticStruct();
					FLiveLinkAnimationFrameData& AnimData = *FrameData.Cast<FLiveLinkAnimationFrameData>();

					// 使用动捕数据，更新到角色的transform
					AnimData.WorldTime = FPlatformTime::Seconds();
					AnimData.Transforms = frameData.BoneTransform;
				}

				Client->PushSubjectFrameData_AnyThread(Key, MoveTemp(FrameData));
			}
			CurrentTimeStamp = frameData.Timestamp;
		}
		else
		{
			SourceStatus = LOCTEXT("SourceStatus_DataError", "Data Error");

		}
	}
}

void FZEDLiveLinkSource::UpdateCameraStaticData(FName SubjectName, FTransform CameraPose)
{
	FLiveLinkSubjectKey Key = FLiveLinkSubjectKey(SourceGuid, SubjectName);
	FLiveLinkStaticDataStruct StaticData(FLiveLinkCameraStaticData::StaticStruct());
	FLiveLinkCameraStaticData& CameraData = *StaticData.Cast<FLiveLinkCameraStaticData>();

	CameraData.bIsAspectRatioSupported = false;
	CameraData.bIsFieldOfViewSupported = false;
	CameraData.bIsFocalLengthSupported = false;
	CameraData.bIsFocusDistanceSupported = false;
	CameraData.bIsProjectionModeSupported = false;

	Client->PushSubjectStaticData_AnyThread(Key, ULiveLinkCameraRole::StaticClass(), MoveTemp(StaticData));
}

void FZEDLiveLinkSource::UpdateAnimationStaticData(FName SubjectName, TArray<int>ParentsIdx, TArray<FName> TargetBones)
{
	FLiveLinkSubjectKey Key = FLiveLinkSubjectKey(SourceGuid, SubjectName);
	FLiveLinkStaticDataStruct StaticData(FLiveLinkSkeletonStaticData::StaticStruct());
	FLiveLinkSkeletonStaticData* SkeletonData = StaticData.Cast<FLiveLinkSkeletonStaticData>();
	SkeletonData->SetBoneNames(TargetBones);
	SkeletonData->SetBoneParents(ParentsIdx);
	Client->PushSubjectStaticData_AnyThread(Key, ULiveLinkAnimationRole::StaticClass(), MoveTemp(StaticData));
}

void FZEDLiveLinkSource::ClearSubjects()
{
	FScopeLock Lock(&SubjectsCriticalSection);
	{
		while (Subjects.Num() > 0)
		{
			auto Subject = Subjects.Pop();
			FLiveLinkSubjectKey KeyToRemove = FLiveLinkSubjectKey(SourceGuid, Subject);
			Client->RemoveSubject_AnyThread(KeyToRemove);
		}
	}
}

#undef LOCTEXT_NAMESPACE
