#pragma once
#include "pch.hpp"
#include "../../Core/Sim/DefPlatform.hpp"
#include "../../Core/Sim/DefSensor.hpp"
#include "../../Core/Sim/DefWeapon.hpp"

namespace UtilSimConfig {

	bool load_sim_ctrl(int& sim_iter_ms, double& time_acc_ratio);
	bool save_sim_ctrl(int sim_iter_ms, double time_acc_ratio);

	bool load_plat_define(std::map<QString, S_PLAT_DEFINE>& list_plat_define);
	bool load_plat_config(std::map<QString, std::vector<S_PLAT_CONFIG>>& list_plat_config);
	bool save_plat_define(QSharedPointer<const std::pair<QString, S_PLAT_DEFINE>> plat_define);
	bool save_plat_config(QSharedPointer<const std::pair<QString, std::vector<S_PLAT_CONFIG>>> plat_config);
	bool rmov_plat_define(QSharedPointer<const std::pair<QString, S_PLAT_DEFINE>> plat_define);
	bool rmov_plat_config(QSharedPointer<const std::pair<QString, std::vector<S_PLAT_CONFIG>>> plat_config);

	bool load_weapon_define(std::map<QString, S_WEAPON_DEFINE>& list_weapon_define);
	bool save_weapon_define(QSharedPointer<const std::pair<QString, S_WEAPON_DEFINE>> weapon_define);
	bool rmov_weapon_define(QSharedPointer<const std::pair<QString, S_WEAPON_DEFINE>> weapon_define);

	bool load_sensor_define(std::map<QString, S_SENSOR_DEFINE>& list_sensor_define);
	bool save_sensor_define(QSharedPointer<const std::pair<QString, S_SENSOR_DEFINE>> sensor_define);
	bool rmov_sensor_define(QSharedPointer<const std::pair<QString, S_SENSOR_DEFINE>> sensor_define);

}
