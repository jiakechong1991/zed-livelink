#pragma once

#include "AnimGraphNode_Base.h"
#include "AnimNode_ZEDLiveLinkPose.h"
#include "AnimGraphDefinitions.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "AnimGraphNode_ZEDLiveLinkPose.generated.h"  // UHT 自动生成的头文件

/*
class ZEDLIVELINKEDITOR_API[宏，定义api范围] UAnimGraphNode_ZEDLiveLinkPose[类名称] : public UAnimGraphNode_Base[父类]
*/ 
UCLASS(BlueprintType)
class ZEDLIVELINKEDITOR_API UAnimGraphNode_ZEDLiveLinkPose : public UAnimGraphNode_Base
{
	GENERATED_BODY()  // 用于生成UE需要的反射信息

public:

	//~ UEdGraphNode interface
	// 虚函数，用于获取节点标题，会被该类重写
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override; 
	// 虚函数，用于获取节点描述，会被该类重写
	virtual FText GetTooltipText() const override;
	// 虚函数，用于 获取节点在蓝图编辑器菜单中的分类 
	virtual FText GetMenuCategory() const;
	//~ End of UEdGraphNode

public:

	// 定义一个属性面板上的属性
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_ZEDLiveLinkPose Node;

};
