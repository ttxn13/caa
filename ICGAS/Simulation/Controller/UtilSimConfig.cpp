#include "pch.hpp"
#include "UtilSimConfig.hpp"
#include "../../Core/Util/UtilCoor.hpp"
#include "../../Core/Util/UtilEnum.hpp"

namespace {
const QString kPlatformConfigDir = QStringLiteral("Config/ConfigSim/Platform");
const QString kWeaponConfigDir = QStringLiteral("Config/ConfigSim/Weapon");
const QString kSensorConfigDir = QStringLiteral("Config/ConfigSim/Sensor");
const QString kScenarioConfigDir = QStringLiteral("Config/ConfigSim/Scenario");
const QString kSimCtrlConfigPath = QStringLiteral("Config/ConfigSim/ConfigSimCtrl.json");
const QString kDefaultScenarioId = QStringLiteral("OldDefence");

bool read_json_object_file(const QString& config_path, QJsonObject* root_obj)
{
	QFile file_config(config_path);
	if (!file_config.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Error: cannot open file:" << config_path;
		return false;
	}

	QJsonParseError parse_error;
	const QJsonDocument doc_config = QJsonDocument::fromJson(file_config.readAll(), &parse_error);
	if (parse_error.error != QJsonParseError::NoError || !doc_config.isObject()) {
		qDebug() << "Error: cannot parse file:" << config_path << parse_error.errorString();
		return false;
	}

	*root_obj = doc_config.object();
	return true;
}

bool write_json_object_file(const QString& config_path, const QJsonObject& root_obj)
{
	const QFileInfo file_info(config_path);
	if (!QDir().mkpath(file_info.absolutePath())) {
		qDebug() << "Error: cannot create config dir:" << file_info.absolutePath();
		return false;
	}

	QFile file_config(config_path);
	if (!file_config.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		qDebug() << "Error: cannot write file:" << config_path;
		return false;
	}

	file_config.write(QJsonDocument(root_obj).toJson(QJsonDocument::Indented));
	return true;
}

double json_double(const QJsonObject& obj, const QString& key, double default_value = 0.0)
{
	return obj.value(key).toDouble(default_value);
}

bool json_bool(const QJsonObject& obj, const QString& key, bool default_value = false)
{
	return obj.value(key).toBool(default_value);
}

QString json_id(const QJsonObject& obj, const QString& key)
{
	const QJsonValue value = obj.value(key);
	if (value.isString()) return value.toString();
	if (value.isDouble()) return QString::number(static_cast<long long>(value.toDouble()));
	return QString();
}

void read_common_flight_cap(const QJsonObject& obj,
	double& min_speed_ms, double& max_speed_ms, double& min_alt_m, double& max_alt_m,
	double& max_linear_load_g, double& max_radial_load_g)
{
	min_speed_ms = json_double(obj, QStringLiteral("min_speed_ms"));
	max_speed_ms = json_double(obj, QStringLiteral("max_speed_ms"));
	min_alt_m = json_double(obj, QStringLiteral("min_alt_m"));
	max_alt_m = json_double(obj, QStringLiteral("max_alt_m"));
	max_linear_load_g = json_double(obj, QStringLiteral("max_linear_load_g"));
	max_radial_load_g = json_double(obj, QStringLiteral("max_radial_load_g"));
}

QJsonObject write_common_flight_cap(double min_speed_ms, double max_speed_ms, double min_alt_m, double max_alt_m,
	double max_linear_load_g, double max_radial_load_g)
{
	QJsonObject obj;
	obj[QStringLiteral("min_speed_ms")] = min_speed_ms;
	obj[QStringLiteral("max_speed_ms")] = max_speed_ms;
	obj[QStringLiteral("min_alt_m")] = min_alt_m;
	obj[QStringLiteral("max_alt_m")] = max_alt_m;
	obj[QStringLiteral("max_linear_load_g")] = max_linear_load_g;
	obj[QStringLiteral("max_radial_load_g")] = max_radial_load_g;
	return obj;
}

S_AIR_MOVER_CAP read_air_mover_cap(const QJsonObject& obj)
{
	S_AIR_MOVER_CAP cap;
	read_common_flight_cap(obj, cap.min_speed_ms, cap.max_speed_ms, cap.min_alt_m, cap.max_alt_m,
		cap.max_linear_load_g, cap.max_radial_load_g);
	cap.max_climb_vel_ms = json_double(obj, QStringLiteral("max_climb_vel_ms"));
	cap.max_drop_vel_ms = json_double(obj, QStringLiteral("max_drop_vel_ms"));
	return cap;
}

QJsonObject write_air_mover_cap(const S_AIR_MOVER_CAP& cap)
{
	QJsonObject obj = write_common_flight_cap(cap.min_speed_ms, cap.max_speed_ms, cap.min_alt_m, cap.max_alt_m,
		cap.max_linear_load_g, cap.max_radial_load_g);
	obj[QStringLiteral("max_climb_vel_ms")] = cap.max_climb_vel_ms;
	obj[QStringLiteral("max_drop_vel_ms")] = cap.max_drop_vel_ms;
	return obj;
}

S_SEA_MOVER_CAP read_sea_mover_cap(const QJsonObject& obj)
{
	S_SEA_MOVER_CAP cap;
	cap.min_speed_ms = json_double(obj, QStringLiteral("min_speed_ms"));
	cap.max_speed_ms = json_double(obj, QStringLiteral("max_speed_ms"));
	cap.max_linear_acc_ms2 = json_double(obj, QStringLiteral("max_linear_acc_ms2"));
	cap.max_radial_acc_ms2 = json_double(obj, QStringLiteral("max_radial_acc_ms2"));
	return cap;
}

QJsonObject write_sea_mover_cap(const S_SEA_MOVER_CAP& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("min_speed_ms")] = cap.min_speed_ms;
	obj[QStringLiteral("max_speed_ms")] = cap.max_speed_ms;
	obj[QStringLiteral("max_linear_acc_ms2")] = cap.max_linear_acc_ms2;
	obj[QStringLiteral("max_radial_acc_ms2")] = cap.max_radial_acc_ms2;
	return obj;
}

S_MIS_MOVER_CAP read_mis_mover_cap(const QJsonObject& obj)
{
	S_MIS_MOVER_CAP cap;
	read_common_flight_cap(obj, cap.min_speed_ms, cap.max_speed_ms, cap.min_alt_m, cap.max_alt_m,
		cap.max_linear_load_g, cap.max_radial_load_g);
	cap.max_climb_vel_ms = json_double(obj, QStringLiteral("max_climb_vel_ms"));
	cap.max_drop_vel_ms = json_double(obj, QStringLiteral("max_drop_vel_ms"));
	cap.life_range_m = json_double(obj, QStringLiteral("life_range_m"));
	cap.life_time_s = json_double(obj, QStringLiteral("life_time_s"));
	return cap;
}

