#include "pch.hpp"
#include "UtilCoor.hpp"

S_POS_ECEF UtilCoor::pos_lla2ecef(const S_POS_LLA& pos_lla) {
	const double sin_lat = std::sin(pos_lla.lat_rad);
	const double cos_lat = std::cos(pos_lla.lat_rad);
	const double sin_lon = std::sin(pos_lla.lon_rad);
	const double cos_lon = std::cos(pos_lla.lon_rad);
	const double n = WGS84_A_M / std::sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);

	return S_POS_ECEF(
		(n + pos_lla.alt_m) * cos_lat * cos_lon,
		(n + pos_lla.alt_m) * cos_lat * sin_lon,
		(n * (1.0 - WGS84_E2) + pos_lla.alt_m) * sin_lat);
}

S_POS_ENU UtilCoor::pos_ecef2enu(const S_POS_ECEF& ecef_del, const S_POS_LLA& lla_own) {
	S_POS_ENU pos_enu;
	const double sin_lat = std::sin(lla_own.lat_rad);
	const double cos_lat = std::cos(lla_own.lat_rad);
	const double sin_lon = std::sin(lla_own.lon_rad);
	const double cos_lon = std::cos(lla_own.lon_rad);

	pos_enu.e_m = -sin_lon * ecef_del.x_m + cos_lon * ecef_del.y_m;
	pos_enu.n_m = -sin_lat * cos_lon * ecef_del.x_m - sin_lat * sin_lon * ecef_del.y_m + cos_lat * ecef_del.z_m;
	pos_enu.u_m = cos_lat * cos_lon * ecef_del.x_m + cos_lat * sin_lon * ecef_del.y_m + sin_lat * ecef_del.z_m;
	return pos_enu;
}

S_VEL_ENU UtilCoor::vel_ecef2enu(const S_VEL_ECEF& ecef_del, const S_POS_LLA& lla_own) {
	S_VEL_ENU vel_enu;
	const double sin_lat = std::sin(lla_own.lat_rad);
	const double cos_lat = std::cos(lla_own.lat_rad);
	const double sin_lon = std::sin(lla_own.lon_rad);
	const double cos_lon = std::cos(lla_own.lon_rad);

	vel_enu.ve_ms = -sin_lon * ecef_del.vx_ms + cos_lon * ecef_del.vy_ms;
	vel_enu.vn_ms = -sin_lat * cos_lon * ecef_del.vx_ms - sin_lat * sin_lon * ecef_del.vy_ms + cos_lat * ecef_del.vz_ms;
	vel_enu.vu_ms = cos_lat * cos_lon * ecef_del.vx_ms + cos_lat * sin_lon * ecef_del.vy_ms + sin_lat * ecef_del.vz_ms;
	return vel_enu;
}

S_VEL_ECEF UtilCoor::vel_enu2ecef(const S_VEL_ENU& vel_enu, const S_POS_LLA& lla_own) {
	S_VEL_ECEF vel_ecef;
	const double sin_lat = std::sin(lla_own.lat_rad);
	const double cos_lat = std::cos(lla_own.lat_rad);
	const double sin_lon = std::sin(lla_own.lon_rad);
	const double cos_lon = std::cos(lla_own.lon_rad);

	vel_ecef.vx_ms = -sin_lon * vel_enu.ve_ms - sin_lat * cos_lon * vel_enu.vn_ms + cos_lat * cos_lon * vel_enu.vu_ms;
	vel_ecef.vy_ms = cos_lon * vel_enu.ve_ms - sin_lat * sin_lon * vel_enu.vn_ms + cos_lat * sin_lon * vel_enu.vu_ms;
	vel_ecef.vz_ms = cos_lat * vel_enu.vn_ms + sin_lat * vel_enu.vu_ms;
	return vel_ecef;
}

S_VEL_ECEF UtilCoor::vel_loc2ecef(const S_VEL_LOC& vel_loc, const S_POS_LLA& lla_own) {
	const double cos_path = std::cos(vel_loc.path_ang_rad);
	const S_VEL_ENU vel_enu(
		vel_loc.speed_ms * cos_path * std::sin(vel_loc.track_ang_rad),
		vel_loc.speed_ms * cos_path * std::cos(vel_loc.track_ang_rad),
		vel_loc.speed_ms * std::sin(vel_loc.path_ang_rad));
	return vel_enu2ecef(vel_enu, lla_own);
}

double UtilCoor::cal_dis_ecef(const S_POS_ECEF& ecef_own, const S_POS_ECEF& ecef_tgt)
{
    return std::sqrt(std::pow(ecef_tgt.x_m - ecef_own.x_m, 2) +
					 std::pow(ecef_tgt.y_m - ecef_own.y_m, 2) +
					 std::pow(ecef_tgt.z_m - ecef_own.z_m, 2));
}

double UtilCoor::cal_dis_surf(const S_POS_LLA& lla_own, const S_POS_LLA& lla_tgt) {
	const double d_lat = lla_tgt.lat_rad - lla_own.lat_rad;
	const double d_lon = lla_tgt.lon_rad - lla_own.lon_rad;
	const double sin_d_lat = std::sin(d_lat * 0.5);
	const double sin_d_lon = std::sin(d_lon * 0.5);
	const double a = sin_d_lat * sin_d_lat
		+ std::cos(lla_own.lat_rad) * std::cos(lla_tgt.lat_rad) * sin_d_lon * sin_d_lon;
	const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
	return EARTH_RADIUS_M * c;
}

