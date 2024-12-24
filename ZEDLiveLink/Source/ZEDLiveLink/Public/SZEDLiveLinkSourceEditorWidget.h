#pragma once

#include <Widgets/SCompoundWidget.h>  // UE控件组件，用于自定义UI

// UE UI组件
#include "Widgets/Input/SCheckBox.h" 
#include "Widgets/Input/SEditableTextBox.h"
#include "ZEDLiveLinkSettings.h"

// 声明了一个带有单个参数的委托FZEDLiveLinkOnSourceCreated，当LiveLink源创建时会触发这个委托，
// 并传递FZEDLiveLinkSettings对象作为参数
DECLARE_DELEGATE_OneParam(FZEDLiveLinkOnSourceCreated, FZEDLiveLinkSettings);

/*
该类用于 在UE中自定义编辑器界面，用于创建和设置 ZED livelink源
*/

class SZEDLiveLinkSourceEditorWidget : public SCompoundWidget  // 继承自SCompoundWidget，用于创建自定义UI控件
{
public:

	SLATE_BEGIN_ARGS(SZEDLiveLinkSourceEditorWidget){}
	SLATE_EVENT(FZEDLiveLinkOnSourceCreated, OnSourceCreated)
SLATE_END_ARGS()

void Construct(const FArguments& Args); //构造函数

private:

	TSharedPtr<FString> GetConnectionTypeString() const; // 获取当前连接类型的字符串表示
	// 当连接类型改变时调用，更新控件的状态
	void OnConnectionTypeChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo);
	// 当用户点击 创建按钮 时调用，执行创建源的操作
	FReply OnCreateClicked() const;

	TSharedPtr<SEditableTextBox> IpAddress;
	EZEDLiveLinkConnectionType ConnectionType = EZEDLiveLinkConnectionType::Multicast;
	TArray<TSharedPtr <FString>> ConnectionTypeStrings;

	FZEDLiveLinkOnSourceCreated OnSourceCreated;
};