QJsonObject write_mis_mover_cap(const S_MIS_MOVER_CAP& cap)
{
	QJsonObject obj = write_common_flight_cap(cap.min_speed_ms, cap.max_speed_ms, cap.min_alt_m, cap.max_alt_m,
		cap.max_linear_load_g, cap.max_radial_load_g);
	obj[QStringLiteral("max_climb_vel_ms")] = cap.max_climb_vel_ms;
	obj[QStringLiteral("max_drop_vel_ms")] = cap.max_drop_vel_ms;
	obj[QStringLiteral("life_range_m")] = cap.life_range_m;
	obj[QStringLiteral("life_time_s")] = cap.life_time_s;
	return obj;
}

S_MOVER_CAP read_mover_cap(const QJsonObject& obj, E_MOVER mover)
{
	switch (mover) {
	case E_MOVER::SEA_MOVER:
		return read_sea_mover_cap(obj.value(QStringLiteral("sea_mover")).toObject());
	case E_MOVER::MIS_MOVER:
		return read_mis_mover_cap(obj.value(QStringLiteral("mis_mover")).toObject());
	case E_MOVER::AIR_MOVER:
	default:
		return read_air_mover_cap(obj.value(QStringLiteral("air_mover")).toObject());
	}
}

QJsonObject write_mover_cap(const S_MOVER_CAP& cap)
{
	QJsonObject obj;
	if (const S_AIR_MOVER_CAP* air_cap = std::get_if<S_AIR_MOVER_CAP>(&cap)) {
		obj[QStringLiteral("air_mover")] = write_air_mover_cap(*air_cap);
	}
	else if (const S_SEA_MOVER_CAP* sea_cap = std::get_if<S_SEA_MOVER_CAP>(&cap)) {
		obj[QStringLiteral("sea_mover")] = write_sea_mover_cap(*sea_cap);
	}
	else if (const S_MIS_MOVER_CAP* mis_cap = std::get_if<S_MIS_MOVER_CAP>(&cap)) {
		obj[QStringLiteral("mis_mover")] = write_mis_mover_cap(*mis_cap);
	}
	return obj;
}

S_PLAT_SIGN read_plat_sign(const QJsonObject& obj)
{
	S_PLAT_SIGN sign;
	sign.radar_rcs_m2 = json_double(obj, QStringLiteral("radar_rcs_m2"));
	sign.opt_area_m2 = json_double(obj, QStringLiteral("opt_area_m2"));
	sign.infrared_wsr = json_double(obj, QStringLiteral("infrared_wsr"));
	return sign;
}

QJsonObject write_plat_sign(const S_PLAT_SIGN& sign)
{
	QJsonObject obj;
	obj[QStringLiteral("radar_rcs_m2")] = sign.radar_rcs_m2;
	obj[QStringLiteral("opt_area_m2")] = sign.opt_area_m2;
	obj[QStringLiteral("infrared_wsr")] = sign.infrared_wsr;
	return obj;
}

S_WEAPON_SIGN read_weapon_sign(const QJsonObject& obj)
{
	S_WEAPON_SIGN sign;
	sign.radar_rcs_m2 = json_double(obj, QStringLiteral("radar_rcs_m2"));
	sign.opt_area_m2 = json_double(obj, QStringLiteral("opt_area_m2"));
	sign.infrared_wsr = json_double(obj, QStringLiteral("infrared_wsr"));
	return sign;
}

QJsonObject write_weapon_sign(const S_WEAPON_SIGN& sign)
{
	QJsonObject obj;
	obj[QStringLiteral("radar_rcs_m2")] = sign.radar_rcs_m2;
	obj[QStringLiteral("opt_area_m2")] = sign.opt_area_m2;
	obj[QStringLiteral("infrared_wsr")] = sign.infrared_wsr;
	return obj;
}

S_PLAT_DEFINE read_plat_define(const QJsonObject& obj)
{
	S_PLAT_DEFINE define;
	define.type_name = obj.value(QStringLiteral("type_name")).toString(
		obj.value(QStringLiteral("name")).toString());
	define.icon_path = obj.value(QStringLiteral("icon_path")).toString();
	define.domain = UtilEnum::trans_domain(obj.value(QStringLiteral("domain")).toString());
	define.type = UtilEnum::trans_type(obj.value(QStringLiteral("type")).toString());
	define.mover = UtilEnum::trans_mover(obj.value(QStringLiteral("mover")).toString());
	define.cap = read_mover_cap(obj.value(QStringLiteral("cap")).toObject(), define.mover);
	define.sign = read_plat_sign(obj.value(QStringLiteral("sign")).toObject());
	return define;
}

QJsonObject write_plat_define(const S_PLAT_DEFINE& define)
{
	QJsonObject obj;
	obj[QStringLiteral("type_name")] = define.type_name;
	obj[QStringLiteral("icon_path")] = define.icon_path;
	obj[QStringLiteral("domain")] = UtilEnum::trans_domain_en(define.domain);
	obj[QStringLiteral("type")] = UtilEnum::trans_type_en(define.type);
	obj[QStringLiteral("mover")] = UtilEnum::trans_mover_en(define.mover);
	obj[QStringLiteral("cap")] = write_mover_cap(define.cap);
	obj[QStringLiteral("sign")] = write_plat_sign(define.sign);
	return obj;
}

S_LAUNCH_CAP read_weapon_launch_cap(const QJsonObject& obj)
{
	S_LAUNCH_CAP cap;
	cap.max_range_m = json_double(obj, QStringLiteral("max_range_m"));
	cap.min_range_m = json_double(obj, QStringLiteral("min_range_m"));
	cap.firing_delay_s = json_double(obj, QStringLiteral("firing_delay_s"));
	return cap;
}

QJsonObject write_weapon_launch_cap(const S_LAUNCH_CAP& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("max_range_m")] = cap.max_range_m;
	obj[QStringLiteral("min_range_m")] = cap.min_range_m;
	obj[QStringLiteral("firing_delay_s")] = cap.firing_delay_s;
	return obj;
}

S_KILL_CAP read_weapon_kill_cap(const QJsonObject& obj)
{
	S_KILL_CAP cap;
	cap.kill_range_m = json_double(obj, QStringLiteral("kill_range_m"));
	cap.kill_prob = json_double(obj, QStringLiteral("kill_prob"));
	return cap;
}

QJsonObject write_weapon_kill_cap(const S_KILL_CAP& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("kill_range_m")] = cap.kill_range_m;
	obj[QStringLiteral("kill_prob")] = cap.kill_prob;
	return obj;
}

