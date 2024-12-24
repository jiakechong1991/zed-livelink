#pragma once

#include "CoreMinimal.h"
#include <ILiveLinkSource.h>
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "IMessageContext.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "ZEDLiveLinkSettings.h"
#include "FrameData.h"


class FRunnableThread;
class FSocket;
class ILiveLinkClient;
class ISocketSubsystem;


class ZEDLIVELINK_API FZEDLiveLinkSource : public ILiveLinkSource, public FRunnable
{
public:
	FZEDLiveLinkSource(const FZEDLiveLinkSettings& InSettings);

	virtual ~FZEDLiveLinkSource();  // 析构函数

	// Begin ILiveLinkSource Interface[接口]
	
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;

	virtual bool IsSourceStillValid() const override;

	virtual bool RequestSourceShutdown() override;

	virtual FText GetSourceType() const override { return SourceType; };
	virtual FText GetSourceMachineName() const override { return SourceMachineName; }
	virtual FText GetSourceStatus() const override { return SourceStatus; }

	// End ILiveLinkSource Interface

	// Begin FRunnable Interface[接口]

	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	void Start();
	virtual void Stop() override;
	virtual void Exit() override { }

	// End FRunnable Interface

	// 处理接收的数据
	void ProcessReceivedData(TSharedPtr<TArray<uint8>> ReceivedData);

private:

	ILiveLinkClient* Client; //指向LiveLink客户端的指针

	// Our identifier in LiveLink
	FGuid SourceGuid; //源的唯一标识符

	FMessageAddress ConnectionAddress; //连接地址

	// 源的类型、机器名称和状态
	FText SourceType; 
	FText SourceMachineName;
	FText SourceStatus;  // Disconnected

	FZEDLiveLinkSettings ZEDSettings;
	// Socket to receive data on
	FSocket* Socket;

	// Subsystem associated to Socket
	ISocketSubsystem* SocketSubsystem;

	// Threadsafe Bool for terminating the main thread loop
	FThreadSafeBool bIsRunning;

	// Thread to run socket operations on
	FRunnableThread* Thread;

	// Name of the sockets thread
	FString ThreadName;

	// Time to wait between attempted receives 尝试接收之间的等待时间
	FTimespan WaitTime;  //超过该时间将断开

	// List of subject[角色]s we've already encountered
	TArray<FName> Subjects;

	// Buffer to receive socket data into
	TArray<uint8> RecvBuffer; // 接收套接字数据的缓冲区

	mutable FCriticalSection SubjectsCriticalSection;

	bool FirstConnection = true;  // 是否首次连接
	// Check if static data is setup

	// current timestamp
	double CurrentTimeStamp = 0;

	void UpdateAnimationStaticData(FName SubjectName, TArray<int>ParentsIdx, TArray<FName> TargetBones);

	void UpdateCameraStaticData(FName SubjectName, FTransform CameraPose);

	void ClearSubjects();
};
