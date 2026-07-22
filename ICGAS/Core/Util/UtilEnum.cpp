#include "pch.hpp"
#include "UtilEnum.hpp"

QString UtilEnum::trans_side_en(E_SIDE side) {
	switch (side) {
	case E_SIDE::UKN	:	return "UKN"	;
	case E_SIDE::RED	:	return "RED"	;
	case E_SIDE::BLUE	:	return "BLUE"	;
	case E_SIDE::WHITE	:	return "WHITE"	;
	default				:	return "UKN"	;
	}
}

QString UtilEnum::trans_side_ch(E_SIDE side) {
	switch (side) {
	case E_SIDE::UKN	:	return "不明"	;
	case E_SIDE::RED	:	return "红方"	;
	case E_SIDE::BLUE	:	return "蓝方"	;
	case E_SIDE::WHITE	:	return "白方"	;
	default				:	return "不明"	;
	}
}

E_SIDE UtilEnum::trans_side(const QString& text) {
	const QString side = text.trimmed().toUpper();
	if (side == "RED" || text.trimmed() == QStringLiteral("红方"))	return E_SIDE::RED;
	if (side == "BLUE" || text.trimmed() == QStringLiteral("蓝方"))	return E_SIDE::BLUE;
	if (side == "WHITE" || text.trimmed() == QStringLiteral("白方"))	return E_SIDE::WHITE;
	return E_SIDE::UKN;
}

E_SIDE UtilEnum::trans_side_from_name(const QString& name) {
	const QString side = name.trimmed().toUpper();
	if (side.startsWith(QStringLiteral("RED_")))		return E_SIDE::RED;
	if (side.startsWith(QStringLiteral("BLUE_")))	return E_SIDE::BLUE;
	if (side.startsWith(QStringLiteral("WHITE_")))	return E_SIDE::WHITE;
	return E_SIDE::UKN;
}

QString UtilEnum::trans_domain_en(E_DOMAIN domain) {
	switch (domain) {
	case E_DOMAIN::UKN	:	return "UKN"	;
	case E_DOMAIN::AIR	:	return "AIR"	;
	case E_DOMAIN::SEA	:	return "SEA"	;
	case E_DOMAIN::MIS	:	return "MIS"	;
	default				:	return "UKN"	;
	}
}

QString UtilEnum::trans_domain_ch(E_DOMAIN domain) {
	switch (domain) {
	case E_DOMAIN::UKN	:	return "不明"		;
	case E_DOMAIN::AIR	:	return "空中平台"	;
	case E_DOMAIN::SEA	:	return "水面平台"	;
	case E_DOMAIN::MIS	:	return "导弹平台"	;
	default				:	return "不明"		;
	}
}

E_DOMAIN UtilEnum::trans_domain(const QString& text) {
	const QString domain = text.trimmed().toUpper();
	if (domain == "AIR")	return E_DOMAIN::AIR;
	if (domain == "SEA")	return E_DOMAIN::SEA;
	if (domain == "MIS")	return E_DOMAIN::MIS;
	return E_DOMAIN::UKN;
}

QString UtilEnum::trans_mover_en(E_MOVER mover) {
	switch (mover) {
	case E_MOVER::UKN		:	return "UKN"		;
	case E_MOVER::AIR_MOVER	:	return "AIR_MOVER"	;
	case E_MOVER::SEA_MOVER	:	return "SEA_MOVER"	;
	case E_MOVER::MIS_MOVER	:	return "MIS_MOVER"	;
	default					:	return "UKN"		;
	}
}

QString UtilEnum::trans_mover_ch(E_MOVER mover) {
	switch (mover) {
	case E_MOVER::UKN		:	return "不明"			;
	case E_MOVER::AIR_MOVER	:	return "空中平台Mover"	;
	case E_MOVER::SEA_MOVER	:	return "水面平台Mover"	;
	case E_MOVER::MIS_MOVER	:	return "导弹平台Mover"	;
	default					:	return "不明"			;
	}
}

E_MOVER UtilEnum::trans_mover(const QString& text) {
	const QString mover = text.trimmed().toUpper();
	if (mover == "AIR_MOVER")	return E_MOVER::AIR_MOVER;
	if (mover == "SEA_MOVER")	return E_MOVER::SEA_MOVER;
	if (mover == "MIS_MOVER")	return E_MOVER::MIS_MOVER;
	return E_MOVER::UKN;
}