S_WEAPON_DEFINE read_weapon_define(const QJsonObject& obj)
{
	S_WEAPON_DEFINE define;
	define.type_name = obj.value(QStringLiteral("type_name")).toString();
	define.icon_path = obj.value(QStringLiteral("icon_path")).toString();
	define.domain = UtilEnum::trans_domain(obj.value(QStringLiteral("domain")).toString(QStringLiteral("MIS")));
	define.type = UtilEnum::trans_weapon_type(obj.value(QStringLiteral("type")).toString());
	define.mover = UtilEnum::trans_mover(obj.value(QStringLiteral("mover")).toString(QStringLiteral("MIS_MOVER")));
	define.mover_cap = read_mover_cap(obj.value(QStringLiteral("mover_cap")).toObject(), define.mover);
	define.launch_cap = read_weapon_launch_cap(obj.value(QStringLiteral("launch_cap")).toObject());
	define.kill_cap = read_weapon_kill_cap(obj.value(QStringLiteral("kill_cap")).toObject());
	define.sign = read_weapon_sign(obj.value(QStringLiteral("sign")).toObject());
	return define;
}

QJsonObject write_weapon_define(const S_WEAPON_DEFINE& define)
{
	QJsonObject obj;
	obj[QStringLiteral("type_name")] = define.type_name;
	obj[QStringLiteral("icon_path")] = define.icon_path;
	obj[QStringLiteral("domain")] = UtilEnum::trans_domain_en(define.domain);
	obj[QStringLiteral("type")] = UtilEnum::trans_weapon_type_en(define.type);
	obj[QStringLiteral("mover")] = UtilEnum::trans_mover_en(define.mover);
	obj[QStringLiteral("mover_cap")] = write_mover_cap(define.mover_cap);
	obj[QStringLiteral("launch_cap")] = write_weapon_launch_cap(define.launch_cap);
	obj[QStringLiteral("kill_cap")] = write_weapon_kill_cap(define.kill_cap);
	obj[QStringLiteral("sign")] = write_weapon_sign(define.sign);
	return obj;
}

std::vector<E_DOMAIN> read_domain_list(const QJsonArray& array)
{
	std::vector<E_DOMAIN> domain_list;
	for (const QJsonValue& value : array) {
		const E_DOMAIN domain = UtilEnum::trans_domain(value.toString());
		if (domain != E_DOMAIN::UKN) domain_list.push_back(domain);
	}
	return domain_list;
}

QJsonArray write_domain_list(const std::vector<E_DOMAIN>& domain_list)
{
	QJsonArray array;
	for (const E_DOMAIN domain : domain_list) {
		if (domain == E_DOMAIN::UKN) continue;
		array.append(UtilEnum::trans_domain_en(domain));
	}
	return array;
}

S_SENSOR_CAP_ENV read_sensor_cap_env(const QJsonObject& obj)
{
	S_SENSOR_CAP_ENV cap;
	cap.min_range_m = json_double(obj, QStringLiteral("min_range_m"));
	cap.max_range_m = json_double(obj, QStringLiteral("max_range_m"));
	cap.az_fov_deg = json_double(obj, QStringLiteral("az_fov_deg"));
	cap.min_el_deg = json_double(obj, QStringLiteral("min_el_deg"));
	cap.max_el_deg = json_double(obj, QStringLiteral("max_el_deg"));
	cap.min_alt_m = json_double(obj, QStringLiteral("min_alt_m"));
	cap.max_alt_m = json_double(obj, QStringLiteral("max_alt_m"));
	return cap;
}

QJsonObject write_sensor_cap_env(const S_SENSOR_CAP_ENV& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("min_range_m")] = cap.min_range_m;
	obj[QStringLiteral("max_range_m")] = cap.max_range_m;
	obj[QStringLiteral("az_fov_deg")] = cap.az_fov_deg;
	obj[QStringLiteral("min_el_deg")] = cap.min_el_deg;
	obj[QStringLiteral("max_el_deg")] = cap.max_el_deg;
	obj[QStringLiteral("min_alt_m")] = cap.min_alt_m;
	obj[QStringLiteral("max_alt_m")] = cap.max_alt_m;
	return obj;
}

S_SENSOR_CAP_PROB read_sensor_cap_prob(const QJsonObject& obj)
{
	S_SENSOR_CAP_PROB cap;
	cap.detection_prob = json_double(obj, QStringLiteral("detection_prob"));
	cap.false_alarm_prob = json_double(obj, QStringLiteral("false_alarm_prob"));
	cap.sensitive_slope = json_double(obj, QStringLiteral("sensitive_slope"));
	return cap;
}

QJsonObject write_sensor_cap_prob(const S_SENSOR_CAP_PROB& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("detection_prob")] = cap.detection_prob;
	obj[QStringLiteral("false_alarm_prob")] = cap.false_alarm_prob;
	obj[QStringLiteral("sensitive_slope")] = cap.sensitive_slope;
	return obj;
}

S_SENSOR_CAP_ACC read_sensor_cap_acc(const QJsonObject& obj)
{
	S_SENSOR_CAP_ACC cap;
	cap.measure_range = json_bool(obj, QStringLiteral("measure_range"), true);
	cap.measure_az = json_bool(obj, QStringLiteral("measure_az"), true);
	cap.measure_el = json_bool(obj, QStringLiteral("measure_el"), true);
	cap.measure_speed = json_bool(obj, QStringLiteral("measure_speed"));
	cap.range_sigma_m = json_double(obj, QStringLiteral("range_sigma_m"));
	cap.azimuth_sigma_deg = json_double(obj, QStringLiteral("azimuth_sigma_deg"));
	cap.elevation_sigma_deg = json_double(obj, QStringLiteral("elevation_sigma_deg"));
	cap.speed_sigma_ms = json_double(obj, QStringLiteral("speed_sigma_ms"));
	return cap;
}

QJsonObject write_sensor_cap_acc(const S_SENSOR_CAP_ACC& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("measure_range")] = cap.measure_range;
	obj[QStringLiteral("measure_az")] = cap.measure_az;
	obj[QStringLiteral("measure_el")] = cap.measure_el;
	obj[QStringLiteral("measure_speed")] = cap.measure_speed;
	obj[QStringLiteral("range_sigma_m")] = cap.range_sigma_m;
	obj[QStringLiteral("azimuth_sigma_deg")] = cap.azimuth_sigma_deg;
	obj[QStringLiteral("elevation_sigma_deg")] = cap.elevation_sigma_deg;
	obj[QStringLiteral("speed_sigma_ms")] = cap.speed_sigma_ms;
	return obj;
}

