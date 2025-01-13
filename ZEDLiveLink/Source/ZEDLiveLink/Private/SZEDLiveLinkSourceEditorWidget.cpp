#include "SZEDLiveLinkSourceEditorWidget.h"
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Input/STextComboBox.h>

#define LOCTEXT_NAMESPACE "ZEDLiveLinkSourceEditor"

// 构造函数：  使用SLATE框架创建UI界面，并初始化控件的状态
void SZEDLiveLinkSourceEditorWidget::Construct(const FArguments& Args)
{
	OnSourceCreated = Args._OnSourceCreated; // 从构造参数中获取OnSourceCreated委托

	const float kRowPadding = 3;
	const float kLabelColMinWidth = 125;
	const float kEditColMinWidth = 125;

	ConnectionTypeStrings.Add(MakeShareable<FString>(new FString("Multicast")));
	ConnectionTypeStrings.Add(MakeShareable<FString>(new FString("Unicast")));

	// 使用SLATE的布局系统创建一个垂直布局的盒子（SVerticalBox）
	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.Padding(kRowPadding)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("IPAddressLabel", "IP Address"))
		.MinDesiredWidth(kLabelColMinWidth)
		]
	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		[
			SAssignNew(IpAddress, SEditableTextBox) // 创建一个可编辑的文本框（SEditableTextBox
			.Text(FText::FromString("192.168.1.13:3001")) // 设置默认值为"230.0.0.1:2000"
		.MinDesiredWidth(kEditColMinWidth)
		]
		]
	+ SVerticalBox::Slot()
		.Padding(kRowPadding)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ConnectionTypeLabel", "Connection Type"))
		.MinDesiredWidth(kLabelColMinWidth)
		]
	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		[
			SNew(STextComboBox)
			.OptionsSource(&ConnectionTypeStrings)
		.InitiallySelectedItem(GetConnectionTypeString())
		.OnSelectionChanged(this, &SZEDLiveLinkSourceEditorWidget::OnConnectionTypeChanged)
		]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(1)
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("CreateButtonLabel", "Create Source"))
		.HAlign(HAlign_Center)
		.OnClicked(this, &SZEDLiveLinkSourceEditorWidget::OnCreateClicked)
		]
	+ SHorizontalBox::Slot()
		.FillWidth(1)
		]
		];
}

TSharedPtr<FString> SZEDLiveLinkSourceEditorWidget::GetConnectionTypeString() const
{
	switch (ConnectionType)
	{
	case EZEDLiveLinkConnectionType::Unicast:
		return ConnectionTypeStrings[1];
	case EZEDLiveLinkConnectionType::Multicast:
	default:
		return ConnectionTypeStrings[0];
	}
}

void SZEDLiveLinkSourceEditorWidget::OnConnectionTypeChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
{
	if (Value->Equals("Multicast"))
		ConnectionType = EZEDLiveLinkConnectionType::Multicast;
	else if (Value->Equals("Unicast"))
		ConnectionType = EZEDLiveLinkConnectionType::Unicast;
}

/*点击创建按钮*/
FReply SZEDLiveLinkSourceEditorWidget::OnCreateClicked() const
{
	FZEDLiveLinkSettings Settings;
	// 将IP-port解析后，存储在 setttings.endpoint中
    FIPv4Endpoint::Parse(IpAddress.Get()->GetText().ToString(), Settings.Endpoint);
	Settings.ConnectionType = ConnectionType;

	// 检查 OnSourceCreated委托是否有绑定的回调函数（即是否有其他组件订阅了这个委托）
	// 如果有，则执行这些回调函数，并将Settings对象作为参数传递
	// 这是Unreal Engine中委托机制的一个典型用法，用于实现组件间的松耦合通信
	OnSourceCreated.ExecuteIfBound(Settings);

	return FReply::Handled(); // 表示 按钮点击事件 已经被处理。这可以阻止事件继续冒泡，也可以让UI系统知道不需要进行进一步的处理
}


#undef LOCTEXT_NAMESPACE