QString UtilEnum::trans_type_en(E_PLAT_TYPE type) {
	switch (type) {
	case E_PLAT_TYPE::UKN	:	return "UKN";
	case E_PLAT_TYPE::CAR	:	return "CAR";
	case E_PLAT_TYPE::DST	:	return "DST";
	case E_PLAT_TYPE::FRI	:	return "FRI";
	case E_PLAT_TYPE::WAN	:	return "WAN";
	case E_PLAT_TYPE::ELC	:	return "ELC";
	case E_PLAT_TYPE::FLT	:	return "FLT";
	case E_PLAT_TYPE::BOM	:	return "BOM";
	case E_PLAT_TYPE::MIS	:	return "MIS";
	default					:	return "UKN";
	}
}

QString UtilEnum::trans_type_ch(E_PLAT_TYPE type) {
	switch (type) {
	case E_PLAT_TYPE::UKN	:	return "不明"		;
	case E_PLAT_TYPE::CAR	:	return "航空母舰"	;
	case E_PLAT_TYPE::DST	:	return "驱逐舰"		;
	case E_PLAT_TYPE::FRI	:	return "护卫舰"		;
	case E_PLAT_TYPE::WAN	:	return "预警机"		;
	case E_PLAT_TYPE::ELC	:	return "电子战飞机"	;
	case E_PLAT_TYPE::FLT	:	return "战斗机"		;
	case E_PLAT_TYPE::BOM	:	return "轰炸机"		;
	case E_PLAT_TYPE::MIS	:	return "巡航导弹"	;
	default					:	return "不明"		;
	}
}

E_PLAT_TYPE UtilEnum::trans_type(const QString& text) {
	const QString type = text.trimmed().toUpper();
	if (type == "CAR")	return E_PLAT_TYPE::CAR;
	if (type == "DST")	return E_PLAT_TYPE::DST;
	if (type == "FRI")	return E_PLAT_TYPE::FRI;
	if (type == "WAN")	return E_PLAT_TYPE::WAN;
	if (type == "ELC")	return E_PLAT_TYPE::ELC;
	if (type == "FLT")	return E_PLAT_TYPE::FLT;
	if (type == "BOM")	return E_PLAT_TYPE::BOM;
	if (type == "MIS")	return E_PLAT_TYPE::MIS;
	return E_PLAT_TYPE::UKN;
}

QString UtilEnum::trans_weapon_type_en(E_WEAPON_TYPE type) {
	switch (type) {
	case E_WEAPON_TYPE::UKN		:	return "UKN";
	case E_WEAPON_TYPE::AAM		:	return "AAM";
	case E_WEAPON_TYPE::SAM		:	return "SAM";
	case E_WEAPON_TYPE::ASM		:	return "ASM";
	default						:	return "UKN";
	}
}

E_WEAPON_TYPE UtilEnum::trans_weapon_type(const QString& text) {
	const QString type = text.trimmed().toUpper();
	if (type == "AAM")	return E_WEAPON_TYPE::AAM;
	if (type == "SAM")	return E_WEAPON_TYPE::SAM;
	if (type == "ASM" || type == "ASCM")	return E_WEAPON_TYPE::ASM;
	return E_WEAPON_TYPE::UKN;
}

QString UtilEnum::trans_sensor_type_en(E_SENSOR_TYPE type) {
	switch (type) {
	case E_SENSOR_TYPE::UKN		:	return "UKN";
	case E_SENSOR_TYPE::RADAR	:	return "RADAR";
	case E_SENSOR_TYPE::OPTICAL	:	return "OPTICAL";
	case E_SENSOR_TYPE::INFRARED	:	return "INFRARED";
	default						:	return "UKN";
	}
}

E_SENSOR_TYPE UtilEnum::trans_sensor_type(const QString& text) {
	const QString type = text.trimmed().toUpper();
	if (type == "RADAR" || type == "RADAR_SEARCH" || type == "RADAR_FIRE_CONTROL") {
		return E_SENSOR_TYPE::RADAR;
	}
	if (type == "OPTICAL" || type == "EO")	return E_SENSOR_TYPE::OPTICAL;
	if (type == "INFRARED" || type == "IR")	return E_SENSOR_TYPE::INFRARED;
	return E_SENSOR_TYPE::UKN;
}

QString UtilEnum::trans_sensor_task_en(E_SENSOR_TASK task) {
	switch (task) {
	case E_SENSOR_TASK::UKN		:	return "UKN";
	case E_SENSOR_TASK::DETECT	:	return "DETECT";
	case E_SENSOR_TASK::TRACK	:	return "TRACK";
	case E_SENSOR_TASK::GUIDE	:	return "GUIDE";
	default						:	return "UKN";
	}
}