S_SENSOR_CAP_TIME read_sensor_cap_time(const QJsonObject& obj)
{
	S_SENSOR_CAP_TIME cap;
	cap.update_period_s = json_double(obj, QStringLiteral("update_period_s"),
		json_double(obj, QStringLiteral("scan_period_s")));
	cap.delay_ms = json_double(obj, QStringLiteral("delay_ms"));
	return cap;
}

QJsonObject write_sensor_cap_time(const S_SENSOR_CAP_TIME& cap)
{
	QJsonObject obj;
	obj[QStringLiteral("update_period_s")] = cap.update_period_s;
	obj[QStringLiteral("delay_ms")] = cap.delay_ms;
	return obj;
}

S_SENSOR_TASK_MODE read_sensor_task_mode(const QJsonObject& obj)
{
	S_SENSOR_TASK_MODE mode;
	mode.max_task_num = obj.value(QStringLiteral("max_task_num")).toInt();
	mode.cap_env = read_sensor_cap_env(obj.value(QStringLiteral("cap_env")).toObject());
	mode.cap_prob = read_sensor_cap_prob(obj.value(QStringLiteral("cap_prob")).toObject());
	mode.cap_acc = read_sensor_cap_acc(obj.value(QStringLiteral("cap_acc")).toObject());
	mode.cap_time = read_sensor_cap_time(obj.value(QStringLiteral("cap_time")).toObject());
	return mode;
}

QJsonObject write_sensor_task_mode(const S_SENSOR_TASK_MODE& mode)
{
	QJsonObject obj;
	obj[QStringLiteral("max_task_num")] = mode.max_task_num;
	obj[QStringLiteral("cap_env")] = write_sensor_cap_env(mode.cap_env);
	obj[QStringLiteral("cap_prob")] = write_sensor_cap_prob(mode.cap_prob);
	obj[QStringLiteral("cap_acc")] = write_sensor_cap_acc(mode.cap_acc);
	obj[QStringLiteral("cap_time")] = write_sensor_cap_time(mode.cap_time);
	return obj;
}

std::map<E_SENSOR_TASK, S_SENSOR_TASK_MODE> read_sensor_task_mode_list(const QJsonObject& obj)
{
	std::map<E_SENSOR_TASK, S_SENSOR_TASK_MODE> task_mode_list;
	for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
		const E_SENSOR_TASK task = UtilEnum::trans_sensor_task(iter.key());
		if (task == E_SENSOR_TASK::UKN || !iter.value().isObject()) continue;
		task_mode_list[task] = read_sensor_task_mode(iter.value().toObject());
	}
	return task_mode_list;
}

QJsonObject write_sensor_task_mode_list(const std::map<E_SENSOR_TASK, S_SENSOR_TASK_MODE>& task_mode_list)
{
	QJsonObject obj;
	for (const auto& item : task_mode_list) {
		if (item.first == E_SENSOR_TASK::UKN) continue;
		obj[UtilEnum::trans_sensor_task_en(item.first)] = write_sensor_task_mode(item.second);
	}
	return obj;
}

S_SENSOR_DEFINE read_sensor_define(const QJsonObject& obj)
{
	S_SENSOR_DEFINE define;
	define.type_name = obj.value(QStringLiteral("type_name")).toString();
	define.type = UtilEnum::trans_sensor_type(obj.value(QStringLiteral("type")).toString());
	define.reliability = json_double(obj, QStringLiteral("reliability"), 1.0);
	define.act_domain_list = read_domain_list(obj.value(QStringLiteral("act_domain_list")).toArray());
	define.task_mode_list = read_sensor_task_mode_list(obj.value(QStringLiteral("task_mode_list")).toObject());
	return define;
}

QJsonObject write_sensor_define(const S_SENSOR_DEFINE& define)
{
	QJsonObject obj;
	obj[QStringLiteral("type_name")] = define.type_name;
	obj[QStringLiteral("type")] = UtilEnum::trans_sensor_type_en(define.type);
	obj[QStringLiteral("reliability")] = define.reliability;
	obj[QStringLiteral("act_domain_list")] = write_domain_list(define.act_domain_list);
	obj[QStringLiteral("task_mode_list")] = write_sensor_task_mode_list(define.task_mode_list);
	return obj;
}

double route_heading_rad_from_points(const S_PLAT_ROUTE& route)
{
	if (route.point_list.size() < 2) return 0.0;
	const S_ROUTE_POINT& from = route.point_list.at(0);
	const S_ROUTE_POINT& to = route.point_list.at(1);
	const double lat_avg = (from.pos_lla.lat_rad + to.pos_lla.lat_rad) * 0.5;
	const double east = (to.pos_lla.lon_rad - from.pos_lla.lon_rad) * std::cos(lat_avg);
	const double north = to.pos_lla.lat_rad - from.pos_lla.lat_rad;
	if (std::abs(east) <= 1.0e-12 && std::abs(north) <= 1.0e-12) return 0.0;
	return UtilCoor::norm_rad_0_2pi(std::atan2(east, north));
}

S_VEL_LOC route_initial_velocity(const S_PLAT_ROUTE& route)
{
	const double speed = route.point_list.empty() ? 0.0 : route.point_list.front().speed_ms;
	return S_VEL_LOC(speed, route_heading_rad_from_points(route), 0.0);
}

S_POS_LLA read_pos_lla(const QJsonObject& obj)
{
	S_POS_LLA pos;
	pos.lon_rad = json_double(obj, QStringLiteral("lon_rad"));
	pos.lat_rad = json_double(obj, QStringLiteral("lat_rad"));
	pos.alt_m = json_double(obj, QStringLiteral("alt_m"));
	return pos;
}

QJsonObject write_pos_lla(const S_POS_LLA& pos)
{
	QJsonObject obj;
	obj[QStringLiteral("lon_rad")] = pos.lon_rad;
	obj[QStringLiteral("lat_rad")] = pos.lat_rad;
	obj[QStringLiteral("alt_m")] = pos.alt_m;
	return obj;
}

S_VEL_LOC read_vel_loc(const QJsonObject& obj)
{
	S_VEL_LOC vel;
	vel.speed_ms = json_double(obj, QStringLiteral("speed_ms"));
	vel.track_ang_rad = json_double(obj, QStringLiteral("track_ang_rad"));
	vel.path_ang_rad = json_double(obj, QStringLiteral("path_ang_rad"));
	return vel;
}

QJsonObject write_vel_loc(const S_VEL_LOC& vel)
{
	QJsonObject obj;
	obj[QStringLiteral("speed_ms")] = vel.speed_ms;
	obj[QStringLiteral("track_ang_rad")] = vel.track_ang_rad;
	obj[QStringLiteral("path_ang_rad")] = vel.path_ang_rad;
	return obj;
}

