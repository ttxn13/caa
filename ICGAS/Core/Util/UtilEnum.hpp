#pragma once
#include "pch.hpp"
#include "../Comm/CoreGraph.hpp"
#include "../Sim/CoreSimEnum.hpp"

namespace UtilEnum {

	QString trans_side_en(E_SIDE side);
	QString trans_side_ch(E_SIDE side);
	E_SIDE trans_side(const QString& text);
	E_SIDE trans_side_from_name(const QString& name);

	QString trans_domain_en(E_DOMAIN domain);
	QString trans_domain_ch(E_DOMAIN domain);
	E_DOMAIN trans_domain(const QString& text);

	QString trans_mover_en(E_MOVER mover);
	QString trans_mover_ch(E_MOVER mover);
	E_MOVER trans_mover(const QString& text);

	QString trans_type_en(E_PLAT_TYPE type);
	QString trans_type_ch(E_PLAT_TYPE type);
	E_PLAT_TYPE trans_type(const QString& text);

	QString trans_weapon_type_en(E_WEAPON_TYPE type);
	E_WEAPON_TYPE trans_weapon_type(const QString& text);

	QString trans_sensor_type_en(E_SENSOR_TYPE type);
	E_SENSOR_TYPE trans_sensor_type(const QString& text);
	QString trans_sensor_task_en(E_SENSOR_TASK task);
	E_SENSOR_TASK trans_sensor_task(const QString& text);

	QString trans_plat_life_en(E_PLAT_LIFE state);
	QString trans_plat_life_ch(E_PLAT_LIFE state);

	QString trans_scenario_type_en(E_SCENARIO_TYPE type);
	QString trans_scenario_type_ch(E_SCENARIO_TYPE type);
	E_SCENARIO_TYPE trans_scenario_type(const QString& text);

	QString trans_node_type_en(E_NODE_TYPE type);
	QString trans_node_type_ch(E_NODE_TYPE type);
	QString trans_node_type_prefix(E_NODE_TYPE type);
	E_NODE_TYPE trans_node_type(const QString& text);

	QString trans_edge_type_en(E_EDGE_TYPE type);
	QString trans_edge_type_ch(E_EDGE_TYPE type);
	E_EDGE_TYPE trans_edge_type(const QString& text);
	E_EDGE_TYPE trans_edge_type(E_NODE_TYPE source, E_NODE_TYPE target);

	QString trans_dis_pdu_type_en(quint8 type);
	QString trans_dis_pdu_type_ch(quint8 type);
}