E_SENSOR_TASK UtilEnum::trans_sensor_task(const QString& text) {
	const QString task = text.trimmed().toUpper();
	if (task == "DETECT")	return E_SENSOR_TASK::DETECT;
	if (task == "TRACK")	return E_SENSOR_TASK::TRACK;
	if (task == "GUIDE")	return E_SENSOR_TASK::GUIDE;
	return E_SENSOR_TASK::UKN;
}

QString UtilEnum::trans_plat_life_en(E_PLAT_LIFE state) {
	switch (state) {
	case E_PLAT_LIFE::UKN		:	return "UKN";
	case E_PLAT_LIFE::ALIVE		:	return "ALIVE";
	case E_PLAT_LIFE::DAMAGED	:	return "DAMAGED";
	case E_PLAT_LIFE::DISABLED	:	return "DISABLED";
	case E_PLAT_LIFE::DESTROY	:	return "DESTROY";
	default						:	return "UKN";
	}
}

QString UtilEnum::trans_plat_life_ch(E_PLAT_LIFE state) {
	switch (state) {
	case E_PLAT_LIFE::UKN		:	return "不明";
	case E_PLAT_LIFE::ALIVE		:	return "存活";
	case E_PLAT_LIFE::DAMAGED	:	return "受损";
	case E_PLAT_LIFE::DISABLED	:	return "失能";
	case E_PLAT_LIFE::DESTROY	:	return "摧毁";
	default						:	return "不明";
	}
}

QString UtilEnum::trans_scenario_type_en(E_SCENARIO_TYPE type) {
	switch (type) {
	case E_SCENARIO_TYPE::UKN		:	return "UKN"		;
	case E_SCENARIO_TYPE::DEFENSE	:	return "DEFENSE"	;
	case E_SCENARIO_TYPE::ATTACK	:	return "ATTACK"		;
	case E_SCENARIO_TYPE::ESCORT	:	return "ESCORT"		;
	case E_SCENARIO_TYPE::STRIKE	:	return "STRIKE"		;
	default							:	return "UKN"		;
	}
}

QString UtilEnum::trans_scenario_type_ch(E_SCENARIO_TYPE type) {
	switch (type) {
	case E_SCENARIO_TYPE::UKN		:	return "不明"	;
	case E_SCENARIO_TYPE::DEFENSE	:	return "防御类"	;
	case E_SCENARIO_TYPE::ATTACK	:	return "进攻类"	;
	case E_SCENARIO_TYPE::ESCORT	:	return "护航类"	;
	case E_SCENARIO_TYPE::STRIKE	:	return "打击类"	;
	default							:	return "不明"	;
	}
}

E_SCENARIO_TYPE UtilEnum::trans_scenario_type(const QString& text) {
	const QString type = text.trimmed().toUpper();
	if (type == "DEFENSE")	return E_SCENARIO_TYPE::DEFENSE;
	if (type == "ATTACK")	return E_SCENARIO_TYPE::ATTACK;
	if (type == "ESCORT")	return E_SCENARIO_TYPE::ESCORT;
	if (type == "STRIKE")	return E_SCENARIO_TYPE::STRIKE;
	return E_SCENARIO_TYPE::UKN;
}

QString UtilEnum::trans_node_type_en(E_NODE_TYPE type) {
	switch (type) {
	case E_NODE_TYPE::UKN	:	return "UKN"	;
	case E_NODE_TYPE::MIS	:	return "MIS"	;
	case E_NODE_TYPE::TSK	:	return "TSK"	;
	case E_NODE_TYPE::ACT	:	return "ACT"	;
	case E_NODE_TYPE::OBJ	:	return "OBJ"	;
	case E_NODE_TYPE::COG	:	return "COG"	;
	case E_NODE_TYPE::DEC	:	return "DEC"	;
	case E_NODE_TYPE::EFT	:	return "EFT"	;
	case E_NODE_TYPE::LOO	:	return "LOO"	;
	case E_NODE_TYPE::COA	:	return "COA"	;
	case E_NODE_TYPE::PLT	:	return "PLT"	;
	default					:	return "UKN"	;
	}
}