S_PLAT_STATE read_plat_state(const QJsonObject& obj)
{
	S_PLAT_STATE state;
	state.pos_lla = read_pos_lla(obj.value(QStringLiteral("pos_lla")).toObject());
	state.vel_loc = read_vel_loc(obj.value(QStringLiteral("vel_loc")).toObject());
	state.update_time_ms = 0;
	return state;
}

QJsonObject write_plat_state(const S_PLAT_STATE& state)
{
	QJsonObject obj;
	obj[QStringLiteral("pos_lla")] = write_pos_lla(state.pos_lla);
	obj[QStringLiteral("vel_loc")] = write_vel_loc(state.vel_loc);
	return obj;
}

S_ROUTE_POINT read_route_point(const QJsonObject& obj)
{
	S_ROUTE_POINT point;
	point.point_id = obj.value(QStringLiteral("point_id")).toString();
	point.pos_lla = read_pos_lla(obj.value(QStringLiteral("pos_lla")).toObject());
	point.speed_ms = json_double(obj, QStringLiteral("speed_ms"));
	return point;
}

QJsonObject write_route_point(const S_ROUTE_POINT& point)
{
	QJsonObject obj;
	obj[QStringLiteral("point_id")] = point.point_id;
	obj[QStringLiteral("pos_lla")] = write_pos_lla(point.pos_lla);
	obj[QStringLiteral("speed_ms")] = point.speed_ms;
	return obj;
}

S_PLAT_ROUTE read_route(const QJsonObject& obj)
{
	S_PLAT_ROUTE route;
	route.route_id = obj.value(QStringLiteral("route_id")).toString();
	route.is_circulate = obj.value(QStringLiteral("is_circulate")).toBool(false);

	const QJsonArray point_array = obj.value(QStringLiteral("point_list")).toArray();
	for (const QJsonValue& point_value : point_array) {
		if (!point_value.isObject()) continue;
		route.point_list.push_back(read_route_point(point_value.toObject()));
	}

	return route;
}

QJsonObject write_route(const S_PLAT_ROUTE& route)
{
	QJsonObject obj;
	obj[QStringLiteral("route_id")] = route.route_id;
	obj[QStringLiteral("is_circulate")] = route.is_circulate;

	QJsonArray point_array;
	for (const S_ROUTE_POINT& point : route.point_list) {
		point_array.append(write_route_point(point));
	}
	obj[QStringLiteral("point_list")] = point_array;
	return obj;
}

QString legacy_config_type_name(const QString& config_name, const QString& platform_id)
{
	const std::pair<QString, QString> legacy_type_names[] = {
		{QStringLiteral("CAR"), QStringLiteral("CVN-59")},
		{QStringLiteral("DST"), QStringLiteral("052D")},
		{QStringLiteral("WAN"), QStringLiteral("E-3")},
		{QStringLiteral("ELC"), QStringLiteral("F-18E")},
		{QStringLiteral("FLT"), QStringLiteral("F-15")},
		{QStringLiteral("BOM"), QStringLiteral("B-52")},
	};

	QString type_name = config_name.trimmed();
	const QString id = platform_id.trimmed();
	if (!id.isEmpty() &&
		type_name.endsWith(QStringLiteral("_") + id, Qt::CaseInsensitive)) {
		type_name.chop(id.size() + 1);
	}
	for (const QString& prefix : { QStringLiteral("RED_"), QStringLiteral("BLUE_"), QStringLiteral("WHITE_") }) {
		if (type_name.startsWith(prefix, Qt::CaseInsensitive)) {
			type_name = type_name.mid(prefix.size());
			break;
		}
	}
	for (const auto& legacy_type : legacy_type_names) {
		if (type_name.compare(legacy_type.first, Qt::CaseInsensitive) == 0) {
			return legacy_type.second;
		}
	}
	return type_name;
}

QString inferred_fmt_id(E_SIDE side, const QString& platform_id)
{
	const QString id = platform_id.trimmed();
	int fmt_base = 0;
	if (side == E_SIDE::RED) {
		fmt_base = 3000;
	}
	else if (side == E_SIDE::BLUE) {
		fmt_base = 4000;
	}
	else if (id.startsWith(QStringLiteral("1"))) {
		fmt_base = 3000;
	}
	else if (id.startsWith(QStringLiteral("2"))) {
		fmt_base = 4000;
	}
	else {
		return QString();
	}

	if (id.size() < 3) return QString();
	bool ok = false;
	const int formation_num = id.mid(1, 2).toInt(&ok);
	if (!ok || formation_num <= 0) return QString();
	return QString::number(fmt_base + formation_num);
}

QString legacy_weapon_load_type_name(const QString& key)
{
	const QString type = key.trimmed().toUpper();
	if (type == QStringLiteral("AAM_IR")) return QStringLiteral("PL-10");
	if (type == QStringLiteral("AAM_BVR")) return QStringLiteral("PL-12");
	if (type == QStringLiteral("SAM")) return QStringLiteral("HQ-15");
	if (type == QStringLiteral("ASCM")) return QStringLiteral("KD-88");
	return key.trimmed();
}

std::map<QString, int> read_weapon_load(const QJsonObject& obj)
{
	std::map<QString, int> load_map;
	for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
		const QString type_name = legacy_weapon_load_type_name(iter.key());
		if (type_name.isEmpty()) continue;
		load_map[type_name] = iter.value().toInt();
	}
	return load_map;
}

QJsonObject write_weapon_load(const std::map<QString, int>& weapon_load)
{
	QJsonObject obj;
	for (const auto& item : weapon_load) {
		const QString type_name = item.first.trimmed();
		if (type_name.isEmpty()) continue;
		obj[type_name] = item.second;
	}
	return obj;
}