S_POS_LLA UtilCoor::pos_ecef2lla(const S_POS_ECEF& pos_ecef) {
	const double p = std::hypot(pos_ecef.x_m, pos_ecef.y_m);
	double lon_rad = std::atan2(pos_ecef.y_m, pos_ecef.x_m);
	double lat_rad = std::atan2(pos_ecef.z_m, p * (1.0 - WGS84_E2));
	double alt_m = 0.0;

	// 迭代求解纬度和高度，8次迭代收敛到毫米级精度
	for (int i = 0; i < 8; ++i) {
		const double sin_lat = std::sin(lat_rad);
		const double n = WGS84_A_M / std::sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);
		alt_m = p / std::cos(lat_rad) - n;
		lat_rad = std::atan2(pos_ecef.z_m, p * (1.0 - WGS84_E2 * n / (n + alt_m)));
	}
	double sin_lat = std::sin(lat_rad);
	double n = WGS84_A_M / std::sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);
	alt_m = p / std::cos(lat_rad) - n;

	return S_POS_LLA(lon_rad, lat_rad, alt_m);
}

S_VEL_LOC UtilCoor::vel_ecef2loc(const S_POS_LLA& pos_lla, const S_VEL_ECEF& vel_ecef) {
	const double sin_lon = std::sin(pos_lla.lon_rad);
	const double cos_lon = std::cos(pos_lla.lon_rad);
	const double sin_lat = std::sin(pos_lla.lat_rad);
	const double cos_lat = std::cos(pos_lla.lat_rad);
	const double ve = -sin_lon * vel_ecef.vx_ms + cos_lon * vel_ecef.vy_ms;
	const double vn = -cos_lon * sin_lat * vel_ecef.vx_ms - sin_lon * sin_lat * vel_ecef.vy_ms + cos_lat * vel_ecef.vz_ms;
	const double vu = cos_lon * cos_lat * vel_ecef.vx_ms + sin_lon * cos_lat * vel_ecef.vy_ms + sin_lat * vel_ecef.vz_ms;
	const double speed_ms = std::sqrt(ve * ve + vn * vn + vu * vu);
	const double theta_rad = std::atan2(ve, vn);
	const double gamma_rad = std::atan2(vu, std::hypot(ve, vn));
	return S_VEL_LOC(speed_ms, theta_rad, gamma_rad);
}

double UtilCoor::norm_rad_0_2pi(const double value)
{
	double norm_value = value;
	while (norm_value < 0.0)				norm_value += 2.0 * CORE_PI;
	while (norm_value >= 2.0 * CORE_PI)	norm_value -= 2.0 * CORE_PI;
	return norm_value;
}

double UtilCoor::norm_rad_pi_pi(const double value)
{
	double norm_value = value;
	while (norm_value < -CORE_PI)	norm_value += 2.0 * CORE_PI;
	while (norm_value >= CORE_PI)	norm_value -= 2.0 * CORE_PI;
	return norm_value;
}

S_POS_ECEF UtilCoor::equal_pos_ecef(const S_POS_ECEF& raw_ecef, const S_POS_ECEF& hst_ecef, const double raw_ratio)
{
	const double hst_ratio = 1.0 - raw_ratio;
	return S_POS_ECEF(
		raw_ecef.x_m * raw_ratio + hst_ecef.x_m * hst_ratio,
		raw_ecef.y_m * raw_ratio + hst_ecef.y_m * hst_ratio,
		raw_ecef.z_m * raw_ratio + hst_ecef.z_m * hst_ratio);
}

S_VEL_ECEF UtilCoor::equal_vel_ecef(const S_VEL_ECEF& raw_ecef, const S_VEL_ECEF& hst_ecef, const double raw_ratio)
{
	const double hst_ratio = 1.0 - raw_ratio;
	return S_VEL_ECEF(
		raw_ecef.vx_ms * raw_ratio + hst_ecef.vx_ms * hst_ratio,
		raw_ecef.vy_ms * raw_ratio + hst_ecef.vy_ms * hst_ratio,
		raw_ecef.vz_ms * raw_ratio + hst_ecef.vz_ms * hst_ratio);
}

S_POS_LLA UtilCoor::equal_pos_lla(const S_POS_LLA& raw_lla, const S_POS_LLA& hst_lla, const double raw_ratio)
{
	const double hst_ratio = 1.0 - raw_ratio;
	return S_POS_LLA(
		norm_rad_pi_pi(hst_lla.lon_rad + norm_rad_pi_pi(raw_lla.lon_rad - hst_lla.lon_rad) * raw_ratio),
		raw_lla.lat_rad * raw_ratio + hst_lla.lat_rad * hst_ratio,
		raw_lla.alt_m * raw_ratio + hst_lla.alt_m * hst_ratio);
}

S_VEL_LOC UtilCoor::equal_vel_loc(const S_VEL_LOC& raw_loc, const S_VEL_LOC& hst_loc, const double raw_ratio)
{
	const double hst_ratio = 1.0 - raw_ratio;
	return S_VEL_LOC(
		raw_loc.speed_ms * raw_ratio + hst_loc.speed_ms * hst_ratio,
		norm_rad_0_2pi(hst_loc.track_ang_rad + norm_rad_pi_pi(raw_loc.track_ang_rad - hst_loc.track_ang_rad) * raw_ratio),
		norm_rad_pi_pi(hst_loc.path_ang_rad + norm_rad_pi_pi(raw_loc.path_ang_rad - hst_loc.path_ang_rad) * raw_ratio));
}