QString UtilEnum::trans_node_type_ch(E_NODE_TYPE type) {
	switch (type) {
	case E_NODE_TYPE::UKN	:	return QStringLiteral("不明")		;
	case E_NODE_TYPE::MIS	:	return QStringLiteral("上级使命")	;
	case E_NODE_TYPE::TSK	:	return QStringLiteral("关键任务")	;
	case E_NODE_TYPE::ACT	:	return QStringLiteral("可行行动")	;
	case E_NODE_TYPE::OBJ	:	return QStringLiteral("作战目标")	;
	case E_NODE_TYPE::COG	:	return QStringLiteral("作战重心")	;
	case E_NODE_TYPE::DEC	:	return QStringLiteral("决策点")		;
	case E_NODE_TYPE::EFT	:	return QStringLiteral("评估效果")	;
	case E_NODE_TYPE::LOO	:	return QStringLiteral("作战线")		;
	case E_NODE_TYPE::COA	:	return QStringLiteral("行动方案")	;
	case E_NODE_TYPE::PLT	:	return QStringLiteral("可用平台")	;
	default					:	return QStringLiteral("不明")		;
	}
}

QString UtilEnum::trans_node_type_prefix(E_NODE_TYPE type) {
	const QString type_text = trans_node_type_en(type);
	return type_text == "UKN" ? QStringLiteral("NKN") : type_text;
}

E_NODE_TYPE UtilEnum::trans_node_type(const QString& text) {
	const QString type = text.trimmed().toUpper();
	if (type == "MIS")	return E_NODE_TYPE::MIS;
	if (type == "TSK")	return E_NODE_TYPE::TSK;
	if (type == "ACT")	return E_NODE_TYPE::ACT;
	if (type == "OBJ")	return E_NODE_TYPE::OBJ;
	if (type == "COG")	return E_NODE_TYPE::COG;
	if (type == "DEC")	return E_NODE_TYPE::DEC;
	if (type == "EFT")	return E_NODE_TYPE::EFT;
	if (type == "LOO")	return E_NODE_TYPE::LOO;
	if (type == "COA")	return E_NODE_TYPE::COA;
	if (type == "PLT")	return E_NODE_TYPE::PLT;
	return E_NODE_TYPE::UKN;
}

QString UtilEnum::trans_edge_type_en(E_EDGE_TYPE type) {
	switch (type) {
	case E_EDGE_TYPE::UKN		:	return "UKN"		;
	case E_EDGE_TYPE::MIS_TO_COG	:	return "MIS_TO_COG"	;
	case E_EDGE_TYPE::MIS_TO_OBJ	:	return "MIS_TO_OBJ"	;
	case E_EDGE_TYPE::OBJ_TO_LOO	:	return "OBJ_TO_LOO"	;
	case E_EDGE_TYPE::LOO_TO_COA	:	return "LOO_TO_COA"	;
	case E_EDGE_TYPE::COG_TO_DEC	:	return "COG_TO_DEC"	;
	case E_EDGE_TYPE::OBJ_TO_DEC	:	return "OBJ_TO_DEC"	;
	case E_EDGE_TYPE::DEC_TO_DEC	:	return "DEC_TO_DEC"	;
	case E_EDGE_TYPE::DEC_TO_EFT	:	return "DEC_TO_EFT"	;
	case E_EDGE_TYPE::EFT_TO_ACT	:	return "EFT_TO_ACT"	;
	case E_EDGE_TYPE::ACT_TO_ACT	:	return "ACT_TO_ACT"	;
	case E_EDGE_TYPE::ACT_TO_TSK	:	return "ACT_TO_TSK"	;
	case E_EDGE_TYPE::TSK_TO_PLT	:	return "TSK_TO_PLT"	;
	default						:	return "UKN"		;
	}
}

QString UtilEnum::trans_edge_type_ch(E_EDGE_TYPE type) {
	switch (type) {
	case E_EDGE_TYPE::UKN		:	return "不明"			;
	case E_EDGE_TYPE::MIS_TO_COG	:	return "使命到重心"		;
	case E_EDGE_TYPE::MIS_TO_OBJ	:	return "使命到目标"		;
	case E_EDGE_TYPE::OBJ_TO_LOO	:	return "目标到作战线"		;
	case E_EDGE_TYPE::LOO_TO_COA	:	return "作战线到方案"		;
	case E_EDGE_TYPE::COG_TO_DEC	:	return "重心到决策点"		;
	case E_EDGE_TYPE::OBJ_TO_DEC	:	return "目标到决策点"		;
	case E_EDGE_TYPE::DEC_TO_DEC	:	return "决策点到决策点"	;
	case E_EDGE_TYPE::DEC_TO_EFT	:	return "决策点到效果"		;
	case E_EDGE_TYPE::EFT_TO_ACT	:	return "效果到行动"		;
	case E_EDGE_TYPE::ACT_TO_ACT	:	return "行动到行动"		;
	case E_EDGE_TYPE::ACT_TO_TSK	:	return "行动到任务"		;
	case E_EDGE_TYPE::TSK_TO_PLT	:	return "任务到平台"		;
	default						:	return "不明"			;
	}
}