QString legacy_sensor_load_type_name(const QString& key, const QString& platform_type_name)
{
	const QString type = key.trimmed().toUpper();
	const QString platform_type = platform_type_name.trimmed().toUpper();
	if (type == QStringLiteral("DATALINK") || type == QStringLiteral("ESM")) return QString();
	if (type == QStringLiteral("052D_MULTIFUNCTION_RADAR")) return QStringLiteral("AN_SPY_6_V1");
	if (type == QStringLiteral("052D_IRST") || type == QStringLiteral("F15_IRST")) return QStringLiteral("IRST21");
	if (type == QStringLiteral("052D_OPTICAL_TRACKER")) return QStringLiteral("WESCAM_MX_15D");
	if (type == QStringLiteral("F15_MULTIFUNCTION_RADAR")) return QStringLiteral("AN_APG_63_V3");
	if (type == QStringLiteral("F18E_OPTICAL_POD")) return QStringLiteral("AN_ASQ_228_ATFLIR");
	if (type == QStringLiteral("CVN59_AIR_SEARCH_RADAR")) return QStringLiteral("AN_SPS_48G");
	if (type == QStringLiteral("E3_AWACS_RADAR")) return QStringLiteral("AN_APY_2");
	if (type == QStringLiteral("B52_STRIKE_RADAR")) return QStringLiteral("AN_APQ_166");
	if (type == QStringLiteral("RADAR_SEARCH") || type == QStringLiteral("RADAR_FIRE_CONTROL")) {
		if (platform_type == QStringLiteral("E-3")) return QStringLiteral("AN_APY_2");
		if (platform_type == QStringLiteral("052D")) return QStringLiteral("AN_SPY_6_V1");
		if (platform_type == QStringLiteral("CVN-59")) return QStringLiteral("AN_SPS_48G");
		if (platform_type == QStringLiteral("F-15")) return QStringLiteral("AN_APG_63_V3");
		if (platform_type == QStringLiteral("F-18E")) return QStringLiteral("AN_ASQ_228_ATFLIR");
		if (platform_type == QStringLiteral("B-52")) return QStringLiteral("AN_APQ_166");
	}
	return key.trimmed();
}

std::map<QString, int> read_sensor_load(const QJsonObject& obj, const QString& platform_type_name)
{
	std::map<QString, int> load_map;
	for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
		const QString type_name = legacy_sensor_load_type_name(iter.key(), platform_type_name);
		if (type_name.isEmpty()) continue;
		load_map[type_name] += iter.value().toInt();
	}
	return load_map;
}

QJsonObject write_sensor_load(const std::map<QString, int>& sensor_load)
{
	QJsonObject obj;
	for (const auto& item : sensor_load) {
		const QString type_name = item.first.trimmed();
		if (type_name.isEmpty()) continue;
		obj[type_name] = item.second;
	}
	return obj;
}

void fill_state_from_route(S_PLAT_CONFIG& config, double legacy_heading_rad, bool has_legacy_heading)
{
	if (config.init_route.point_list.empty()) return;
	config.init_state.pos_lla = config.init_route.point_list.front().pos_lla;
	config.init_state.vel_loc = route_initial_velocity(config.init_route);
	if (has_legacy_heading) config.init_state.vel_loc.track_ang_rad = legacy_heading_rad;
}

void ensure_route_defaults(S_PLAT_CONFIG& config)
{
	const QString platform_id = config.plt_id.trimmed();
	if (config.init_route.route_id.trimmed().isEmpty()) {
		config.init_route.route_id = platform_id + QStringLiteral("_route");
	}
	if (!config.init_route.point_list.empty()) return;

	S_ROUTE_POINT first_point;
	first_point.point_id = platform_id + QStringLiteral("_wp_001");
	first_point.pos_lla = config.init_state.pos_lla;
	first_point.speed_ms = config.init_state.vel_loc.speed_ms;
	config.init_route.point_list.push_back(first_point);
}

S_PLAT_CONFIG read_plat_config(const QJsonObject& obj)
{
	S_PLAT_CONFIG config;
	config.plt_id = json_id(obj, QStringLiteral("plt_id"));
	const QString legacy_name = obj.value(QStringLiteral("name")).toString();
	config.type_name = obj.value(QStringLiteral("type_name")).toString();
	if (config.type_name.trimmed().isEmpty()) {
		config.type_name = legacy_config_type_name(legacy_name, config.plt_id);
	}
	config.cmd_id = json_id(obj, QStringLiteral("cmd_id"));
	config.side = UtilEnum::trans_side(obj.value(QStringLiteral("side")).toString());
	if (config.side == E_SIDE::UKN) config.side = UtilEnum::trans_side_from_name(legacy_name);
	if (config.side == E_SIDE::UKN) config.side = UtilEnum::trans_side_from_name(config.type_name);
	if (config.side == E_SIDE::UKN) config.side = UtilEnum::trans_side_from_name(config.plt_id);
	config.fmt_id = json_id(obj, QStringLiteral("fmt_id"));
	if (config.fmt_id.trimmed().isEmpty()) {
		config.fmt_id = inferred_fmt_id(config.side, config.plt_id);
	}

	bool has_legacy_heading = false;
	double legacy_heading_rad = 0.0;
	if (obj.contains(QStringLiteral("init_route"))) {
		config.init_route = read_route(obj.value(QStringLiteral("init_route")).toObject());
	}
	else if (obj.contains(QStringLiteral("route"))) {
		const QJsonObject legacy_route_obj = obj.value(QStringLiteral("route")).toObject();
		config.init_route = read_route(legacy_route_obj);
		if (legacy_route_obj.contains(QStringLiteral("init_heading_rad"))) {
			legacy_heading_rad = json_double(legacy_route_obj, QStringLiteral("init_heading_rad"));
			has_legacy_heading = true;
		}
		else if (legacy_route_obj.contains(QStringLiteral("init_heading_deg"))) {
			legacy_heading_rad = json_double(legacy_route_obj, QStringLiteral("init_heading_deg")) *
				std::numbers::pi_v<double> / 180.0;
			has_legacy_heading = true;
		}
	}

	if (obj.contains(QStringLiteral("init_state"))) {
		config.init_state = read_plat_state(obj.value(QStringLiteral("init_state")).toObject());
	}
	else {
		fill_state_from_route(config, legacy_heading_rad, has_legacy_heading);
	}

	ensure_route_defaults(config);
	config.weapon_load = read_weapon_load(obj.value(QStringLiteral("weapon_load")).toObject());
	config.sensor_load = read_sensor_load(obj.value(QStringLiteral("sensor_load")).toObject(), config.type_name);
	return config;
}

QJsonObject write_plat_config(const S_PLAT_CONFIG& config)
{
	QJsonObject obj;
	obj[QStringLiteral("plt_id")] = config.plt_id;
	obj[QStringLiteral("type_name")] = config.type_name;
	obj[QStringLiteral("cmd_id")] = config.cmd_id;
	const QString fmt_id = config.fmt_id.trimmed().isEmpty() ?
		inferred_fmt_id(config.side, config.plt_id) : config.fmt_id;
	obj[QStringLiteral("fmt_id")] = fmt_id;
	obj[QStringLiteral("side")] = UtilEnum::trans_side_en(config.side);
	obj[QStringLiteral("init_state")] = write_plat_state(config.init_state);
	obj[QStringLiteral("init_route")] = write_route(config.init_route);
	obj[QStringLiteral("weapon_load")] = write_weapon_load(config.weapon_load);
	obj[QStringLiteral("sensor_load")] = write_sensor_load(config.sensor_load);
	return obj;
}

