#pragma once
#include "pch.hpp"

// 场景类型枚举
enum class E_SCENARIO_TYPE {
	UKN,		// 0 - 不明
	DEFENSE,	// 1 - 防御类
	ATTACK,		// 2 - 进攻类
	ESCORT,		// 3 - 护航类
	STRIKE,		// 4 - 打击类
};

// 图结构节点枚举
enum class E_NODE_TYPE {
	UKN,	//  0 - 不明
	MIS,	//  1 - Mission
	TSK,	//  2 - Task
	ACT,	//  3 - Action
	OBJ,	//  4 - Objective
	COG,	//  5 - Center of Gravity
	DEC,	//  6 - Decisive Condition
	EFT,	//  7 - Effect
	LOO,	//  8 - Line of Operation
	COA,	//  9 - Course of Action
	PLT,	// 10 - Platform
};

// 图结构边枚举
enum class E_EDGE_TYPE {
	UKN,		// 0 - 不明
	MIS_TO_COG,	// 1->N
	MIS_TO_OBJ,	// 1->N
	OBJ_TO_LOO,	// 1->1
	LOO_TO_COA,	// N->1
	COG_TO_DEC,	// 1->N
	OBJ_TO_DEC,	// 1->N
	DEC_TO_DEC,	// 1->1
	DEC_TO_EFT,	// 1->N
	EFT_TO_ACT,	// 1->1
	ACT_TO_ACT,	// 1->1
	ACT_TO_TSK,	// 1->N
	TSK_TO_PLT,	// 1->N
};

// 图结构节点定义
struct S_GRAPH_NODE {
	QString id;			// 节点ID
	QString name;		// 节点名称
	E_NODE_TYPE type;	// 节点类型

	bool active = false;	// 是否激活
};

// 图结构边定义
struct S_GRAPH_EDGE {
	QString id;				// 边ID
	QString name;			// 边名称
	E_EDGE_TYPE type;		// 边类型

	QString font_node_id;	// 起始节点ID
	QString back_node_id;	// 终止节点ID
	double weight = 1.0;		// 权重(0-1)

	bool active = false;		// 是否激活
};

// 图结构定义
struct S_CONCEPT_GRAPH {
	QString scenario_id;					// 场景ID
	QString scenario_name;					// 场景名称
	E_SCENARIO_TYPE scenario_type;			// 场景类型

	std::vector<S_GRAPH_NODE> node_list;		// 节点列表
	std::vector<S_GRAPH_EDGE> edge_list;		// 边列表

	QMultiMap<QString, QString> back_edges;	// 起始节点ID -> 边ID
	QMultiMap<QString, QString> font_edges;	// 终止节点ID -> 边ID
};