E_EDGE_TYPE UtilEnum::trans_edge_type(const QString& text) {
	const QString type = text.trimmed().toUpper();
	if (type == "MIS_TO_COG")	return E_EDGE_TYPE::MIS_TO_COG;
	if (type == "MIS_TO_OBJ")	return E_EDGE_TYPE::MIS_TO_OBJ;
	if (type == "OBJ_TO_LOO")	return E_EDGE_TYPE::OBJ_TO_LOO;
	if (type == "LOO_TO_COA")	return E_EDGE_TYPE::LOO_TO_COA;
	if (type == "COG_TO_DEC")	return E_EDGE_TYPE::COG_TO_DEC;
	if (type == "OBJ_TO_DEC")	return E_EDGE_TYPE::OBJ_TO_DEC;
	if (type == "DEC_TO_DEC")	return E_EDGE_TYPE::DEC_TO_DEC;
	if (type == "DEC_TO_EFT")	return E_EDGE_TYPE::DEC_TO_EFT;
	if (type == "EFT_TO_ACT")	return E_EDGE_TYPE::EFT_TO_ACT;
	if (type == "ACT_TO_ACT")	return E_EDGE_TYPE::ACT_TO_ACT;
	if (type == "ACT_TO_TSK")	return E_EDGE_TYPE::ACT_TO_TSK;
	if (type == "TSK_TO_PLT")	return E_EDGE_TYPE::TSK_TO_PLT;
	return E_EDGE_TYPE::UKN;
}

E_EDGE_TYPE UtilEnum::trans_edge_type(E_NODE_TYPE source, E_NODE_TYPE target) {
	if (source == E_NODE_TYPE::MIS && target == E_NODE_TYPE::COG) return E_EDGE_TYPE::MIS_TO_COG;
	if (source == E_NODE_TYPE::MIS && target == E_NODE_TYPE::OBJ) return E_EDGE_TYPE::MIS_TO_OBJ;
	if (source == E_NODE_TYPE::OBJ && target == E_NODE_TYPE::LOO) return E_EDGE_TYPE::OBJ_TO_LOO;
	if (source == E_NODE_TYPE::LOO && target == E_NODE_TYPE::COA) return E_EDGE_TYPE::LOO_TO_COA;
	if (source == E_NODE_TYPE::COG && target == E_NODE_TYPE::DEC) return E_EDGE_TYPE::COG_TO_DEC;
	if (source == E_NODE_TYPE::OBJ && target == E_NODE_TYPE::DEC) return E_EDGE_TYPE::OBJ_TO_DEC;
	if (source == E_NODE_TYPE::DEC && target == E_NODE_TYPE::DEC) return E_EDGE_TYPE::DEC_TO_DEC;
	if (source == E_NODE_TYPE::DEC && target == E_NODE_TYPE::EFT) return E_EDGE_TYPE::DEC_TO_EFT;
	if (source == E_NODE_TYPE::EFT && target == E_NODE_TYPE::ACT) return E_EDGE_TYPE::EFT_TO_ACT;
	if (source == E_NODE_TYPE::ACT && target == E_NODE_TYPE::ACT) return E_EDGE_TYPE::ACT_TO_ACT;
	if (source == E_NODE_TYPE::ACT && target == E_NODE_TYPE::TSK) return E_EDGE_TYPE::ACT_TO_TSK;
	if (source == E_NODE_TYPE::TSK && target == E_NODE_TYPE::PLT) return E_EDGE_TYPE::TSK_TO_PLT;
	return E_EDGE_TYPE::UKN;
}

QString UtilEnum::trans_dis_pdu_type_en(quint8 type) {
	switch (type) {
	case 0	: return "Other"								;
	case 1	: return "Entity State"							;
	case 2	: return "Fire"									;
	case 3	: return "Detonation"							;
	case 22	: return "Comment"								;
	case 23	: return "Electromagnetic Emission"				;
	default	: return "Unsupported"							;
	}
}

QString UtilEnum::trans_dis_pdu_type_ch(quint8 type) {
	switch (type) {
	case 0	: return QStringLiteral("其他")						;
	case 1	: return QStringLiteral("实体状态")					;
	case 2	: return QStringLiteral("开火")						;
	case 3	: return QStringLiteral("爆炸")						;
	case 22	: return QStringLiteral("注释")						;
	case 23	: return QStringLiteral("电磁辐射")					;
	default	: return QStringLiteral("不支持")					;
	}
}