QString platform_config_path(const QString& define_name)
{
	return kPlatformConfigDir + QStringLiteral("/") + define_name.trimmed() + QStringLiteral(".json");
}

QString weapon_define_path(const QString& define_name)
{
	return kWeaponConfigDir + QStringLiteral("/") + define_name.trimmed() + QStringLiteral(".json");
}

QString sensor_define_path(const QString& define_name)
{
	return kSensorConfigDir + QStringLiteral("/") + define_name.trimmed() + QStringLiteral(".json");
}

QString scenario_config_path(const QString& scenario_id)
{
	return kScenarioConfigDir + QStringLiteral("/") + scenario_id + QStringLiteral(".json");
}

bool config_files(const QString& config_dir, QFileInfoList* files)
{
	const QDir dir(config_dir);
	if (!dir.exists()) {
		qDebug() << "Error: config dir not found:" << config_dir;
		return false;
	}
	*files = dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Name);
	return true;
}

bool remove_config_file(const QString& config_path)
{
	if (!QFileInfo::exists(config_path)) return true;
	if (QFile::remove(config_path)) return true;
	qDebug() << "Error: cannot remove file:" << config_path;
	return false;
}

bool save_plat_define_pair(const std::pair<QString, S_PLAT_DEFINE>& plat_define)
{
	S_PLAT_DEFINE define = plat_define.second;
	if (define.type_name.trimmed().isEmpty()) {
		define.type_name = plat_define.first.trimmed();
	}
	if (define.type_name.trimmed().isEmpty()) {
		qDebug() << "Error: cannot save plat_define with empty type_name";
		return false;
	}

	return write_json_object_file(platform_config_path(define.type_name), write_plat_define(define));
}

bool save_weapon_define_pair(const std::pair<QString, S_WEAPON_DEFINE>& weapon_define)
{
	S_WEAPON_DEFINE define = weapon_define.second;
	if (define.type_name.trimmed().isEmpty()) {
		define.type_name = weapon_define.first.trimmed();
	}
	if (define.type_name.trimmed().isEmpty()) {
		qDebug() << "Error: cannot save weapon_define with empty type_name";
		return false;
	}

	return write_json_object_file(weapon_define_path(define.type_name), write_weapon_define(define));
}

bool save_sensor_define_pair(const std::pair<QString, S_SENSOR_DEFINE>& sensor_define)
{
	S_SENSOR_DEFINE define = sensor_define.second;
	if (define.type_name.trimmed().isEmpty()) {
		define.type_name = sensor_define.first.trimmed();
	}
	if (define.type_name.trimmed().isEmpty()) {
		qDebug() << "Error: cannot save sensor_define with empty type_name";
		return false;
	}

	return write_json_object_file(sensor_define_path(define.type_name), write_sensor_define(define));
}

bool save_plat_config_pair(const std::pair<QString, std::vector<S_PLAT_CONFIG>>& plat_config)
{
	const QString scenario_id = plat_config.first.trimmed().isEmpty() ? kDefaultScenarioId : plat_config.first.trimmed();
	QJsonArray config_array;
	for (const S_PLAT_CONFIG& config : plat_config.second) {
		config_array.append(write_plat_config(config));
	}

	QJsonObject root_obj;
	QJsonObject existing_root;
	if (read_json_object_file(scenario_config_path(scenario_id), &existing_root)) {
		root_obj = existing_root;
	}
	root_obj[QStringLiteral("plat_config")] = config_array;
	return write_json_object_file(scenario_config_path(scenario_id), root_obj);
}

}

bool UtilSimConfig::load_sim_ctrl(int& sim_iter_ms, double& time_acc_ratio)
{
	QJsonObject root_obj;
	if (!read_json_object_file(kSimCtrlConfigPath, &root_obj)) {
		return false;
	}

	const QJsonObject ctrl_obj = root_obj.contains(QStringLiteral("sim_ctrl")) ?
		root_obj.value(QStringLiteral("sim_ctrl")).toObject() : root_obj;

	const int loaded_sim_iter_ms = ctrl_obj.value(QStringLiteral("sim_iter_ms")).toInt(sim_iter_ms);
	const double loaded_time_acc_ratio = ctrl_obj.value(QStringLiteral("time_acc_ratio")).toDouble(time_acc_ratio);
	if (loaded_sim_iter_ms <= 0 || loaded_time_acc_ratio <= 0.0) {
		qDebug() << "Error: invalid sim ctrl config:" << kSimCtrlConfigPath;
		return false;
	}

	sim_iter_ms = loaded_sim_iter_ms;
	time_acc_ratio = loaded_time_acc_ratio;
	return true;
}

bool UtilSimConfig::save_sim_ctrl(int sim_iter_ms, double time_acc_ratio)
{
	if (sim_iter_ms <= 0 || time_acc_ratio <= 0.0) {
		qDebug() << "Error: cannot save invalid sim ctrl config";
		return false;
	}

	QJsonObject ctrl_obj;
	ctrl_obj[QStringLiteral("sim_iter_ms")] = sim_iter_ms;
	ctrl_obj[QStringLiteral("time_acc_ratio")] = time_acc_ratio;

	QJsonObject root_obj;
	root_obj[QStringLiteral("sim_ctrl")] = ctrl_obj;
	return write_json_object_file(kSimCtrlConfigPath, root_obj);
}

bool UtilSimConfig::load_plat_define(std::map<QString, S_PLAT_DEFINE>& list_plat_define)
{
	list_plat_define.clear();

	bool load_success = true;
	QFileInfoList files;
	if (!config_files(kPlatformConfigDir, &files)) return false;

	for (const QFileInfo& file_info : files) {
		QJsonObject root_obj;
		if (!read_json_object_file(file_info.filePath(), &root_obj)) {
			load_success = false;
			continue;
		}

		const QJsonObject define_obj = root_obj.contains(QStringLiteral("plat_define")) ?
			root_obj.value(QStringLiteral("plat_define")).toObject() : root_obj;
		if (define_obj.isEmpty()) {
			qDebug() << "Error: missing plat_define:" << file_info.filePath();
			load_success = false;
			continue;
		}

		const S_PLAT_DEFINE define = read_plat_define(define_obj);
		if (define.type_name.trimmed().isEmpty()) {
			qDebug() << "Error: missing plat_define type_name:" << file_info.filePath();
			load_success = false;
			continue;
		}
		list_plat_define[define.type_name] = define;
	}

	return load_success;
}

bool UtilSimConfig::load_plat_config(std::map<QString, std::vector<S_PLAT_CONFIG>>& list_plat_config)
{
	list_plat_config.clear();

	bool load_success = true;
	QFileInfoList files;
	if (!config_files(kScenarioConfigDir, &files)) return false;

	for (const QFileInfo& file_info : files) {
		QJsonObject root_obj;
		if (!read_json_object_file(file_info.filePath(), &root_obj)) {
			load_success = false;
			continue;
		}

		const QJsonArray config_array = root_obj.value(QStringLiteral("plat_config")).toArray();
		for (const QJsonValue& config_value : config_array) {
			if (!config_value.isObject()) continue;
			list_plat_config[file_info.baseName()].push_back(read_plat_config(config_value.toObject()));
		}
	}

	return load_success;
}

bool UtilSimConfig::load_weapon_define(std::map<QString, S_WEAPON_DEFINE>& list_weapon_define)
{
	list_weapon_define.clear();

	bool load_success = true;
	QFileInfoList files;
	if (!config_files(kWeaponConfigDir, &files)) return false;

	for (const QFileInfo& file_info : files) {
		QJsonObject root_obj;
		if (!read_json_object_file(file_info.filePath(), &root_obj)) {
			load_success = false;
			continue;
		}

		const QJsonObject define_obj = root_obj.contains(QStringLiteral("weapon_define")) ?
			root_obj.value(QStringLiteral("weapon_define")).toObject() : root_obj;
		if (define_obj.isEmpty()) {
			qDebug() << "Error: missing weapon_define:" << file_info.filePath();
			load_success = false;
			continue;
		}

		const S_WEAPON_DEFINE define = read_weapon_define(define_obj);
		if (define.type_name.trimmed().isEmpty()) {
			qDebug() << "Error: missing weapon_define type_name:" << file_info.filePath();
			load_success = false;
			continue;
		}
		list_weapon_define[define.type_name] = define;
	}

	return load_success;
}

bool UtilSimConfig::load_sensor_define(std::map<QString, S_SENSOR_DEFINE>& list_sensor_define)
{
	list_sensor_define.clear();

	bool load_success = true;
	QFileInfoList files;
	if (!config_files(kSensorConfigDir, &files)) return false;

	for (const QFileInfo& file_info : files) {
		QJsonObject root_obj;
		if (!read_json_object_file(file_info.filePath(), &root_obj)) {
			load_success = false;
			continue;
		}

		const QJsonObject define_obj = root_obj.contains(QStringLiteral("sensor_define")) ?
			root_obj.value(QStringLiteral("sensor_define")).toObject() : root_obj;
		if (define_obj.isEmpty()) {
			qDebug() << "Error: missing sensor_define:" << file_info.filePath();
			load_success = false;
			continue;
		}

		const S_SENSOR_DEFINE define = read_sensor_define(define_obj);
		if (define.type_name.trimmed().isEmpty()) {
			qDebug() << "Error: missing sensor_define type_name:" << file_info.filePath();
			load_success = false;
			continue;
		}
		list_sensor_define[define.type_name] = define;
	}

	return load_success;
}

bool UtilSimConfig::save_plat_define(QSharedPointer<const std::pair<QString, S_PLAT_DEFINE>> plat_define)
{
	if (plat_define.isNull()) {
		qDebug() << "Error: cannot save null plat_define snapshot";
		return false;
	}
	return save_plat_define_pair(*plat_define);
}

bool UtilSimConfig::save_plat_config(QSharedPointer<const std::pair<QString, std::vector<S_PLAT_CONFIG>>> plat_config)
{
	if (plat_config.isNull()) {
		qDebug() << "Error: cannot save null plat_config snapshot";
		return false;
	}
	return save_plat_config_pair(*plat_config);
}

bool UtilSimConfig::save_weapon_define(QSharedPointer<const std::pair<QString, S_WEAPON_DEFINE>> weapon_define)
{
	if (weapon_define.isNull()) {
		qDebug() << "Error: cannot save null weapon_define snapshot";
		return false;
	}
	return save_weapon_define_pair(*weapon_define);
}

bool UtilSimConfig::save_sensor_define(QSharedPointer<const std::pair<QString, S_SENSOR_DEFINE>> sensor_define)
{
	if (sensor_define.isNull()) {
		qDebug() << "Error: cannot save null sensor_define snapshot";
		return false;
	}
	return save_sensor_define_pair(*sensor_define);
}

bool UtilSimConfig::rmov_plat_define(QSharedPointer<const std::pair<QString, S_PLAT_DEFINE>> plat_define)
{
	if (plat_define.isNull()) {
		qDebug() << "Error: cannot remove null plat_define snapshot";
		return false;
	}

	QString define_name = plat_define->first.trimmed();
	if (define_name.isEmpty()) define_name = plat_define->second.type_name.trimmed();
	if (define_name.isEmpty()) {
		qDebug() << "Error: cannot remove plat_define with empty type_name";
		return false;
	}

	return remove_config_file(platform_config_path(define_name));
}

bool UtilSimConfig::rmov_weapon_define(QSharedPointer<const std::pair<QString, S_WEAPON_DEFINE>> weapon_define)
{
	if (weapon_define.isNull()) {
		qDebug() << "Error: cannot remove null weapon_define snapshot";
		return false;
	}

	QString define_name = weapon_define->first.trimmed();
	if (define_name.isEmpty()) define_name = weapon_define->second.type_name.trimmed();
	if (define_name.isEmpty()) {
		qDebug() << "Error: cannot remove weapon_define with empty type_name";
		return false;
	}

	return remove_config_file(weapon_define_path(define_name));
}

bool UtilSimConfig::rmov_sensor_define(QSharedPointer<const std::pair<QString, S_SENSOR_DEFINE>> sensor_define)
{
	if (sensor_define.isNull()) {
		qDebug() << "Error: cannot remove null sensor_define snapshot";
		return false;
	}

	QString define_name = sensor_define->first.trimmed();
	if (define_name.isEmpty()) define_name = sensor_define->second.type_name.trimmed();
	if (define_name.isEmpty()) {
		qDebug() << "Error: cannot remove sensor_define with empty type_name";
		return false;
	}

	return remove_config_file(sensor_define_path(define_name));
}

bool UtilSimConfig::rmov_plat_config(QSharedPointer<const std::pair<QString, std::vector<S_PLAT_CONFIG>>> plat_config)
{
	if (plat_config.isNull()) {
		qDebug() << "Error: cannot remove null plat_config snapshot";
		return false;
	}

	const QString scenario_id = plat_config->first.trimmed();
	if (scenario_id.isEmpty()) {
		qDebug() << "Error: cannot remove plat_config with empty scenario id";
		return false;
	}

	return remove_config_file(scenario_config_path(scenario_id));
}
