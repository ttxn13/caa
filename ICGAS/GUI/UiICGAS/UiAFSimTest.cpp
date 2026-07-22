#include "pch.hpp"
#include "UiAFSimTest.hpp"

#include <array>
#include <limits>

namespace {

constexpr quint32 DERIVED_AIRCRAFT_PLATFORM_ID = 90101;
constexpr quint16 DERIVED_AIRCRAFT_DIS_ENTITY_ID = static_cast<quint16>(DERIVED_AIRCRAFT_PLATFORM_ID & 0xFFFFu);
constexpr double DERIVED_AIRCRAFT_SOUTH_OFFSET_M = 100000.0;
constexpr double DERIVED_AIRCRAFT_EAST_SPEED_MPS = 120.0;
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kDegToRad = kPi / 180.0;

struct S_TEST_LLA {
	double lat_deg = 0.0;
	double lon_deg = 0.0;
	double alt_m = 0.0;
};

struct S_TEST_ENU_VEL {
	double east_mps = 0.0;
	double north_mps = 0.0;
	double up_mps = 0.0;
	double speed_mps = 0.0;
	double track_deg = 0.0;
	double pitch_deg = 0.0;
};

QVariantMap make_field(const QString& name, const QString& value, const QString& note = QString())
{
	QVariantMap row;
	row["type_name"] = name;
	row["value"] = value;
	row["note"] = note;
	return row;
}

QVariantMap send_field(const QString& key, const QString& label, const QString& value, const QString& note = QString())
{
	return QVariantMap{
		{"key", key},
		{"label", label},
		{"value", value},
		{"note", note}
	};
}

QVariantMap send_group(const QString& label, const QVariantList& parts, const QString& note = QString(),
	int row_height = 38, int columns = 0)
{
	QVariantMap row{
		{"label", label},
		{"parts", parts},
		{"note", note}
	};
	if (row_height > 0) row["rowHeight"] = row_height;
	if (columns > 0) row["columns"] = columns;
	return row;
}

QString cfg_path_candidate(const QString& base)
{
	return QDir::cleanPath(base + QStringLiteral("/ICGAS/Config/ConfigComm.json"));
}

QString number_text(double value, char format = 'f', int precision = 3)
{
	return QString::number(value, format, precision);
}

QString f32(float value, int precision = 3)
{
	return QString::number(value, 'f', precision);
}

QString f64(double value, int precision = 3)
{
	return QString::number(value, 'f', precision);
}

QString fdeg(double value, int precision = 6)
{
	return QString::number(value, 'f', precision) + QStringLiteral(" deg");
}

double norm_deg_0_360(double value)
{
	while (value < 0.0) value += 360.0;
	while (value >= 360.0) value -= 360.0;
	return value;
}

QString offset_note(int offset, int size = 1)
{
	if (size <= 1) {
		return QString("offset +%1").arg(offset);
	}
	return QString("offset +%1..+%2").arg(offset).arg(offset + size - 1);
}

bool has_bytes(const QByteArray& data, int offset, int size)
{
	return offset >= 0 && size >= 0 && offset <= data.size() && size <= data.size() - offset;
}

int remaining_bytes(const QByteArray& data, int offset)
{
	return offset < data.size() ? static_cast<int>(data.size() - offset) : 0;
}

QString bytes_to_hex(const QByteArray& data)
{
	return QString::fromLatin1(data.toHex(' ').toUpper());
}

QByteArray hex_to_bytes(const QString& text, QString* error)
{
	QString compact = text;
	compact.remove(QChar(' '));
	compact.remove(QChar('\n'));
	compact.remove(QChar('\r'));
	compact.remove(QChar('\t'));
	compact.remove(QStringLiteral("0x"), Qt::CaseInsensitive);
	compact.remove(QChar(','));

	if (compact.isEmpty()) {
		if (error != nullptr) *error = QStringLiteral("十六进制输入为空。");
		return {};
	}
	if ((compact.size() % 2) != 0) {
		if (error != nullptr) *error = QStringLiteral("十六进制输入长度不是偶数。");
		return {};
	}
	for (const QChar ch : compact) {
		if (!ch.isDigit() && (ch.toLower() < QChar('a') || ch.toLower() > QChar('f'))) {
			if (error != nullptr) *error = QStringLiteral("十六进制字符无效：%1").arg(ch);
			return {};
		}
	}
	if (error != nullptr) error->clear();
	return QByteArray::fromHex(compact.toLatin1());
}

QString pdu_type_name(int pdu_type)
{
	switch (pdu_type) {
	case 1: return QStringLiteral("实体状态");
	case 2: return QStringLiteral("发射");
	case 3: return QStringLiteral("爆炸");
	case 23: return QStringLiteral("电磁辐射");
	default: return QStringLiteral("未知");
	}
}

QString pdu_structure_name(int pdu_type)
{
	switch (pdu_type) {
	case 1: return QStringLiteral("实体状态 PDU");
	case 2: return QStringLiteral("发射 PDU");
	case 3: return QStringLiteral("爆炸 PDU");
	case 23: return QStringLiteral("电磁辐射 PDU");
	default: return QStringLiteral("未知 PDU");
	}
}

void write_u8(QByteArray& data, int offset, quint8 value)
{
	if (has_bytes(data, offset, 1)) data[offset] = static_cast<char>(value);
}

void write_u16(QByteArray& data, int offset, quint16 value)
{
	if (has_bytes(data, offset, 2)) qToBigEndian(value, reinterpret_cast<uchar*>(data.data() + offset));
}

void write_u32(QByteArray& data, int offset, quint32 value)
{
	if (has_bytes(data, offset, 4)) qToBigEndian(value, reinterpret_cast<uchar*>(data.data() + offset));
}

void write_f32(QByteArray& data, int offset, float value)
{
	quint32 raw = 0;
	std::memcpy(&raw, &value, sizeof(raw));
	write_u32(data, offset, raw);
}

void write_f64(QByteArray& data, int offset, double value)
{
	quint64 raw = 0;
	std::memcpy(&raw, &value, sizeof(raw));
	if (has_bytes(data, offset, 8)) qToBigEndian(raw, reinterpret_cast<uchar*>(data.data() + offset));
}

void write_header(QByteArray& data, quint8 exercise_id, quint8 pdu_type, quint8 family, quint16 length)
{
	write_u8(data, 0, 5);
	write_u8(data, 1, exercise_id);
	write_u8(data, 2, pdu_type);
	write_u8(data, 3, family);
	write_u32(data, 4, static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF));
	write_u16(data, 8, length);
	write_u8(data, 10, 0);
	write_u8(data, 11, 0);
}

S_AFSimTestEntityIdentifier read_entity_id(const QByteArray& data, int offset)
{
	S_AFSimTestEntityIdentifier id;
	id.site = UtilDisPdu::read_u16(data, offset);
	id.application = UtilDisPdu::read_u16(data, offset + 2);
	id.entity = UtilDisPdu::read_u16(data, offset + 4);
	id.text = QString("%1:%2:%3").arg(id.site).arg(id.application).arg(id.entity);
	return id;
}

S_AFSimTestEntityType read_entity_type(const QByteArray& data, int offset)
{
	S_AFSimTestEntityType type;
	type.kind = UtilDisPdu::read_u8(data, offset);
	type.domain = UtilDisPdu::read_u8(data, offset + 1);
	type.country = UtilDisPdu::read_u16(data, offset + 2);
	type.category = UtilDisPdu::read_u8(data, offset + 4);
	type.subcategory = UtilDisPdu::read_u8(data, offset + 5);
	type.specific = UtilDisPdu::read_u8(data, offset + 6);
	type.extra = UtilDisPdu::read_u8(data, offset + 7);
	type.text = QString("%1:%2:%3:%4:%5:%6:%7")
		.arg(type.kind).arg(type.domain).arg(type.country).arg(type.category)
		.arg(type.subcategory).arg(type.specific).arg(type.extra);
	return type;
}

S_AFSimTestBurstDescriptor read_burst_descriptor(const QByteArray& data, int offset)
{
	S_AFSimTestBurstDescriptor burst;
	burst.entity_type = read_entity_type(data, offset);
	burst.warhead = UtilDisPdu::read_u16(data, offset + 8);
	burst.fuse = UtilDisPdu::read_u16(data, offset + 10);
	burst.quantity = UtilDisPdu::read_u16(data, offset + 12);
	burst.rate = UtilDisPdu::read_u16(data, offset + 14);
	return burst;
}

void write_entity_id(QByteArray& data, int offset, const S_AFSimTestEntityIdentifier& id)
{
	write_u16(data, offset, id.site);
	write_u16(data, offset + 2, id.application);
	write_u16(data, offset + 4, id.entity);
}

void write_entity_type(QByteArray& data, int offset, const S_AFSimTestEntityType& type)
{
	write_u8(data, offset, type.kind);
	write_u8(data, offset + 1, type.domain);
	write_u16(data, offset + 2, type.country);
	write_u8(data, offset + 4, type.category);
	write_u8(data, offset + 5, type.subcategory);
	write_u8(data, offset + 6, type.specific);
	write_u8(data, offset + 7, type.extra);
}

void write_burst_descriptor(QByteArray& data, int offset, const S_AFSimTestBurstDescriptor& burst)
{
	write_entity_type(data, offset, burst.entity_type);
	write_u16(data, offset + 8, burst.warhead);
	write_u16(data, offset + 10, burst.fuse);
	write_u16(data, offset + 12, burst.quantity);
	write_u16(data, offset + 14, burst.rate);
}

QVariantMap id_map(const S_AFSimTestEntityIdentifier& id)
{
	return QVariantMap{
		{"text", id.text},
		{"site", id.site},
		{"application", id.application},
		{"entity", id.entity}
	};
}

QVariantMap type_map(const S_AFSimTestEntityType& type)
{
	return QVariantMap{
		{"text", type.text},
		{"kind", type.kind},
		{"domain", type.domain},
		{"country", type.country},
		{"category", type.category},
		{"subcategory", type.subcategory},
		{"specific", type.specific},
		{"extra", type.extra}
	};
}

QVariantMap burst_map(const S_AFSimTestBurstDescriptor& burst)
{
	return QVariantMap{
		{"entity_type", type_map(burst.entity_type)},
		{"warhead", burst.warhead},
		{"fuse", burst.fuse},
		{"quantity", burst.quantity},
		{"rate", burst.rate}
	};
}

QString marking_text(const S_PDU_ENTITY_STATE& pdu)
{
	QByteArray marking;
	for (int i = 0; i < 11; ++i) {
		marking.append(static_cast<char>(pdu.entity_marking[i]));
	}
	const int zero_pos = marking.indexOf('\0');
	if (zero_pos >= 0) marking.truncate(zero_pos);
	return QString::fromLatin1(marking).trimmed();
}

template<typename PduEntityId>
S_AFSimTestEntityIdentifier entity_id_from_pdu(const PduEntityId& pdu_id)
{
	S_AFSimTestEntityIdentifier id;
	id.text = pdu_id.entity_identifier;
	id.site = pdu_id.entity_id_site;
	id.application = pdu_id.entity_id_application;
	id.entity = pdu_id.entity_id_entity;
	return id;
}

template<typename PduEntityType>
S_AFSimTestEntityType entity_type_from_pdu(const PduEntityType& pdu_type)
{
	S_AFSimTestEntityType type;
	type.text = pdu_type.entity_type_complete;
	type.kind = pdu_type.entity_type_kind;
	type.domain = pdu_type.entity_type_domain;
	type.country = pdu_type.entity_type_country;
	type.category = pdu_type.entity_type_category;
	type.subcategory = pdu_type.entity_type_subcategory;
	type.specific = pdu_type.entity_type_specific;
	type.extra = pdu_type.entity_type_extra;
	return type;
}

bool ecef_to_lla(double x_m, double y_m, double z_m, S_TEST_LLA& lla)
{
	if (!std::isfinite(x_m) || !std::isfinite(y_m) || !std::isfinite(z_m)) return false;
	if (std::hypot(std::hypot(x_m, y_m), z_m) < 1.0) return false;

	const S_POS_ECEF ecef(x_m, y_m, z_m);
	const S_POS_LLA pos_lla = UtilCoor::pos_ecef2lla(ecef);
	lla.lon_deg = pos_lla.lon_rad * kRadToDeg;
	lla.lat_deg = pos_lla.lat_rad * kRadToDeg;
	lla.alt_m = pos_lla.alt_m;
	return std::isfinite(lla.lon_deg) && std::isfinite(lla.lat_deg) && std::isfinite(lla.alt_m);
}

void lla_to_ecef(const S_TEST_LLA& lla, double& x_m, double& y_m, double& z_m)
{
	const S_POS_ECEF ecef = UtilCoor::pos_lla2ecef(S_POS_LLA(lla.lon_deg * kDegToRad, lla.lat_deg * kDegToRad, lla.alt_m));
	x_m = ecef.x_m;
	y_m = ecef.y_m;
	z_m = ecef.z_m;
}

bool ecef_velocity_to_enu(double vx_mps, double vy_mps, double vz_mps, const S_TEST_LLA& lla, S_TEST_ENU_VEL& velocity)
{
	S_POS_LLA pos_lla(lla.lon_deg * kDegToRad, lla.lat_deg * kDegToRad, lla.alt_m);
	const S_VEL_ENU enu = UtilCoor::vel_ecef2enu(S_VEL_ECEF(vx_mps, vy_mps, vz_mps), pos_lla);
	velocity.east_mps = enu.ve_ms;
	velocity.north_mps = enu.vn_ms;
	velocity.up_mps = enu.vu_ms;
	velocity.speed_mps = std::sqrt(enu.ve_ms * enu.ve_ms + enu.vn_ms * enu.vn_ms + enu.vu_ms * enu.vu_ms);
	velocity.track_deg = norm_deg_0_360(std::atan2(enu.ve_ms, enu.vn_ms) * kRadToDeg);
	velocity.pitch_deg = std::atan2(enu.vu_ms, std::hypot(enu.ve_ms, enu.vn_ms)) * kRadToDeg;
	return std::isfinite(velocity.speed_mps);
}

void enu_velocity_to_ecef(const S_TEST_LLA& lla, double east_mps, double north_mps, double up_mps,
	double& vx_mps, double& vy_mps, double& vz_mps)
{
	const S_POS_LLA pos_lla(lla.lon_deg * kDegToRad, lla.lat_deg * kDegToRad, lla.alt_m);
	const S_VEL_ECEF ecef = UtilCoor::vel_enu2ecef(S_VEL_ENU(east_mps, north_mps, up_mps), pos_lla);
	vx_mps = ecef.vx_ms;
	vy_mps = ecef.vy_ms;
	vz_mps = ecef.vz_ms;
}

S_TEST_ENU_VEL track_to_enu(double speed_mps, double track_deg, double pitch_deg)
{
	S_TEST_ENU_VEL velocity;
	const double track_rad = track_deg * kDegToRad;
	const double pitch_rad = pitch_deg * kDegToRad;
	const double horizontal = speed_mps * std::cos(pitch_rad);
	velocity.east_mps = horizontal * std::sin(track_rad);
	velocity.north_mps = horizontal * std::cos(track_rad);
	velocity.up_mps = speed_mps * std::sin(pitch_rad);
	velocity.speed_mps = speed_mps;
	velocity.track_deg = norm_deg_0_360(track_deg);
	velocity.pitch_deg = pitch_deg;
	return velocity;
}

QString lla_text(const S_TEST_LLA& lla)
{
	return QString("纬度 %1，经度 %2，高度 %3 m")
		.arg(f64(lla.lat_deg, 6), f64(lla.lon_deg, 6), f64(lla.alt_m, 3));
}

QString enu_velocity_text(const S_TEST_ENU_VEL& velocity)
{
	return QString("E %1 m/s，N %2 m/s，U %3 m/s；速度 %4 m/s，航迹角 %5，俯仰角 %6")
		.arg(f64(velocity.east_mps, 3), f64(velocity.north_mps, 3), f64(velocity.up_mps, 3),
			f64(velocity.speed_mps, 3), fdeg(velocity.track_deg, 3), fdeg(velocity.pitch_deg, 3));
}

QString attitude_text(float psi, float theta, float phi)
{
	return QString("Psi %1 rad，Theta %2 rad，Phi %3 rad")
		.arg(f32(psi, 6), f32(theta, 6), f32(phi, 6));
}

void append_id_fields(QVariantList& fields, const QString& prefix, const S_AFSimTestEntityIdentifier& id)
{
	fields.push_back(make_field(prefix, id.text, QStringLiteral("站点:应用:实体")));
}

void append_type_fields(QVariantList& fields, const QString& prefix, const S_AFSimTestEntityType& type)
{
	fields.push_back(make_field(prefix, type.text, QStringLiteral("种类:领域:国家:类别:子类:特定:扩展")));
}

void append_burst_fields(QVariantList& fields, const QString& prefix, const S_AFSimTestBurstDescriptor& burst)
{
	append_type_fields(fields, prefix + QStringLiteral(".实体类型"), burst.entity_type);
	fields.push_back(make_field(prefix + QStringLiteral(".战斗部"), QString::number(burst.warhead)));
	fields.push_back(make_field(prefix + QStringLiteral(".引信"), QString::number(burst.fuse)));
	fields.push_back(make_field(prefix + QStringLiteral(".数量"), QString::number(burst.quantity)));
	fields.push_back(make_field(prefix + QStringLiteral(".射速"), QString::number(burst.rate)));
}

void append_entity_state(QVariantList& fields, QVariantMap& structure, const S_PDU_ENTITY_STATE& pdu)
{
	const S_AFSimTestEntityIdentifier entity_id = entity_id_from_pdu(pdu.entity_id);
	const S_AFSimTestEntityType entity_type = entity_type_from_pdu(pdu.entity_type);
	const S_AFSimTestEntityType alternate_entity_type = entity_type_from_pdu(pdu.alt_entity_type);
	const QString marking = marking_text(pdu);

	S_TEST_LLA lla;
	S_TEST_ENU_VEL velocity;
	const bool has_lla = ecef_to_lla(pdu.entity_location_x, pdu.entity_location_y, pdu.entity_location_z, lla);
	const bool has_velocity = has_lla && ecef_velocity_to_enu(
		pdu.entity_linear_velocity_x, pdu.entity_linear_velocity_y, pdu.entity_linear_velocity_z, lla, velocity);

	append_id_fields(fields, QStringLiteral("实体ID"), entity_id);
	append_type_fields(fields, QStringLiteral("实体类型"), entity_type);
	fields.push_back(make_field(QStringLiteral("经纬高（LLA）"),
		has_lla ? lla_text(lla) : QStringLiteral("坐标无效")));
	fields.push_back(make_field(QStringLiteral("航迹速度（ENU）"),
		has_velocity ? enu_velocity_text(velocity) : QStringLiteral("速度或坐标无效")));
	fields.push_back(make_field(QStringLiteral("姿态"), attitude_text(
		pdu.entity_orientation_psi, pdu.entity_orientation_theta, pdu.entity_orientation_phi)));
	fields.push_back(make_field(QStringLiteral("实体标识"), marking.isEmpty() ? QStringLiteral("-") : marking));

	structure = QVariantMap{
		{"entity_id", id_map(entity_id)},
		{"force_id", pdu.force_id},
		{"variable_parameter_count", pdu.variable_parameter_count},
		{"entity_type", type_map(entity_type)},
		{"alternate_entity_type", type_map(alternate_entity_type)},
		{"velocity_x", pdu.entity_linear_velocity_x},
		{"velocity_y", pdu.entity_linear_velocity_y},
		{"velocity_z", pdu.entity_linear_velocity_z},
		{"location_x", pdu.entity_location_x},
		{"location_y", pdu.entity_location_y},
		{"location_z", pdu.entity_location_z},
		{"psi", pdu.entity_orientation_psi},
		{"theta", pdu.entity_orientation_theta},
		{"phi", pdu.entity_orientation_phi},
		{"appearance", pdu.entity_appearance},
		{"dead_reckoning_algorithm", pdu.dead_reckoning_algorithm},
		{"marking_character_set", pdu.entity_marking_character_set},
		{"marking", marking},
		{"capabilities", pdu.entity_capabilities},
		{"latitude_deg", lla.lat_deg},
		{"longitude_deg", lla.lon_deg},
		{"altitude_m", lla.alt_m},
		{"velocity_east_mps", velocity.east_mps},
		{"velocity_north_mps", velocity.north_mps},
		{"velocity_up_mps", velocity.up_mps},
		{"speed_mps", velocity.speed_mps},
		{"track_deg", velocity.track_deg},
		{"pitch_deg", velocity.pitch_deg}
	};
}

void append_fire(QVariantList& fields, QVariantMap& structure, const S_PDU_FIRE& pdu)
{
	const auto firing = S_AFSimTestEntityIdentifier{
		pdu.firing_entity_id.entity_identifier,
		pdu.firing_entity_id.entity_id_site,
		pdu.firing_entity_id.entity_id_application,
		pdu.firing_entity_id.entity_id_entity
	};
	const auto target = S_AFSimTestEntityIdentifier{
		pdu.target_entity_id.entity_identifier,
		pdu.target_entity_id.entity_id_site,
		pdu.target_entity_id.entity_id_application,
		pdu.target_entity_id.entity_id_entity
	};
	const auto munition = S_AFSimTestEntityIdentifier{
		pdu.munition_id.entity_identifier,
		pdu.munition_id.entity_id_site,
		pdu.munition_id.entity_id_application,
		pdu.munition_id.entity_id_entity
	};
	const auto event = S_AFSimTestEntityIdentifier{
		pdu.event_id.event_identifier,
		pdu.event_id.event_id_site,
		pdu.event_id.event_id_application,
		pdu.event_id.event_id_event
	};
	S_AFSimTestBurstDescriptor burst;
	burst.entity_type.text = pdu.burst_descriptor.entity_type_complete;
	burst.entity_type.kind = pdu.burst_descriptor.entity_type_kind;
	burst.entity_type.domain = pdu.burst_descriptor.entity_type_domain;
	burst.entity_type.country = pdu.burst_descriptor.entity_type_country;
	burst.entity_type.category = pdu.burst_descriptor.entity_type_category;
	burst.entity_type.subcategory = pdu.burst_descriptor.entity_type_subcategory;
	burst.entity_type.specific = pdu.burst_descriptor.entity_type_specific;
	burst.entity_type.extra = pdu.burst_descriptor.entity_type_extra;
	burst.warhead = pdu.burst_descriptor.warhead;
	burst.fuse = pdu.burst_descriptor.fuse;
	burst.quantity = pdu.burst_descriptor.quantity;
	burst.rate = pdu.burst_descriptor.rate;

	append_id_fields(fields, QStringLiteral("发射实体ID"), firing);
	append_id_fields(fields, QStringLiteral("目标实体ID"), target);
	append_id_fields(fields, QStringLiteral("弹药ID"), munition);
	append_id_fields(fields, QStringLiteral("事件ID"), event);
	fields.push_back(make_field(QStringLiteral("火力任务索引"), QString::number(pdu.fire_mission_index)));
	fields.push_back(make_field(QStringLiteral("发射位置X"), f64(pdu.location_in_world_coordinate_x), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("发射位置Y"), f64(pdu.location_in_world_coordinate_y), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("发射位置Z"), f64(pdu.location_in_world_coordinate_z), QStringLiteral("m")));
	append_burst_fields(fields, QStringLiteral("弹药描述符"), burst);
	fields.push_back(make_field(QStringLiteral("速度X"), f32(pdu.velocity_x), QStringLiteral("m/s")));
	fields.push_back(make_field(QStringLiteral("速度Y"), f32(pdu.velocity_y), QStringLiteral("m/s")));
	fields.push_back(make_field(QStringLiteral("速度Z"), f32(pdu.velocity_z), QStringLiteral("m/s")));
	fields.push_back(make_field(QStringLiteral("射程"), f32(pdu.range), QStringLiteral("m")));

	structure = QVariantMap{
		{"firing_entity_id", id_map(firing)},
		{"target_entity_id", id_map(target)},
		{"munition_id", id_map(munition)},
		{"event_id", id_map(event)},
		{"fire_mission_index", pdu.fire_mission_index},
		{"location_x", pdu.location_in_world_coordinate_x},
		{"location_y", pdu.location_in_world_coordinate_y},
		{"location_z", pdu.location_in_world_coordinate_z},
		{"burst_descriptor", burst_map(burst)},
		{"velocity_x", pdu.velocity_x},
		{"velocity_y", pdu.velocity_y},
		{"velocity_z", pdu.velocity_z},
		{"range", pdu.range}
	};
}

void append_detonation(QVariantList& fields, QVariantMap& structure, const S_PDU_DETONATION& pdu)
{
	const auto firing = S_AFSimTestEntityIdentifier{
		pdu.firing_entity_id.entity_identifier,
		pdu.firing_entity_id.entity_id_site,
		pdu.firing_entity_id.entity_id_application,
		pdu.firing_entity_id.entity_id_entity
	};
	const auto target = S_AFSimTestEntityIdentifier{
		pdu.target_entity_id.entity_identifier,
		pdu.target_entity_id.entity_id_site,
		pdu.target_entity_id.entity_id_application,
		pdu.target_entity_id.entity_id_entity
	};
	const auto munition = S_AFSimTestEntityIdentifier{
		pdu.munition_id.entity_identifier,
		pdu.munition_id.entity_id_site,
		pdu.munition_id.entity_id_application,
		pdu.munition_id.entity_id_entity
	};
	const auto event = S_AFSimTestEntityIdentifier{
		pdu.event_id.event_identifier,
		pdu.event_id.event_id_site,
		pdu.event_id.event_id_application,
		pdu.event_id.event_id_event
	};
	S_AFSimTestBurstDescriptor burst;
	burst.entity_type.text = pdu.burst_descriptor.entity_type_complete;
	burst.entity_type.kind = pdu.burst_descriptor.entity_type_kind;
	burst.entity_type.domain = pdu.burst_descriptor.entity_type_domain;
	burst.entity_type.country = pdu.burst_descriptor.entity_type_country;
	burst.entity_type.category = pdu.burst_descriptor.entity_type_category;
	burst.entity_type.subcategory = pdu.burst_descriptor.entity_type_subcategory;
	burst.entity_type.specific = pdu.burst_descriptor.entity_type_specific;
	burst.entity_type.extra = pdu.burst_descriptor.entity_type_extra;
	burst.warhead = pdu.burst_descriptor.warhead;
	burst.fuse = pdu.burst_descriptor.fuse;
	burst.quantity = pdu.burst_descriptor.quantity;
	burst.rate = pdu.burst_descriptor.rate;

	append_id_fields(fields, QStringLiteral("发射实体ID"), firing);
	append_id_fields(fields, QStringLiteral("目标实体ID"), target);
	append_id_fields(fields, QStringLiteral("弹药ID"), munition);
	append_id_fields(fields, QStringLiteral("事件ID"), event);
	fields.push_back(make_field(QStringLiteral("速度X"), f32(pdu.velocity_x), QStringLiteral("m/s")));
	fields.push_back(make_field(QStringLiteral("速度Y"), f32(pdu.velocity_y), QStringLiteral("m/s")));
	fields.push_back(make_field(QStringLiteral("速度Z"), f32(pdu.velocity_z), QStringLiteral("m/s")));
	fields.push_back(make_field(QStringLiteral("爆炸位置X"), f64(pdu.location_in_world_coordinate_x), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("爆炸位置Y"), f64(pdu.location_in_world_coordinate_y), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("爆炸位置Z"), f64(pdu.location_in_world_coordinate_z), QStringLiteral("m")));
	append_burst_fields(fields, QStringLiteral("弹药描述符"), burst);
	fields.push_back(make_field(QStringLiteral("实体坐标X"), f32(pdu.location_in_entity_coordinate_x), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("实体坐标Y"), f32(pdu.location_in_entity_coordinate_y), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("实体坐标Z"), f32(pdu.location_in_entity_coordinate_z), QStringLiteral("m")));
	fields.push_back(make_field(QStringLiteral("爆炸结果"), QString::number(pdu.detonation_result)));
	fields.push_back(make_field(QStringLiteral("可变参数数量"), QString::number(pdu.variable_parameter_count)));
	fields.push_back(make_field(QStringLiteral("填充"), QString::number(pdu.padding)));

	structure = QVariantMap{
		{"firing_entity_id", id_map(firing)},
		{"target_entity_id", id_map(target)},
		{"munition_id", id_map(munition)},
		{"event_id", id_map(event)},
		{"velocity_x", pdu.velocity_x},
		{"velocity_y", pdu.velocity_y},
		{"velocity_z", pdu.velocity_z},
		{"location_x", pdu.location_in_world_coordinate_x},
		{"location_y", pdu.location_in_world_coordinate_y},
		{"location_z", pdu.location_in_world_coordinate_z},
		{"burst_descriptor", burst_map(burst)},
		{"entity_location_x", pdu.location_in_entity_coordinate_x},
		{"entity_location_y", pdu.location_in_entity_coordinate_y},
		{"entity_location_z", pdu.location_in_entity_coordinate_z},
		{"detonation_result", pdu.detonation_result},
		{"variable_parameter_count", pdu.variable_parameter_count}
	};
}

int emission_beam_count(const S_PDU_ELC_EMISSION& pdu)
{
	int count = 0;
	for (const auto& system : pdu.record) {
		count += static_cast<int>(system.beam_records.size());
	}
	return count;
}

void append_emission(QVariantList& fields, QVariantMap& structure, const S_PDU_ELC_EMISSION& pdu)
{
	const auto emitting = S_AFSimTestEntityIdentifier{
		pdu.emitting_entity_id.entity_identifier,
		pdu.emitting_entity_id.entity_id_site,
		pdu.emitting_entity_id.entity_id_application,
		pdu.emitting_entity_id.entity_id_entity
	};
	const auto event = S_AFSimTestEntityIdentifier{
		pdu.event_id.event_identifier,
		pdu.event_id.event_id_site,
		pdu.event_id.event_id_application,
		pdu.event_id.event_id_event
	};

	append_id_fields(fields, QStringLiteral("辐射实体ID"), emitting);
	append_id_fields(fields, QStringLiteral("事件ID"), event);
	fields.push_back(make_field(QStringLiteral("状态更新指示"), QString::number(pdu.state_update_indicator), offset_note(24)));
	fields.push_back(make_field(QStringLiteral("辐射系统数量"), QString::number(pdu.number_of_systems), offset_note(25)));
	fields.push_back(make_field(QStringLiteral("填充"), QString::number(pdu.padding), offset_note(26, 2)));
	fields.push_back(make_field(QStringLiteral("变长记录总长度"), QString("%1 字节").arg(pdu.emitter_system_records.size())));

	QVariantList systems;
	for (int system_index = 0; system_index < static_cast<int>(pdu.record.size()); ++system_index) {
		const auto& system = pdu.record[system_index];
		const QString system_prefix = QString("辐射系统[%1]").arg(system_index + 1);
		fields.push_back(make_field(system_prefix + QStringLiteral(".数据长度"),
			QString("%1 个32位字 / %2 字节").arg(system.data_length).arg(system.data_length * 4),
			offset_note(system.start_offset)));
		fields.push_back(make_field(system_prefix + QStringLiteral(".波束数量"),
			QString::number(system.number_of_beams), offset_note(system.start_offset + 1)));
		fields.push_back(make_field(system_prefix + QStringLiteral(".辐射源名称"),
			QString::number(system.emitter_name), offset_note(system.start_offset + 4, 2)));
		fields.push_back(make_field(system_prefix + QStringLiteral(".辐射源功能"),
			QString::number(system.emitter_function), offset_note(system.start_offset + 6)));
		fields.push_back(make_field(system_prefix + QStringLiteral(".辐射源编号"),
			QString::number(system.emitter_number), offset_note(system.start_offset + 7)));
		fields.push_back(make_field(system_prefix + QStringLiteral(".安装位置X"), f32(system.location_x), QStringLiteral("m")));
		fields.push_back(make_field(system_prefix + QStringLiteral(".安装位置Y"), f32(system.location_y), QStringLiteral("m")));
		fields.push_back(make_field(system_prefix + QStringLiteral(".安装位置Z"), f32(system.location_z), QStringLiteral("m")));

		QVariantList beams;
		for (int beam_index = 0; beam_index < static_cast<int>(system.beam_records.size()); ++beam_index) {
			const auto& beam = system.beam_records[beam_index];
			const QString beam_prefix = system_prefix + QString(".波束[%1]").arg(beam_index + 1);
			fields.push_back(make_field(beam_prefix + QStringLiteral(".数据长度"),
				QString("%1 个32位字 / %2 字节").arg(beam.data_length).arg(beam.data_length * 4),
				offset_note(beam.start_offset)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".波束编号"), QString::number(beam.beam_number),
				offset_note(beam.start_offset + 1)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".参数索引"), QString::number(beam.beam_parameter_index),
				offset_note(beam.start_offset + 2, 2)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".频率"), f32(beam.fundamental_parameter_frequency), QStringLiteral("Hz")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".频率范围"), f32(beam.fundamental_parameter_frequency_range), QStringLiteral("Hz")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".有效辐射功率"), f32(beam.fundamental_parameter_effective_radiated_power), QStringLiteral("dBm")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".脉冲重复频率"), f32(beam.fundamental_parameter_pulse_repetition_frequency), QStringLiteral("Hz")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".脉宽"), f32(beam.fundamental_parameter_pulse_width), QStringLiteral("us")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".方位中心"), f32(beam.beam_data_azimuth_center, 6), QStringLiteral("rad")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".方位扫描半角"), f32(beam.beam_data_azimuth_sweep, 6), QStringLiteral("rad")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".俯仰中心"), f32(beam.beam_data_elevation_center, 6), QStringLiteral("rad")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".俯仰扫描半角"), f32(beam.beam_data_elevation_sweep, 6), QStringLiteral("rad")));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".扫描同步"), f32(beam.beam_data_sweep_sync, 6)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".波束功能"), QString::number(beam.beam_function)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".目标数量"), QString::number(beam.number_of_targets)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".高密度跟踪/干扰"), QString::number(beam.high_density_track_jam)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".波束状态"), QString::number(beam.beam_status)));
			fields.push_back(make_field(beam_prefix + QStringLiteral(".干扰技术"),
				QString("%1:%2:%3:%4").arg(beam.jamming_technique_kind).arg(beam.jamming_technique_category)
					.arg(beam.jamming_technique_subcategory).arg(beam.jamming_technique_specific)));

			QVariantList targets;
			for (int target_index = 0; target_index < static_cast<int>(beam.target_records.size()); ++target_index) {
				const auto& target = beam.target_records[target_index];
				const QString target_prefix = beam_prefix + QString(".跟踪干扰目标[%1]").arg(target_index + 1);
				fields.push_back(make_field(target_prefix + QStringLiteral(".实体ID"), target.entity_identifier,
					offset_note(target.start_offset, 6)));
				fields.push_back(make_field(target_prefix + QStringLiteral(".辐射源编号"), QString::number(target.emitter_number),
					offset_note(target.start_offset + 6)));
				fields.push_back(make_field(target_prefix + QStringLiteral(".波束编号"), QString::number(target.beam_number),
					offset_note(target.start_offset + 7)));
				targets.push_back(QVariantMap{
					{"entity_text", target.entity_identifier},
					{"site", target.entity_id_site},
					{"application", target.entity_id_application},
					{"entity", target.entity_id_entity},
					{"emitter_number", target.emitter_number},
					{"beam_number", target.beam_number}
				});
			}
			beams.push_back(QVariantMap{
				{"beam_number", beam.beam_number},
				{"beam_function", beam.beam_function},
				{"number_of_targets", beam.number_of_targets},
				{"targets", targets}
			});
		}
		systems.push_back(QVariantMap{
			{"emitter_name", system.emitter_name},
			{"emitter_function", system.emitter_function},
			{"emitter_number", system.emitter_number},
			{"number_of_beams", system.number_of_beams},
			{"beams", beams}
		});
	}

	for (const QString& warning : pdu.parse_warnings) {
		fields.push_back(make_field(QStringLiteral("解析告警"), warning));
	}

	structure = QVariantMap{
		{"emitting_entity_id", id_map(emitting)},
		{"event_id", id_map(event)},
		{"state_update_indicator", pdu.state_update_indicator},
		{"number_of_systems", pdu.number_of_systems},
		{"padding", pdu.padding},
		{"emitter_system_records_hex", bytes_to_hex(pdu.emitter_system_records)},
		{"emitter_systems", systems},
		{"warnings", pdu.parse_warnings}
	};
}

QString entity_state_summary(const S_PDU_ENTITY_STATE& pdu)
{
	S_TEST_LLA lla;
	const bool has_lla = ecef_to_lla(pdu.entity_location_x, pdu.entity_location_y, pdu.entity_location_z, lla);
	return QString("实体 %1，类型 %2，阵营 %3，%4")
		.arg(pdu.entity_id.entity_identifier,
			pdu.entity_type.entity_type_complete,
			QString::number(pdu.force_id),
			has_lla ? lla_text(lla) : QStringLiteral("坐标无效"));
}

QString fire_summary(const S_PDU_FIRE& pdu)
{
	return QString("发射 %1 -> %2，弹药 %3，事件 %4，射程 %5 m")
		.arg(pdu.firing_entity_id.entity_identifier,
			pdu.target_entity_id.entity_identifier,
			pdu.munition_id.entity_identifier,
			pdu.event_id.event_identifier,
			f32(pdu.range));
}

QString detonation_summary(const S_PDU_DETONATION& pdu)
{
	return QString("爆炸 %1 -> %2，弹药 %3，事件 %4，结果 %5")
		.arg(pdu.firing_entity_id.entity_identifier,
			pdu.target_entity_id.entity_identifier,
			pdu.munition_id.entity_identifier,
			pdu.event_id.event_identifier)
		.arg(pdu.detonation_result);
}

QString emission_summary(const S_PDU_ELC_EMISSION& pdu)
{
	return QString("电磁辐射实体 %1，系统 %2，波束 %3")
		.arg(pdu.emitting_entity_id.entity_identifier)
		.arg(pdu.record.size())
		.arg(emission_beam_count(pdu));
}

bool parse_header(const QByteArray& data, S_PDU_HEADER& header)
{
	if (data.size() < DIS_PDU_HEADER_LEN) {
		return false;
	}
	UtilDisPdu::recv_pdu_header(data, header);
	return true;
}

bool parse_fire(const QByteArray& data, S_PDU_FIRE& pdu)
{
	if (data.size() < DIS_PDU_FIRE_LEN || !parse_header(data, pdu.header)) {
		return false;
	}
	const int b = DIS_PDU_HEADER_LEN;
	pdu.firing_entity_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b);
	pdu.firing_entity_id.entity_id_site = UtilDisPdu::read_u16(data, b);
	pdu.firing_entity_id.entity_id_application = UtilDisPdu::read_u16(data, b + 2);
	pdu.firing_entity_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 4);
	pdu.target_entity_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b + 6);
	pdu.target_entity_id.entity_id_site = UtilDisPdu::read_u16(data, b + 6);
	pdu.target_entity_id.entity_id_application = UtilDisPdu::read_u16(data, b + 8);
	pdu.target_entity_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 10);
	pdu.munition_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b + 12);
	pdu.munition_id.entity_id_site = UtilDisPdu::read_u16(data, b + 12);
	pdu.munition_id.entity_id_application = UtilDisPdu::read_u16(data, b + 14);
	pdu.munition_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 16);
	pdu.event_id.event_identifier = UtilDisPdu::get_dis_ori_id(data, b + 18);
	pdu.event_id.event_id_site = UtilDisPdu::read_u16(data, b + 18);
	pdu.event_id.event_id_application = UtilDisPdu::read_u16(data, b + 20);
	pdu.event_id.event_id_event = UtilDisPdu::read_u16(data, b + 22);
	pdu.fire_mission_index = UtilDisPdu::read_u32(data, b + 24);
	pdu.location_in_world_coordinate_x = UtilDisPdu::read_f64(data, b + 28);
	pdu.location_in_world_coordinate_y = UtilDisPdu::read_f64(data, b + 36);
	pdu.location_in_world_coordinate_z = UtilDisPdu::read_f64(data, b + 44);
	pdu.burst_descriptor.entity_type_complete = UtilDisPdu::get_dis_ori_type(data, b + 52);
	pdu.burst_descriptor.entity_type_kind = UtilDisPdu::read_u8(data, b + 52);
	pdu.burst_descriptor.entity_type_domain = UtilDisPdu::read_u8(data, b + 53);
	pdu.burst_descriptor.entity_type_country = UtilDisPdu::read_u16(data, b + 54);
	pdu.burst_descriptor.entity_type_category = UtilDisPdu::read_u8(data, b + 56);
	pdu.burst_descriptor.entity_type_subcategory = UtilDisPdu::read_u8(data, b + 57);
	pdu.burst_descriptor.entity_type_specific = UtilDisPdu::read_u8(data, b + 58);
	pdu.burst_descriptor.entity_type_extra = UtilDisPdu::read_u8(data, b + 59);
	pdu.burst_descriptor.warhead = UtilDisPdu::read_u16(data, b + 60);
	pdu.burst_descriptor.fuse = UtilDisPdu::read_u16(data, b + 62);
	pdu.burst_descriptor.quantity = UtilDisPdu::read_u16(data, b + 64);
	pdu.burst_descriptor.rate = UtilDisPdu::read_u16(data, b + 66);
	pdu.velocity_x = UtilDisPdu::read_f32(data, b + 68);
	pdu.velocity_y = UtilDisPdu::read_f32(data, b + 72);
	pdu.velocity_z = UtilDisPdu::read_f32(data, b + 76);
	pdu.range = UtilDisPdu::read_f32(data, b + 80);
	return true;
}

bool parse_detonation(const QByteArray& data, S_PDU_DETONATION& pdu)
{
	if (data.size() < DIS_PDU_DETONATION_LEN || !parse_header(data, pdu.header)) {
		return false;
	}
	const int b = DIS_PDU_HEADER_LEN;
	pdu.firing_entity_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b);
	pdu.firing_entity_id.entity_id_site = UtilDisPdu::read_u16(data, b);
	pdu.firing_entity_id.entity_id_application = UtilDisPdu::read_u16(data, b + 2);
	pdu.firing_entity_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 4);
	pdu.target_entity_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b + 6);
	pdu.target_entity_id.entity_id_site = UtilDisPdu::read_u16(data, b + 6);
	pdu.target_entity_id.entity_id_application = UtilDisPdu::read_u16(data, b + 8);
	pdu.target_entity_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 10);
	pdu.munition_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b + 12);
	pdu.munition_id.entity_id_site = UtilDisPdu::read_u16(data, b + 12);
	pdu.munition_id.entity_id_application = UtilDisPdu::read_u16(data, b + 14);
	pdu.munition_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 16);
	pdu.event_id.event_identifier = UtilDisPdu::get_dis_ori_id(data, b + 18);
	pdu.event_id.event_id_site = UtilDisPdu::read_u16(data, b + 18);
	pdu.event_id.event_id_application = UtilDisPdu::read_u16(data, b + 20);
	pdu.event_id.event_id_event = UtilDisPdu::read_u16(data, b + 22);
	pdu.velocity_x = UtilDisPdu::read_f32(data, b + 24);
	pdu.velocity_y = UtilDisPdu::read_f32(data, b + 28);
	pdu.velocity_z = UtilDisPdu::read_f32(data, b + 32);
	pdu.location_in_world_coordinate_x = UtilDisPdu::read_f64(data, b + 36);
	pdu.location_in_world_coordinate_y = UtilDisPdu::read_f64(data, b + 44);
	pdu.location_in_world_coordinate_z = UtilDisPdu::read_f64(data, b + 52);
	pdu.burst_descriptor.entity_type_complete = UtilDisPdu::get_dis_ori_type(data, b + 60);
	pdu.burst_descriptor.entity_type_kind = UtilDisPdu::read_u8(data, b + 60);
	pdu.burst_descriptor.entity_type_domain = UtilDisPdu::read_u8(data, b + 61);
	pdu.burst_descriptor.entity_type_country = UtilDisPdu::read_u16(data, b + 62);
	pdu.burst_descriptor.entity_type_category = UtilDisPdu::read_u8(data, b + 64);
	pdu.burst_descriptor.entity_type_subcategory = UtilDisPdu::read_u8(data, b + 65);
	pdu.burst_descriptor.entity_type_specific = UtilDisPdu::read_u8(data, b + 66);
	pdu.burst_descriptor.entity_type_extra = UtilDisPdu::read_u8(data, b + 67);
	pdu.burst_descriptor.warhead = UtilDisPdu::read_u16(data, b + 68);
	pdu.burst_descriptor.fuse = UtilDisPdu::read_u16(data, b + 70);
	pdu.burst_descriptor.quantity = UtilDisPdu::read_u16(data, b + 72);
	pdu.burst_descriptor.rate = UtilDisPdu::read_u16(data, b + 74);
	pdu.location_in_entity_coordinate_x = UtilDisPdu::read_f32(data, b + 76);
	pdu.location_in_entity_coordinate_y = UtilDisPdu::read_f32(data, b + 80);
	pdu.location_in_entity_coordinate_z = UtilDisPdu::read_f32(data, b + 84);
	pdu.detonation_result = UtilDisPdu::read_u8(data, b + 88);
	pdu.variable_parameter_count = UtilDisPdu::read_u8(data, b + 89);
	pdu.padding = UtilDisPdu::read_u16(data, b + 90);
	return true;
}

bool parse_emission(const QByteArray& data, S_PDU_ELC_EMISSION& pdu)
{
	if (data.size() < DIS_PDU_ELC_EMISSION_FIXED_LEN || !parse_header(data, pdu.header)) {
		return false;
	}
	const int b = DIS_PDU_HEADER_LEN;
	pdu.emitting_entity_id.entity_identifier = UtilDisPdu::get_dis_ori_id(data, b);
	pdu.emitting_entity_id.entity_id_site = UtilDisPdu::read_u16(data, b);
	pdu.emitting_entity_id.entity_id_application = UtilDisPdu::read_u16(data, b + 2);
	pdu.emitting_entity_id.entity_id_entity = UtilDisPdu::read_u16(data, b + 4);
	pdu.event_id.event_identifier = UtilDisPdu::get_dis_ori_id(data, b + 6);
	pdu.event_id.event_id_site = UtilDisPdu::read_u16(data, b + 6);
	pdu.event_id.event_id_application = UtilDisPdu::read_u16(data, b + 8);
	pdu.event_id.event_id_event = UtilDisPdu::read_u16(data, b + 10);
	pdu.state_update_indicator = UtilDisPdu::read_u8(data, b + 12);
	pdu.number_of_systems = UtilDisPdu::read_u8(data, b + 13);
	pdu.padding = UtilDisPdu::read_u16(data, b + 14);
	pdu.emitter_system_records = data.mid(DIS_PDU_ELC_EMISSION_FIXED_LEN);
	pdu.record.clear();
	pdu.parse_warnings.clear();

	int offset = DIS_PDU_ELC_EMISSION_FIXED_LEN;
	for (int system_index = 0; system_index < pdu.number_of_systems; ++system_index) {
		if (!has_bytes(data, offset, 20)) {
			pdu.parse_warnings.push_back(QString("辐射系统[%1] 长度不足：偏移 +%2 后剩余 %3 字节，至少需要 20 字节。")
				.arg(system_index + 1).arg(offset).arg(remaining_bytes(data, offset)));
			break;
		}

		S_PDU_ELC_EMISSION::S_EMITTER_SYSTEM_RECORD system;
		system.start_offset = offset;
		system.data_length = UtilDisPdu::read_u8(data, offset);
		system.number_of_beams = UtilDisPdu::read_u8(data, offset + 1);
		system.padding = UtilDisPdu::read_u16(data, offset + 2);
		system.emitter_name = UtilDisPdu::read_u16(data, offset + 4);
		system.emitter_function = UtilDisPdu::read_u8(data, offset + 6);
		system.emitter_number = UtilDisPdu::read_u8(data, offset + 7);
		system.location_x = UtilDisPdu::read_f32(data, offset + 8);
		system.location_y = UtilDisPdu::read_f32(data, offset + 12);
		system.location_z = UtilDisPdu::read_f32(data, offset + 16);
		offset += 20;

		bool truncated = false;
		for (int beam_index = 0; beam_index < system.number_of_beams; ++beam_index) {
			if (!has_bytes(data, offset, 52)) {
				pdu.parse_warnings.push_back(QString("辐射系统[%1].波束[%2] 长度不足：偏移 +%3 后剩余 %4 字节，至少需要 52 字节。")
					.arg(system_index + 1).arg(beam_index + 1).arg(offset).arg(remaining_bytes(data, offset)));
				truncated = true;
				break;
			}

			S_PDU_ELC_EMISSION::S_BEAM_RECORD beam;
			beam.start_offset = offset;
			beam.data_length = UtilDisPdu::read_u8(data, offset);
			beam.beam_number = UtilDisPdu::read_u8(data, offset + 1);
			beam.beam_parameter_index = UtilDisPdu::read_u16(data, offset + 2);
			beam.fundamental_parameter_frequency = UtilDisPdu::read_f32(data, offset + 4);
			beam.fundamental_parameter_frequency_range = UtilDisPdu::read_f32(data, offset + 8);
			beam.fundamental_parameter_effective_radiated_power = UtilDisPdu::read_f32(data, offset + 12);
			beam.fundamental_parameter_pulse_repetition_frequency = UtilDisPdu::read_f32(data, offset + 16);
			beam.fundamental_parameter_pulse_width = UtilDisPdu::read_f32(data, offset + 20);
			beam.beam_data_azimuth_center = UtilDisPdu::read_f32(data, offset + 24);
			beam.beam_data_azimuth_sweep = UtilDisPdu::read_f32(data, offset + 28);
			beam.beam_data_elevation_center = UtilDisPdu::read_f32(data, offset + 32);
			beam.beam_data_elevation_sweep = UtilDisPdu::read_f32(data, offset + 36);
			beam.beam_data_sweep_sync = UtilDisPdu::read_f32(data, offset + 40);
			beam.beam_function = UtilDisPdu::read_u8(data, offset + 44);
			beam.number_of_targets = UtilDisPdu::read_u8(data, offset + 45);
			beam.high_density_track_jam = UtilDisPdu::read_u8(data, offset + 46);
			beam.beam_status = UtilDisPdu::read_u8(data, offset + 47);
			beam.jamming_technique_kind = UtilDisPdu::read_u8(data, offset + 48);
			beam.jamming_technique_category = UtilDisPdu::read_u8(data, offset + 49);
			beam.jamming_technique_subcategory = UtilDisPdu::read_u8(data, offset + 50);
			beam.jamming_technique_specific = UtilDisPdu::read_u8(data, offset + 51);
			offset += 52;

			for (int target_index = 0; target_index < beam.number_of_targets; ++target_index) {
				if (!has_bytes(data, offset, 8)) {
					pdu.parse_warnings.push_back(QString("辐射系统[%1].波束[%2].目标[%3] 长度不足：偏移 +%4 后剩余 %5 字节，至少需要 8 字节。")
						.arg(system_index + 1).arg(beam_index + 1).arg(target_index + 1).arg(offset)
						.arg(remaining_bytes(data, offset)));
					truncated = true;
					break;
				}
				S_PDU_ELC_EMISSION::S_TRACK_JAM_TARGET_RECORD target;
				target.start_offset = offset;
				target.entity_identifier = UtilDisPdu::get_dis_ori_id(data, offset);
				target.entity_id_site = UtilDisPdu::read_u16(data, offset);
				target.entity_id_application = UtilDisPdu::read_u16(data, offset + 2);
				target.entity_id_entity = UtilDisPdu::read_u16(data, offset + 4);
				target.emitter_number = UtilDisPdu::read_u8(data, offset + 6);
				target.beam_number = UtilDisPdu::read_u8(data, offset + 7);
				beam.target_records.push_back(target);
				offset += 8;
			}

			const int beam_words = beam.data_length;
			if (beam_words > 0) {
				const int expected_size = beam_words * 4;
				const int read_size = offset - beam.start_offset;
				const int extra_size = expected_size - read_size;
				if (extra_size > 0) {
					if (has_bytes(data, offset, extra_size)) {
						beam.extra_data = data.mid(offset, extra_size);
						offset += extra_size;
					}
					else {
						pdu.parse_warnings.push_back(QString("辐射系统[%1].波束[%2] 声明长度还剩 %3 字节，但报文剩余不足。")
							.arg(system_index + 1).arg(beam_index + 1).arg(extra_size));
						truncated = true;
					}
				}
				else if (extra_size < 0) {
					pdu.parse_warnings.push_back(QString("辐射系统[%1].波束[%2] 声明长度 %3 字节，小于已解析长度 %4 字节。")
						.arg(system_index + 1).arg(beam_index + 1).arg(expected_size).arg(read_size));
				}
			}

			system.beam_records.push_back(beam);
			if (truncated) break;
		}

		const int system_words = system.data_length;
		if (system_words > 0) {
			const int expected_size = system_words * 4;
			const int read_size = offset - system.start_offset;
			const int extra_size = expected_size - read_size;
			if (extra_size > 0) {
				if (has_bytes(data, offset, extra_size)) {
					system.extra_data = data.mid(offset, extra_size);
					offset += extra_size;
				}
				else {
					pdu.parse_warnings.push_back(QString("辐射系统[%1] 声明长度还剩 %2 字节，但报文剩余不足。")
						.arg(system_index + 1).arg(extra_size));
					truncated = true;
				}
			}
			else if (extra_size < 0) {
				pdu.parse_warnings.push_back(QString("辐射系统[%1] 声明长度 %2 字节，小于已解析长度 %3 字节。")
					.arg(system_index + 1).arg(expected_size).arg(read_size));
			}
		}

		pdu.record.push_back(system);
		if (truncated) break;
	}

	if (offset < data.size()) {
		pdu.parse_warnings.push_back(QString("PDU-23 末尾仍有 %1 字节未归属到系统记录。").arg(data.size() - offset));
	}
	return true;
}

S_AFSimTestDecodeResult decode_datagram(const QByteArray& data, const QString& direction,
	const QString& endpoint, qint64 time_ms)
{
	S_AFSimTestDecodeResult result;
	result.row["time"] = QDateTime::fromMSecsSinceEpoch(time_ms).toString("hh:mm:ss.zzz");
	result.row["direction"] = direction;
	result.row["endpoint"] = endpoint;
	result.row["length"] = data.size();
	result.row["hex"] = bytes_to_hex(data);

	S_PDU_HEADER header;
	if (!parse_header(data, header)) {
		result.error = QString("DIS 报文头长度不足：%1 字节").arg(data.size());
		result.summary = result.error;
		result.row["pduType"] = "-";
		result.row["summary"] = result.summary;
		result.row["status"] = "error";
		return result;
	}

	result.pdu_type = header.pdu_type;
	result.type_name = pdu_type_name(header.pdu_type);
	result.structure_name = pdu_structure_name(header.pdu_type);
	result.ok = true;

	if (header.pdu_length != 0 && header.pdu_length != data.size()) {
		result.fields.push_back(make_field(QStringLiteral("数据长度告警"),
			QString("PDU 声明长度 %1 字节，实际报文 %2 字节").arg(header.pdu_length).arg(data.size())));
	}

	switch (header.pdu_type) {
	case 1: {
		S_PDU_ENTITY_STATE pdu;
		if (!UtilDisPdu::deal_pdu_entity_state(data, pdu)) {
			result.ok = false;
			result.error = QString("实体状态 PDU 长度不足：%1 字节，需要 %2 字节").arg(data.size()).arg(DIS_PDU_ENTITY_STATE_LEN);
			result.summary = result.error;
		}
		else {
			result.summary = entity_state_summary(pdu);
			append_entity_state(result.fields, result.structure, pdu);
		}
		break;
	}
	case 2: {
		S_PDU_FIRE pdu;
		if (!parse_fire(data, pdu)) {
			result.ok = false;
			result.error = QString("发射 PDU 长度不足：%1 字节，需要 %2 字节").arg(data.size()).arg(DIS_PDU_FIRE_LEN);
			result.summary = result.error;
		}
		else {
			result.summary = fire_summary(pdu);
			append_fire(result.fields, result.structure, pdu);
		}
		break;
	}
	case 3: {
		S_PDU_DETONATION pdu;
		if (!parse_detonation(data, pdu)) {
			result.ok = false;
			result.error = QString("爆炸 PDU 长度不足：%1 字节，需要 %2 字节").arg(data.size()).arg(DIS_PDU_DETONATION_LEN);
			result.summary = result.error;
		}
		else {
			result.summary = detonation_summary(pdu);
			append_detonation(result.fields, result.structure, pdu);
		}
		break;
	}
	case 23: {
		S_PDU_ELC_EMISSION pdu;
		if (!parse_emission(data, pdu)) {
			result.ok = false;
			result.error = QString("电磁辐射 PDU 长度不足：%1 字节，需要 %2 字节")
				.arg(data.size()).arg(DIS_PDU_ELC_EMISSION_FIXED_LEN);
			result.summary = result.error;
		}
		else {
			result.summary = emission_summary(pdu);
			append_emission(result.fields, result.structure, pdu);
		}
		break;
	}
	default:
		result.summary = QString("DIS PDU-%1 已丢弃，%2 字节。").arg(header.pdu_type, 2, 10, QChar('0')).arg(data.size());
		break;
	}

	result.row["pduType"] = QString("PDU-%1").arg(header.pdu_type, 2, 10, QChar('0'));
	result.row["pduTypeNumber"] = header.pdu_type;
	result.row["typeName"] = result.type_name;
	result.row["structureName"] = result.structure_name;
	result.row["summary"] = result.summary;
	result.row["status"] = result.ok ? "ok" : "error";
	result.row["fields"] = result.fields;
	result.row["structure"] = result.structure;
	return result;
}

QByteArray build_entity_state_packet(quint8 exercise_id, quint8 force_id,
	const S_AFSimTestEntityIdentifier& id, const S_AFSimTestEntityType& type,
	double location_x, double location_y, double location_z,
	float velocity_x, float velocity_y, float velocity_z,
	float psi, float theta, float phi, const QString& marking)
{
	QByteArray data(DIS_PDU_ENTITY_STATE_LEN, '\0');
	write_header(data, exercise_id, 1, 1, DIS_PDU_ENTITY_STATE_LEN);

	const int b = DIS_PDU_HEADER_LEN;
	write_u16(data, b, id.site);
	write_u16(data, b + 2, id.application);
	write_u16(data, b + 4, id.entity);
	write_u8(data, b + 6, force_id);
	write_u8(data, b + 7, 0);
	write_entity_type(data, b + 8, type);
	write_entity_type(data, b + 16, type);
	write_f32(data, b + 24, velocity_x);
	write_f32(data, b + 28, velocity_y);
	write_f32(data, b + 32, velocity_z);
	write_f64(data, b + 36, location_x);
	write_f64(data, b + 44, location_y);
	write_f64(data, b + 52, location_z);
	write_f32(data, b + 60, psi);
	write_f32(data, b + 64, theta);
	write_f32(data, b + 68, phi);
	write_u32(data, b + 72, 0);
	write_u8(data, b + 76, 1);
	write_u8(data, b + 116, 1);

	QByteArray marking_data = marking.toLatin1().left(11);
	for (int i = 0; i < marking_data.size(); ++i) {
		write_u8(data, b + 117 + i, static_cast<quint8>(marking_data.at(i)));
	}
	write_u32(data, b + 128, 0);
	return data;
}

QByteArray build_fire_packet(quint8 exercise_id, const S_AFSimTestEntityIdentifier& firing,
	const S_AFSimTestEntityIdentifier& target, const S_AFSimTestEntityIdentifier& munition,
	const S_AFSimTestEntityIdentifier& event, quint32 fire_mission_index,
	double location_x, double location_y, double location_z,
	const S_AFSimTestBurstDescriptor& burst, float velocity_x, float velocity_y, float velocity_z, float range)
{
	QByteArray data(DIS_PDU_FIRE_LEN, '\0');
	write_header(data, exercise_id, 2, 2, DIS_PDU_FIRE_LEN);
	const int b = DIS_PDU_HEADER_LEN;
	write_entity_id(data, b, firing);
	write_entity_id(data, b + 6, target);
	write_entity_id(data, b + 12, munition);
	write_entity_id(data, b + 18, event);
	write_u32(data, b + 24, fire_mission_index);
	write_f64(data, b + 28, location_x);
	write_f64(data, b + 36, location_y);
	write_f64(data, b + 44, location_z);
	write_burst_descriptor(data, b + 52, burst);
	write_f32(data, b + 68, velocity_x);
	write_f32(data, b + 72, velocity_y);
	write_f32(data, b + 76, velocity_z);
	write_f32(data, b + 80, range);
	return data;
}

QByteArray build_detonation_packet(quint8 exercise_id, const S_AFSimTestEntityIdentifier& firing,
	const S_AFSimTestEntityIdentifier& target, const S_AFSimTestEntityIdentifier& munition,
	const S_AFSimTestEntityIdentifier& event, float velocity_x, float velocity_y, float velocity_z,
	double location_x, double location_y, double location_z,
	const S_AFSimTestBurstDescriptor& burst, float entity_location_x, float entity_location_y,
	float entity_location_z, quint8 detonation_result)
{
	QByteArray data(DIS_PDU_DETONATION_LEN, '\0');
	write_header(data, exercise_id, 3, 2, DIS_PDU_DETONATION_LEN);
	const int b = DIS_PDU_HEADER_LEN;
	write_entity_id(data, b, firing);
	write_entity_id(data, b + 6, target);
	write_entity_id(data, b + 12, munition);
	write_entity_id(data, b + 18, event);
	write_f32(data, b + 24, velocity_x);
	write_f32(data, b + 28, velocity_y);
	write_f32(data, b + 32, velocity_z);
	write_f64(data, b + 36, location_x);
	write_f64(data, b + 44, location_y);
	write_f64(data, b + 52, location_z);
	write_burst_descriptor(data, b + 60, burst);
	write_f32(data, b + 76, entity_location_x);
	write_f32(data, b + 80, entity_location_y);
	write_f32(data, b + 84, entity_location_z);
	write_u8(data, b + 88, detonation_result);
	write_u8(data, b + 89, 0);
	write_u16(data, b + 90, 0);
	return data;
}

QByteArray build_emission_packet(quint8 exercise_id, const S_AFSimTestEntityIdentifier& emitting,
	const S_AFSimTestEntityIdentifier& event, quint8 state_update_indicator,
	quint8 number_of_systems, const QByteArray& records)
{
	const quint16 len = static_cast<quint16>(DIS_PDU_ELC_EMISSION_FIXED_LEN + records.size());
	QByteArray data(len, '\0');
	write_header(data, exercise_id, 23, 6, len);
	const int b = DIS_PDU_HEADER_LEN;
	write_entity_id(data, b, emitting);
	write_entity_id(data, b + 6, event);
	write_u8(data, b + 12, state_update_indicator);
	write_u8(data, b + 13, number_of_systems);
	write_u16(data, b + 14, 0);
	if (!records.isEmpty()) {
		std::memcpy(data.data() + DIS_PDU_ELC_EMISSION_FIXED_LEN, records.constData(),
			static_cast<size_t>(records.size()));
	}
	return data;
}

bool local_frame_from_ecef(double x, double y, double z, std::array<double, 3>& east, std::array<double, 3>& south)
{
	if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
	if (std::hypot(std::hypot(x, y), z) < 1.0) return false;

	const double p = std::hypot(x, y);
	const double lon = std::atan2(y, x);
	double lat = p < 1e-6 ? (z >= 0.0 ? kPi * 0.5 : -kPi * 0.5)
		: std::atan2(z, p * (1.0 - WGS84_E2));

	for (int i = 0; i < 5 && p >= 1e-6; ++i) {
		const double sin_lat = std::sin(lat);
		const double cos_lat = std::cos(lat);
		const double n = WGS84_A_M / std::sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);
		const double h = p / std::max(std::abs(cos_lat), 1e-12) - n;
		lat = std::atan2(z, p * (1.0 - WGS84_E2 * n / (n + h)));
	}

	east = { -std::sin(lon), std::cos(lon), 0.0 };
	const std::array<double, 3> north = {
		-std::sin(lat) * std::cos(lon),
		-std::sin(lat) * std::sin(lon),
		std::cos(lat)
	};
	south = { -north[0], -north[1], -north[2] };
	return true;
}

bool build_derived_aircraft_packet(const QByteArray& source_data, QByteArray& packet, QString& error)
{
	if (source_data.size() < DIS_PDU_ENTITY_STATE_LEN) {
		error = QStringLiteral("实体状态 PDU 长度不足，无法创建 90101 平台。");
		return false;
	}

	S_PDU_ENTITY_STATE source;
	if (!UtilDisPdu::deal_pdu_entity_state(source_data, source)) {
		error = QStringLiteral("实体状态 PDU 解析失败，无法创建 90101 平台。");
		return false;
	}

	std::array<double, 3> east{};
	std::array<double, 3> south{};
	if (!local_frame_from_ecef(source.entity_location_x, source.entity_location_y, source.entity_location_z, east, south)) {
		error = QStringLiteral("首个实体坐标无效，无法计算南方 100km 位置。");
		return false;
	}

	S_AFSimTestEntityIdentifier id;
	id.site = source.entity_id.entity_id_site;
	id.application = source.entity_id.entity_id_application;
	id.entity = DERIVED_AIRCRAFT_DIS_ENTITY_ID;
	S_AFSimTestEntityType type;
	type.kind = 1;
	type.domain = 2;
	type.country = source.entity_type.entity_type_country == 0 ? 45 : source.entity_type.entity_type_country;
	type.category = 1;

	packet = build_entity_state_packet(
		source.header.exercise_id == 0 ? 1 : source.header.exercise_id,
		source.force_id == 0 ? 1 : source.force_id,
		id,
		type,
		source.entity_location_x + south[0] * DERIVED_AIRCRAFT_SOUTH_OFFSET_M,
		source.entity_location_y + south[1] * DERIVED_AIRCRAFT_SOUTH_OFFSET_M,
		source.entity_location_z + south[2] * DERIVED_AIRCRAFT_SOUTH_OFFSET_M,
		static_cast<float>(east[0] * DERIVED_AIRCRAFT_EAST_SPEED_MPS),
		static_cast<float>(east[1] * DERIVED_AIRCRAFT_EAST_SPEED_MPS),
		static_cast<float>(east[2] * DERIVED_AIRCRAFT_EAST_SPEED_MPS),
		0.0f, 0.0f, 0.0f,
		QString::number(DERIVED_AIRCRAFT_PLATFORM_ID));
	return true;
}

constexpr int kPduLogTypes[] = {1, 2, 3, 23};
constexpr int kPduLogTypeCount = sizeof(kPduLogTypes) / sizeof(kPduLogTypes[0]);

int pdu_log_slot(int pdu_type, bool send_side)
{
	for (int i = 0; i < kPduLogTypeCount; ++i) {
		if (kPduLogTypes[i] == pdu_type) {
			return i * 2 + (send_side ? 1 : 0);
		}
	}
	return -1;
}

QString default_pdu_log_row(int pdu_type, bool send_side)
{
	return send_side
		? QStringLiteral("[--:--:--.---] [发送] [AFSim服务端] DIS PDU-%1 已发送。次数 0。")
			.arg(pdu_type, 2, 10, QChar('0'))
		: QStringLiteral("[--:--:--.---] [接收] [AFSim客户端] DIS PDU-%1 已接收。次数 0。")
			.arg(pdu_type, 2, 10, QChar('0'));
}

void reset_pdu_log_rows(QStringList& rows)
{
	rows.clear();
	for (const int pdu_type : kPduLogTypes) {
		rows.push_back(default_pdu_log_row(pdu_type, false));
		rows.push_back(default_pdu_log_row(pdu_type, true));
	}
}

}

S_AFSimTestLogStore::S_AFSimTestLogStore()
{
	clear();
}

bool S_AFSimTestLogStore::append(const QString& info)
{
	const QString key = merge_key(info);
	if (!key.isEmpty()) {
		bool ok = false;
		const int slot = key.toInt(&ok);
		if (!ok) {
			return false;
		}
		if (rows.size() != kPduLogTypeCount * 2) {
			reset_pdu_log_rows(rows);
		}
		if (slot >= 0 && slot < rows.size() && rows[slot] != info) {
			rows[slot] = info;
			return true;
		}
		return false;
	}
	return false;
}

QString S_AFSimTestLogStore::merge_key(const QString& info) const
{
	static const QRegularExpression bracket_reg(QStringLiteral("^\\[[^\\]]+\\] \\[([^\\]]+)\\] \\[([^\\]]+)\\] (.+)$"));
	const QRegularExpressionMatch bracket_match = bracket_reg.match(info);
	if (!bracket_match.hasMatch()) {
		return QString();
	}

	const QString stage = bracket_match.captured(1);
	const QString module = bracket_match.captured(2);
	const QString message = bracket_match.captured(3);
	static const QRegularExpression pdu_slot_reg(QStringLiteral("DIS PDU-([0-9]+)"));
	const QRegularExpressionMatch pdu_slot_match = pdu_slot_reg.match(message);
	if (pdu_slot_match.hasMatch()) {
		const int pdu_type = pdu_slot_match.captured(1).toInt();
		const bool recv = stage.compare(QStringLiteral("RECV"), Qt::CaseInsensitive) == 0 ||
			stage.contains(QStringLiteral("接收")) ||
			message.contains(QStringLiteral("已接收")) ||
			message.contains(QStringLiteral("recv"), Qt::CaseInsensitive) ||
			module.contains(QStringLiteral("客户端"));
		const bool send = stage.compare(QStringLiteral("SEND"), Qt::CaseInsensitive) == 0 ||
			stage.contains(QStringLiteral("发送")) ||
			message.contains(QStringLiteral("已发送")) ||
			message.contains(QStringLiteral("send"), Qt::CaseInsensitive) ||
			module.contains(QStringLiteral("服务端"));
		const int slot = (recv != send) ? pdu_log_slot(pdu_type, send) : -1;
		return slot >= 0 ? QString::number(slot) : QString();
	}
	return QString();
}

void S_AFSimTestLogStore::clear()
{
	reset_pdu_log_rows(rows);
}

void S_AFSimTestPduViews::clear()
{
	entity_rows.clear();
	fire_rows.clear();
	detonation_rows.clear();
	emission_rows.clear();
	latest_entity_state.clear();
	latest_fire.clear();
	latest_detonation.clear();
	latest_emission.clear();
	entity_summary = QStringLiteral("尚未收到实体状态 PDU。");
	fire_summary = QStringLiteral("尚未收到发射 PDU。");
	detonation_summary = QStringLiteral("尚未收到爆炸 PDU。");
	emission_summary = QStringLiteral("尚未收到电磁辐射 PDU。");
}

bool S_AFSimTestPduViews::update(const S_AFSimTestDecodeResult& decoded)
{
	if (!decoded.ok) return false;

	switch (decoded.pdu_type) {
	case 1:
		entity_rows = decoded.fields;
		latest_entity_state = decoded.structure;
		entity_summary = decoded.summary;
		return true;
	case 2:
		fire_rows = decoded.fields;
		latest_fire = decoded.structure;
		fire_summary = decoded.summary;
		return true;
	case 3:
		detonation_rows = decoded.fields;
		latest_detonation = decoded.structure;
		detonation_summary = decoded.summary;
		return true;
	case 23:
		emission_rows = decoded.fields;
		latest_emission = decoded.structure;
		emission_summary = decoded.summary;
		return true;
	default:
		return false;
	}
}

void S_AFSimTestPacketStore::clear()
{
	rows.clear();
	selected_rows.clear();
	selected_hex.clear();
	selected_summary = QStringLiteral("未选择数据包。");
	selected_structure_name = QStringLiteral("-");
	selected_index = -1;
	seq = 0;
	pdu_views.clear();
}

bool S_AFSimTestPacketStore::select(int index)
{
	if (index < 0 || index >= rows.size()) {
		return false;
	}
	selected_index = index;
	const QVariantMap packet = rows.at(index).toMap();
	selected_hex = packet.value("hex").toString();
	selected_summary = packet.value("summary").toString();
	selected_structure_name = packet.value("structureName").toString();
	selected_rows = packet.value("fields").toList();
	return true;
}

S_AFSimTestPacketUpdate S_AFSimTestPacketStore::append(const QByteArray& data,
	const QString& direction, const QString& endpoint, qint64 time_ms)
{
	S_AFSimTestPacketUpdate update;
	const S_AFSimTestDecodeResult decoded = decode_datagram(data, direction, endpoint, time_ms);
	QVariantMap packet = decoded.row;
	packet["seq"] = ++seq;

	const bool select_first_packet = selected_index < 0;
	if (selected_index >= 0) {
		++selected_index;
		update.selection_changed = true;
	}

	rows.push_front(packet);
	update.rows_changed = true;
	while (rows.size() > 400) {
		rows.removeLast();
	}

	if (selected_index >= rows.size()) {
		selected_index = -1;
		selected_hex.clear();
		selected_summary = QStringLiteral("未选择数据包。");
		selected_structure_name = QStringLiteral("-");
		selected_rows.clear();
		update.selection_changed = true;
	}
	if (select_first_packet) {
		selected_index = 0;
		selected_hex = packet.value("hex").toString();
		selected_summary = packet.value("summary").toString();
		selected_structure_name = packet.value("structureName").toString();
		selected_rows = decoded.fields;
		update.selection_changed = true;
	}

	update.pdu_views_changed = pdu_views.update(decoded);
	return update;
}

UiAFSimTestSendModel::UiAFSimTestSendModel()
{
	rebuild_fields();
}

bool UiAFSimTestSendModel::set_pdu_index(int index)
{
	if (index < 0 || index >= pdu_types.size() || pdu_index == index) return false;
	pdu_index = index;
	default_packet.clear();
	default_packet_active = false;
	manual_override = false;
	entity_exercise_id = 1;
	entity_force_id = 1;
	rebuild_fields();
	return true;
}

bool UiAFSimTestSendModel::set_hex(const QString& value)
{
	if (hex == value) return false;
	hex = value;
	default_packet.clear();
	default_packet_active = false;
	manual_override = true;
	status = QStringLiteral("十六进制已手动编辑，可解析或直接发送。");
	return true;
}

bool UiAFSimTestSendModel::set_field_value(int index, const QString& value)
{
	if (index < 0 || index >= field_rows.size()) return false;
	const QString key = field_rows.at(index).toMap().value("key").toString();
	if (key.isEmpty()) return false;
	return set_field_value_by_key(key, value);
}

bool UiAFSimTestSendModel::set_field_value_by_key(const QString& key, const QString& value)
{
	if (key.trimmed().isEmpty()) return false;

	for (int i = 0; i < field_rows.size(); ++i) {
		QVariantMap row = field_rows.at(i).toMap();
		if (row.value("key").toString() == key) {
			if (row.value("value").toString() == value) return false;
			row["value"] = value;
			field_rows[i] = row;
			default_packet.clear();
			default_packet_active = false;
			manual_override = true;
			status = QStringLiteral("字段已编辑，发送前请先构建。");
			return true;
		}

		QVariantList parts = row.value("parts").toList();
		for (int j = 0; j < parts.size(); ++j) {
			QVariantMap part = parts.at(j).toMap();
			if (part.value("key").toString() != key) continue;
			if (part.value("value").toString() == value) return false;
			part["value"] = value;
			parts[j] = part;
			row["parts"] = parts;
			field_rows[i] = row;
			sync_entity_velocity_fields(key);
			default_packet.clear();
			default_packet_active = false;
			manual_override = true;
			status = QStringLiteral("字段已编辑，发送前请先构建。");
			return true;
		}
	}

	return false;
}

int UiAFSimTestSendModel::current_pdu_type() const
{
	switch (pdu_index) {
	case 0: return 1;
	case 1: return 2;
	case 2: return 3;
	case 3: return 23;
	default: return -1;
	}
}

bool UiAFSimTestSendModel::clear_default_packet()
{
	if (!default_packet_active) {
		return false;
	}
	default_packet.clear();
	default_packet_active = false;
	if (!manual_override) {
		entity_exercise_id = 1;
		entity_force_id = 1;
		rebuild_fields();
		status = QStringLiteral("已清除首包默认值，恢复内置模板。");
	}
	return true;
}

bool UiAFSimTestSendModel::apply_default_packet(const QByteArray& data)
{
	if (data.size() < DIS_PDU_HEADER_LEN) {
		return false;
	}
	const int pdu_type = UtilDisPdu::read_u8(data, 2);
	if (pdu_type != current_pdu_type() || !can_apply_received_default()) {
		return false;
	}
	if ((pdu_type == 1 && data.size() < DIS_PDU_ENTITY_STATE_LEN) ||
		(pdu_type == 2 && data.size() < DIS_PDU_FIRE_LEN) ||
		(pdu_type == 3 && data.size() < DIS_PDU_DETONATION_LEN) ||
		(pdu_type == 23 && data.size() < DIS_PDU_ELC_EMISSION_FIXED_LEN)) {
		return false;
	}

	rebuild_fields();
	S_PDU_HEADER header;
	if (!parse_header(data, header)) {
		return false;
	}
	set_field_value_quiet(QStringLiteral("exercise_id"), QString::number(header.exercise_id));

	switch (pdu_type) {
	case 1: {
		S_PDU_ENTITY_STATE pdu;
		if (!UtilDisPdu::deal_pdu_entity_state(data, pdu)) return false;
		entity_exercise_id = header.exercise_id == 0 ? 1 : header.exercise_id;
		entity_force_id = pdu.force_id == 0 ? 1 : pdu.force_id;
		set_field_value_quiet(QStringLiteral("entity_site"), QString::number(pdu.entity_id.entity_id_site));
		set_field_value_quiet(QStringLiteral("entity_application"), QString::number(pdu.entity_id.entity_id_application));
		set_field_value_quiet(QStringLiteral("entity_entity"), QString::number(pdu.entity_id.entity_id_entity));
		apply_type_defaults(QStringLiteral("entity"), entity_type_from_pdu(pdu.entity_type));
		S_TEST_LLA lla;
		if (ecef_to_lla(pdu.entity_location_x, pdu.entity_location_y, pdu.entity_location_z, lla)) {
			set_field_value_quiet(QStringLiteral("location_lat"), number_text(lla.lat_deg, 'f', 6));
			set_field_value_quiet(QStringLiteral("location_lon"), number_text(lla.lon_deg, 'f', 6));
			set_field_value_quiet(QStringLiteral("location_alt"), number_text(lla.alt_m, 'f', 3));

			S_TEST_ENU_VEL velocity;
			if (ecef_velocity_to_enu(pdu.entity_linear_velocity_x, pdu.entity_linear_velocity_y,
				pdu.entity_linear_velocity_z, lla, velocity)) {
				set_field_value_quiet(QStringLiteral("velocity_east"), number_text(velocity.east_mps, 'f', 3));
				set_field_value_quiet(QStringLiteral("velocity_north"), number_text(velocity.north_mps, 'f', 3));
				set_field_value_quiet(QStringLiteral("velocity_up"), number_text(velocity.up_mps, 'f', 3));
				set_field_value_quiet(QStringLiteral("velocity_speed"), number_text(velocity.speed_mps, 'f', 3));
				set_field_value_quiet(QStringLiteral("velocity_track"), number_text(velocity.track_deg, 'f', 3));
				set_field_value_quiet(QStringLiteral("velocity_pitch"), number_text(velocity.pitch_deg, 'f', 3));
			}
		}
		set_field_value_quiet(QStringLiteral("psi"), QString::number(pdu.entity_orientation_psi, 'g', 9));
		set_field_value_quiet(QStringLiteral("theta"), QString::number(pdu.entity_orientation_theta, 'g', 9));
		set_field_value_quiet(QStringLiteral("phi"), QString::number(pdu.entity_orientation_phi, 'g', 9));
		set_field_value_quiet(QStringLiteral("marking"), marking_text(pdu));
		break;
	}
	case 2: {
		S_PDU_FIRE pdu;
		if (!parse_fire(data, pdu)) return false;
		apply_id_defaults(QStringLiteral("firing"), S_AFSimTestEntityIdentifier{
			pdu.firing_entity_id.entity_identifier,
			pdu.firing_entity_id.entity_id_site,
			pdu.firing_entity_id.entity_id_application,
			pdu.firing_entity_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("target"), S_AFSimTestEntityIdentifier{
			pdu.target_entity_id.entity_identifier,
			pdu.target_entity_id.entity_id_site,
			pdu.target_entity_id.entity_id_application,
			pdu.target_entity_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("munition"), S_AFSimTestEntityIdentifier{
			pdu.munition_id.entity_identifier,
			pdu.munition_id.entity_id_site,
			pdu.munition_id.entity_id_application,
			pdu.munition_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("event"), S_AFSimTestEntityIdentifier{
			pdu.event_id.event_identifier,
			pdu.event_id.event_id_site,
			pdu.event_id.event_id_application,
			pdu.event_id.event_id_event
		});
		set_field_value_quiet(QStringLiteral("fire_mission_index"), QString::number(pdu.fire_mission_index));
		set_field_value_quiet(QStringLiteral("location_x"), QString::number(pdu.location_in_world_coordinate_x, 'g', 17));
		set_field_value_quiet(QStringLiteral("location_y"), QString::number(pdu.location_in_world_coordinate_y, 'g', 17));
		set_field_value_quiet(QStringLiteral("location_z"), QString::number(pdu.location_in_world_coordinate_z, 'g', 17));
		S_AFSimTestBurstDescriptor burst;
		burst.entity_type.text = pdu.burst_descriptor.entity_type_complete;
		burst.entity_type.kind = pdu.burst_descriptor.entity_type_kind;
		burst.entity_type.domain = pdu.burst_descriptor.entity_type_domain;
		burst.entity_type.country = pdu.burst_descriptor.entity_type_country;
		burst.entity_type.category = pdu.burst_descriptor.entity_type_category;
		burst.entity_type.subcategory = pdu.burst_descriptor.entity_type_subcategory;
		burst.entity_type.specific = pdu.burst_descriptor.entity_type_specific;
		burst.entity_type.extra = pdu.burst_descriptor.entity_type_extra;
		burst.warhead = pdu.burst_descriptor.warhead;
		burst.fuse = pdu.burst_descriptor.fuse;
		burst.quantity = pdu.burst_descriptor.quantity;
		burst.rate = pdu.burst_descriptor.rate;
		apply_burst_defaults(burst);
		set_field_value_quiet(QStringLiteral("velocity_x"), QString::number(pdu.velocity_x, 'g', 9));
		set_field_value_quiet(QStringLiteral("velocity_y"), QString::number(pdu.velocity_y, 'g', 9));
		set_field_value_quiet(QStringLiteral("velocity_z"), QString::number(pdu.velocity_z, 'g', 9));
		set_field_value_quiet(QStringLiteral("range"), QString::number(pdu.range, 'g', 9));
		break;
	}
	case 3: {
		S_PDU_DETONATION pdu;
		if (!parse_detonation(data, pdu)) return false;
		apply_id_defaults(QStringLiteral("firing"), S_AFSimTestEntityIdentifier{
			pdu.firing_entity_id.entity_identifier,
			pdu.firing_entity_id.entity_id_site,
			pdu.firing_entity_id.entity_id_application,
			pdu.firing_entity_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("target"), S_AFSimTestEntityIdentifier{
			pdu.target_entity_id.entity_identifier,
			pdu.target_entity_id.entity_id_site,
			pdu.target_entity_id.entity_id_application,
			pdu.target_entity_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("munition"), S_AFSimTestEntityIdentifier{
			pdu.munition_id.entity_identifier,
			pdu.munition_id.entity_id_site,
			pdu.munition_id.entity_id_application,
			pdu.munition_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("event"), S_AFSimTestEntityIdentifier{
			pdu.event_id.event_identifier,
			pdu.event_id.event_id_site,
			pdu.event_id.event_id_application,
			pdu.event_id.event_id_event
		});
		set_field_value_quiet(QStringLiteral("velocity_x"), QString::number(pdu.velocity_x, 'g', 9));
		set_field_value_quiet(QStringLiteral("velocity_y"), QString::number(pdu.velocity_y, 'g', 9));
		set_field_value_quiet(QStringLiteral("velocity_z"), QString::number(pdu.velocity_z, 'g', 9));
		set_field_value_quiet(QStringLiteral("location_x"), QString::number(pdu.location_in_world_coordinate_x, 'g', 17));
		set_field_value_quiet(QStringLiteral("location_y"), QString::number(pdu.location_in_world_coordinate_y, 'g', 17));
		set_field_value_quiet(QStringLiteral("location_z"), QString::number(pdu.location_in_world_coordinate_z, 'g', 17));
		S_AFSimTestBurstDescriptor burst;
		burst.entity_type.text = pdu.burst_descriptor.entity_type_complete;
		burst.entity_type.kind = pdu.burst_descriptor.entity_type_kind;
		burst.entity_type.domain = pdu.burst_descriptor.entity_type_domain;
		burst.entity_type.country = pdu.burst_descriptor.entity_type_country;
		burst.entity_type.category = pdu.burst_descriptor.entity_type_category;
		burst.entity_type.subcategory = pdu.burst_descriptor.entity_type_subcategory;
		burst.entity_type.specific = pdu.burst_descriptor.entity_type_specific;
		burst.entity_type.extra = pdu.burst_descriptor.entity_type_extra;
		burst.warhead = pdu.burst_descriptor.warhead;
		burst.fuse = pdu.burst_descriptor.fuse;
		burst.quantity = pdu.burst_descriptor.quantity;
		burst.rate = pdu.burst_descriptor.rate;
		apply_burst_defaults(burst);
		set_field_value_quiet(QStringLiteral("entity_location_x"), QString::number(pdu.location_in_entity_coordinate_x, 'g', 9));
		set_field_value_quiet(QStringLiteral("entity_location_y"), QString::number(pdu.location_in_entity_coordinate_y, 'g', 9));
		set_field_value_quiet(QStringLiteral("entity_location_z"), QString::number(pdu.location_in_entity_coordinate_z, 'g', 9));
		set_field_value_quiet(QStringLiteral("detonation_result"), QString::number(pdu.detonation_result));
		break;
	}
	case 23: {
		S_PDU_ELC_EMISSION pdu;
		if (!parse_emission(data, pdu)) return false;
		apply_id_defaults(QStringLiteral("emitting"), S_AFSimTestEntityIdentifier{
			pdu.emitting_entity_id.entity_identifier,
			pdu.emitting_entity_id.entity_id_site,
			pdu.emitting_entity_id.entity_id_application,
			pdu.emitting_entity_id.entity_id_entity
		});
		apply_id_defaults(QStringLiteral("event"), S_AFSimTestEntityIdentifier{
			pdu.event_id.event_identifier,
			pdu.event_id.event_id_site,
			pdu.event_id.event_id_application,
			pdu.event_id.event_id_event
		});
		set_field_value_quiet(QStringLiteral("state_update_indicator"), QString::number(pdu.state_update_indicator));
		set_field_value_quiet(QStringLiteral("number_of_systems"), QString::number(pdu.number_of_systems));
		set_field_value_quiet(QStringLiteral("emitter_records_hex"), bytes_to_hex(pdu.emitter_system_records));
		break;
	}
	default:
		return false;
	}

	default_packet = data;
	default_packet_active = true;
	manual_override = false;
	hex = bytes_to_hex(data);
	status = QString("默认使用首个收到的 PDU-%1，%2 字节。")
		.arg(pdu_type, 2, 10, QChar('0')).arg(data.size());
	return true;
}

void UiAFSimTestSendModel::rebuild_fields()
{
	field_rows.clear();

	const auto add = [this](const QString& key, const QString& label, const QString& value, const QString& note = QString()) {
		field_rows.push_back(send_field(key, label, value, note));
	};
	const auto add_group = [this](const QString& label, const QVariantList& parts, const QString& note = QString(),
		int row_height = 38, int columns = 0) {
		field_rows.push_back(send_group(label, parts, note, row_height, columns));
	};
	const auto part = [](const QString& key, const QString& label, const QString& value, const QString& note = QString()) {
		return send_field(key, label, value, note);
	};
	const auto add_id = [&add_group, &part](const QString& prefix, const QString& label,
		const QString& site, const QString& application, const QString& entity) {
		add_group(label, QVariantList{
			part(prefix + "_site", "站点", site),
			part(prefix + "_application", "应用", application),
			part(prefix + "_entity", "实体", entity)
		});
	};
	const auto add_type = [&add_group, &part](const QString& prefix, const QString& label, int row_height = 38, int columns = 0) {
		add_group(label, QVariantList{
			part(prefix + "_kind", "种类", "1"),
			part(prefix + "_domain", "领域", "2"),
			part(prefix + "_country", "国家", "45"),
			part(prefix + "_category", "类别", "1"),
			part(prefix + "_subcategory", "子类", "1"),
			part(prefix + "_specific", "特定", "0"),
			part(prefix + "_extra", "扩展", "0")
		}, QString(), row_height, columns);
	};
	const auto add_xyz = [&add_group, &part](const QString& label,
		const QString& key_x, const QString& value_x,
		const QString& key_y, const QString& value_y,
		const QString& key_z, const QString& value_z,
		const QString& note = QString()) {
		add_group(label, QVariantList{
			part(key_x, "X", value_x, note),
			part(key_y, "Y", value_y, note),
			part(key_z, "Z", value_z, note)
		}, note);
	};

	if (pdu_index == 0) {
		add_id("entity", "实体ID", "1", "100", "9001");
		add_type("entity", "实体类型", 70, 4);
		add_group("经纬高（LLA）", QVariantList{
			part("location_lat", "纬度deg", "39.800000"),
			part("location_lon", "经度deg", "116.400000"),
			part("location_alt", "高度m", "1000.000")
		});
		add_group("航迹速度", QVariantList{
			part("velocity_east", "E", "120.000"),
			part("velocity_north", "N", "0.000"),
			part("velocity_up", "U", "0.000"),
			part("velocity_speed", "速度", "120.000"),
			part("velocity_track", "航迹角", "90.000"),
			part("velocity_pitch", "俯仰角", "0.000")
		}, "ENU m/s，角度为 deg", 70, 3);
		add_group("姿态", QVariantList{
			part("psi", "Psi", "0.0", "rad"),
			part("theta", "Theta", "0.0", "rad"),
			part("phi", "Phi", "0.0", "rad")
		}, "rad");
		add("marking", "实体标识", "VERIFY", "最大 11 字符");
	}
	else if (pdu_index == 1) {
		add("exercise_id", "演练ID", "1");
		add_id("firing", "发射实体ID", "1", "100", "9001");
		add_id("target", "目标实体ID", "1", "100", "9002");
		add_id("munition", "弹药ID", "1", "100", "9101");
		add_id("event", "事件ID", "1", "100", "1");
		add("fire_mission_index", "火力任务索引", "1");
		add_xyz("发射位置", "location_x", "-2178232.125", "location_y", "4387211.625", "location_z", "4078123.250", "ECEF m");
		add_type("entity", "弹药实体类型");
		add_group("弹药参数", QVariantList{
			part("warhead", "战斗部", "0"),
			part("fuse", "引信", "0"),
			part("quantity", "数量", "1"),
			part("rate", "射速", "0")
		});
		add_xyz("速度", "velocity_x", "300.0", "velocity_y", "0.0", "velocity_z", "0.0", "m/s");
		add("range", "射程", "50000.0", "m");
	}
	else if (pdu_index == 2) {
		add("exercise_id", "演练ID", "1");
		add_id("firing", "发射实体ID", "1", "100", "9001");
		add_id("target", "目标实体ID", "1", "100", "9002");
		add_id("munition", "弹药ID", "1", "100", "9101");
		add_id("event", "事件ID", "1", "100", "1");
		add_xyz("速度", "velocity_x", "300.0", "velocity_y", "0.0", "velocity_z", "0.0", "m/s");
		add_xyz("爆炸位置", "location_x", "-2178232.125", "location_y", "4387211.625", "location_z", "4078123.250", "ECEF m");
		add_type("entity", "弹药实体类型");
		add_group("弹药参数", QVariantList{
			part("warhead", "战斗部", "0"),
			part("fuse", "引信", "0"),
			part("quantity", "数量", "1"),
			part("rate", "射速", "0")
		});
		add_xyz("实体坐标", "entity_location_x", "0.0", "entity_location_y", "0.0", "entity_location_z", "0.0", "m");
		add("detonation_result", "爆炸结果", "1");
	}
	else {
		add("exercise_id", "演练ID", "1");
		add_id("emitting", "辐射实体ID", "1", "100", "9001");
		add_id("event", "事件ID", "1", "100", "1");
		add("state_update_indicator", "状态更新指示", "0");
		add("number_of_systems", "辐射系统数量", "0");
		add("emitter_records_hex", "辐射系统记录十六进制", "", "可选变长记录");
	}
}

QByteArray UiAFSimTestSendModel::build_packet(QString* error) const
{
	if (error != nullptr) error->clear();
	if (using_default_packet()) {
		return default_packet;
	}
	const quint8 exercise = field_u8(QStringLiteral("exercise_id"), 1);

	if (pdu_index == 0) {
		S_TEST_LLA lla;
		lla.lat_deg = field_f64(QStringLiteral("location_lat"), 39.8);
		lla.lon_deg = field_f64(QStringLiteral("location_lon"), 116.4);
		lla.alt_m = field_f64(QStringLiteral("location_alt"), 1000.0);
		double location_x = 0.0;
		double location_y = 0.0;
		double location_z = 0.0;
		lla_to_ecef(lla, location_x, location_y, location_z);
		double velocity_x = 0.0;
		double velocity_y = 0.0;
		double velocity_z = 0.0;
		enu_velocity_to_ecef(lla,
			field_f64(QStringLiteral("velocity_east"), 120.0),
			field_f64(QStringLiteral("velocity_north")),
			field_f64(QStringLiteral("velocity_up")),
			velocity_x, velocity_y, velocity_z);
		return build_entity_state_packet(
			entity_exercise_id == 0 ? 1 : entity_exercise_id,
			entity_force_id == 0 ? 1 : entity_force_id,
			field_id(QStringLiteral("entity")),
			field_type(QStringLiteral("entity")),
			location_x, location_y, location_z,
			static_cast<float>(velocity_x),
			static_cast<float>(velocity_y),
			static_cast<float>(velocity_z),
			field_f32(QStringLiteral("psi")),
			field_f32(QStringLiteral("theta")),
			field_f32(QStringLiteral("phi")),
			field_value(QStringLiteral("marking")).left(11));
	}
	if (pdu_index == 1) {
		return build_fire_packet(
			exercise,
			field_id(QStringLiteral("firing")),
			field_id(QStringLiteral("target")),
			field_id(QStringLiteral("munition")),
			field_id(QStringLiteral("event")),
			field_u32(QStringLiteral("fire_mission_index"), 1),
			field_f64(QStringLiteral("location_x")),
			field_f64(QStringLiteral("location_y")),
			field_f64(QStringLiteral("location_z")),
			field_burst(QStringLiteral("entity")),
			field_f32(QStringLiteral("velocity_x")),
			field_f32(QStringLiteral("velocity_y")),
			field_f32(QStringLiteral("velocity_z")),
			field_f32(QStringLiteral("range")));
	}
	if (pdu_index == 2) {
		return build_detonation_packet(
			exercise,
			field_id(QStringLiteral("firing")),
			field_id(QStringLiteral("target")),
			field_id(QStringLiteral("munition")),
			field_id(QStringLiteral("event")),
			field_f32(QStringLiteral("velocity_x")),
			field_f32(QStringLiteral("velocity_y")),
			field_f32(QStringLiteral("velocity_z")),
			field_f64(QStringLiteral("location_x")),
			field_f64(QStringLiteral("location_y")),
			field_f64(QStringLiteral("location_z")),
			field_burst(QStringLiteral("entity")),
			field_f32(QStringLiteral("entity_location_x")),
			field_f32(QStringLiteral("entity_location_y")),
			field_f32(QStringLiteral("entity_location_z")),
			field_u8(QStringLiteral("detonation_result"), 1));
	}

	QString hex_error;
	const QByteArray records = hex_to_bytes(field_value(QStringLiteral("emitter_records_hex")), &hex_error);
	if (!hex_error.isEmpty() && !field_value(QStringLiteral("emitter_records_hex")).trimmed().isEmpty()) {
		if (error != nullptr) *error = hex_error;
		return {};
	}
	return build_emission_packet(
		exercise,
		field_id(QStringLiteral("emitting")),
		field_id(QStringLiteral("event")),
		field_u8(QStringLiteral("state_update_indicator")),
		field_u8(QStringLiteral("number_of_systems")),
		records);
}

void UiAFSimTestSendModel::set_field_value_quiet(const QString& key, const QString& value)
{
	for (int i = 0; i < field_rows.size(); ++i) {
		QVariantMap row = field_rows.at(i).toMap();
		if (row.value("key").toString() == key) {
			row["value"] = value;
			field_rows[i] = row;
			return;
		}

		QVariantList parts = row.value("parts").toList();
		for (int j = 0; j < parts.size(); ++j) {
			QVariantMap part = parts.at(j).toMap();
			if (part.value("key").toString() != key) continue;
			part["value"] = value;
			parts[j] = part;
			row["parts"] = parts;
			field_rows[i] = row;
			return;
		}
	}
}

void UiAFSimTestSendModel::sync_entity_velocity_fields(const QString& changed_key)
{
	if (pdu_index != 0 || !changed_key.startsWith(QStringLiteral("velocity_"))) {
		return;
	}

	if (changed_key == QStringLiteral("velocity_east") ||
		changed_key == QStringLiteral("velocity_north") ||
		changed_key == QStringLiteral("velocity_up")) {
		S_TEST_LLA lla;
		lla.lat_deg = field_f64(QStringLiteral("location_lat"), 39.8);
		lla.lon_deg = field_f64(QStringLiteral("location_lon"), 116.4);
		lla.alt_m = field_f64(QStringLiteral("location_alt"), 1000.0);

		S_TEST_ENU_VEL velocity;
		velocity.east_mps = field_f64(QStringLiteral("velocity_east"), 120.0);
		velocity.north_mps = field_f64(QStringLiteral("velocity_north"));
		velocity.up_mps = field_f64(QStringLiteral("velocity_up"));
		double vx = 0.0;
		double vy = 0.0;
		double vz = 0.0;
		enu_velocity_to_ecef(lla, velocity.east_mps, velocity.north_mps, velocity.up_mps, vx, vy, vz);
		ecef_velocity_to_enu(vx, vy, vz, lla, velocity);
		set_field_value_quiet(QStringLiteral("velocity_speed"), number_text(velocity.speed_mps, 'f', 3));
		set_field_value_quiet(QStringLiteral("velocity_track"), number_text(velocity.track_deg, 'f', 3));
		set_field_value_quiet(QStringLiteral("velocity_pitch"), number_text(velocity.pitch_deg, 'f', 3));
		return;
	}

	if (changed_key == QStringLiteral("velocity_speed") ||
		changed_key == QStringLiteral("velocity_track") ||
		changed_key == QStringLiteral("velocity_pitch")) {
		const S_TEST_ENU_VEL velocity = track_to_enu(
			field_f64(QStringLiteral("velocity_speed"), 120.0),
			field_f64(QStringLiteral("velocity_track"), 90.0),
			field_f64(QStringLiteral("velocity_pitch")));
		set_field_value_quiet(QStringLiteral("velocity_east"), number_text(velocity.east_mps, 'f', 3));
		set_field_value_quiet(QStringLiteral("velocity_north"), number_text(velocity.north_mps, 'f', 3));
		set_field_value_quiet(QStringLiteral("velocity_up"), number_text(velocity.up_mps, 'f', 3));
	}
}

void UiAFSimTestSendModel::apply_id_defaults(const QString& prefix, const S_AFSimTestEntityIdentifier& id)
{
	set_field_value_quiet(prefix + QStringLiteral("_site"), QString::number(id.site));
	set_field_value_quiet(prefix + QStringLiteral("_application"), QString::number(id.application));
	set_field_value_quiet(prefix + QStringLiteral("_entity"), QString::number(id.entity));
}

void UiAFSimTestSendModel::apply_type_defaults(const QString& prefix, const S_AFSimTestEntityType& type)
{
	set_field_value_quiet(prefix + QStringLiteral("_kind"), QString::number(type.kind));
	set_field_value_quiet(prefix + QStringLiteral("_domain"), QString::number(type.domain));
	set_field_value_quiet(prefix + QStringLiteral("_country"), QString::number(type.country));
	set_field_value_quiet(prefix + QStringLiteral("_category"), QString::number(type.category));
	set_field_value_quiet(prefix + QStringLiteral("_subcategory"), QString::number(type.subcategory));
	set_field_value_quiet(prefix + QStringLiteral("_specific"), QString::number(type.specific));
	set_field_value_quiet(prefix + QStringLiteral("_extra"), QString::number(type.extra));
}

void UiAFSimTestSendModel::apply_burst_defaults(const S_AFSimTestBurstDescriptor& burst)
{
	apply_type_defaults(QStringLiteral("entity"), burst.entity_type);
	set_field_value_quiet(QStringLiteral("warhead"), QString::number(burst.warhead));
	set_field_value_quiet(QStringLiteral("fuse"), QString::number(burst.fuse));
	set_field_value_quiet(QStringLiteral("quantity"), QString::number(burst.quantity));
	set_field_value_quiet(QStringLiteral("rate"), QString::number(burst.rate));
}

QString UiAFSimTestSendModel::field_value(const QString& key) const
{
	for (const QVariant& item : field_rows) {
		const QVariantMap row = item.toMap();
		if (row.value("key").toString() == key) {
			return row.value("value").toString().trimmed();
		}

		const QVariantList parts = row.value("parts").toList();
		for (const QVariant& part_item : parts) {
			const QVariantMap part = part_item.toMap();
			if (part.value("key").toString() == key) {
				return part.value("value").toString().trimmed();
			}
		}
	}
	return {};
}

quint8 UiAFSimTestSendModel::field_u8(const QString& key, quint8 fallback) const
{
	bool ok = false;
	const uint value = field_value(key).toUInt(&ok);
	return ok ? static_cast<quint8>(std::min(value, 255u)) : fallback;
}

quint16 UiAFSimTestSendModel::field_u16(const QString& key, quint16 fallback) const
{
	bool ok = false;
	const uint value = field_value(key).toUInt(&ok);
	return ok ? static_cast<quint16>(std::min(value, 65535u)) : fallback;
}

quint32 UiAFSimTestSendModel::field_u32(const QString& key, quint32 fallback) const
{
	bool ok = false;
	const quint32 value = field_value(key).toUInt(&ok);
	return ok ? value : fallback;
}

float UiAFSimTestSendModel::field_f32(const QString& key, float fallback) const
{
	bool ok = false;
	const float value = field_value(key).toFloat(&ok);
	return ok ? value : fallback;
}

double UiAFSimTestSendModel::field_f64(const QString& key, double fallback) const
{
	bool ok = false;
	const double value = field_value(key).toDouble(&ok);
	return ok ? value : fallback;
}

S_AFSimTestEntityIdentifier UiAFSimTestSendModel::field_id(const QString& prefix) const
{
	S_AFSimTestEntityIdentifier id;
	id.site = field_u16(prefix + QStringLiteral("_site"), 1);
	id.application = field_u16(prefix + QStringLiteral("_application"), 1);
	id.entity = field_u16(prefix + QStringLiteral("_entity"), 1);
	id.text = QString("%1:%2:%3").arg(id.site).arg(id.application).arg(id.entity);
	return id;
}

S_AFSimTestEntityType UiAFSimTestSendModel::field_type(const QString& prefix) const
{
	S_AFSimTestEntityType type;
	type.kind = field_u8(prefix + QStringLiteral("_kind"), 1);
	type.domain = field_u8(prefix + QStringLiteral("_domain"), 2);
	type.country = field_u16(prefix + QStringLiteral("_country"), 45);
	type.category = field_u8(prefix + QStringLiteral("_category"), 1);
	type.subcategory = field_u8(prefix + QStringLiteral("_subcategory"), 1);
	type.specific = field_u8(prefix + QStringLiteral("_specific"));
	type.extra = field_u8(prefix + QStringLiteral("_extra"));
	type.text = QString("%1:%2:%3:%4:%5:%6:%7")
		.arg(type.kind).arg(type.domain).arg(type.country).arg(type.category)
		.arg(type.subcategory).arg(type.specific).arg(type.extra);
	return type;
}

S_AFSimTestBurstDescriptor UiAFSimTestSendModel::field_burst(const QString& prefix) const
{
	S_AFSimTestBurstDescriptor burst;
	burst.entity_type = field_type(prefix);
	burst.warhead = field_u16(QStringLiteral("warhead"));
	burst.fuse = field_u16(QStringLiteral("fuse"));
	burst.quantity = field_u16(QStringLiteral("quantity"), 1);
	burst.rate = field_u16(QStringLiteral("rate"));
	return burst;
}

UiAFSimTest::UiAFSimTest(QObject* parent)
	: QObject(parent)
{
	connect(&client, &AfsimClient::sig_afs_client_msg, this, &UiAFSimTest::on_client_log);
	connect(&client, &AfsimClient::sig_datagram_received, this, &UiAFSimTest::on_datagram_received);
	connect(&server, &AfsimServer::sig_afs_server_msg, this, &UiAFSimTest::on_server_log);
	connect(&server, &AfsimServer::sig_datagram_sent, this, &UiAFSimTest::on_datagram_sent);

	load_icgas_comm_config();
	buildSendPacket();
	append_log(QString("[%1] [状态] [AFSim验证] 工具已初始化。").arg(cur_time_str()));
}

UiAFSimTest::~UiAFSimTest()
{
	client.stop();
	server.stop();
}

void UiAFSimTest::show()
{
	if (engine.rootObjects().isEmpty()) {
		engine.rootContext()->setContextProperty("uiAFSimTest", this);
		const QUrl qml_url(QStringLiteral("qrc:/UiICGAS/UiAFSimTest.qml"));
		engine.load(qml_url);
		if (engine.rootObjects().isEmpty()) {
			qDebug() << "Error: cannot load UiAFSimTest QML" << qml_url;
			return;
		}

		if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
			window->setIcon(window_icon);
			center_on_primary_screen(window);
			window->show();
			QTimer::singleShot(0, window, [this, window]() {
				center_on_primary_screen(window);
			});
		}
		else if (QObject* root = root_object()) {
			root->setProperty("visible", true);
		}
		return;
	}

	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		center_on_primary_screen(window);
		window->show();
		QTimer::singleShot(0, window, [this, window]() {
			center_on_primary_screen(window);
		});
		window->raise();
	}
	else if (QObject* root = root_object()) {
		root->setProperty("visible", true);
	}
}

void UiAFSimTest::setWindowIcon(const QIcon& icon)
{
	window_icon = icon;
	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->setIcon(window_icon);
	}
}

void UiAFSimTest::setBindAddr(const QString& value)
{
	const QString next = value.trimmed();
	if (bind_addr == next) return;
	bind_addr = next;
	emit configChanged();
}

void UiAFSimTest::setBindPort(const QString& value)
{
	const QString next = value.trimmed();
	if (bind_port == next) return;
	bind_port = next;
	emit configChanged();
}

void UiAFSimTest::setTargetAddr(const QString& value)
{
	const QString next = value.trimmed();
	if (target_addr == next) return;
	target_addr = next;
	emit configChanged();
}

void UiAFSimTest::setTargetPort(const QString& value)
{
	const QString next = value.trimmed();
	if (target_port == next) return;
	target_port = next;
	emit configChanged();
}

void UiAFSimTest::setSendPduIndex(int index)
{
	if (!send_model.set_pdu_index(index)) return;
	if (apply_first_received_default()) {
		emit sendModelChanged();
		return;
	}
	buildSendPacket();
}

void UiAFSimTest::setSendHex(const QString& value)
{
	if (!send_model.set_hex(value)) return;
	emit sendModelChanged();
}

void UiAFSimTest::toggleClient()
{
	if (client.running()) {
		client.stop();
		emit stateChanged();
		return;
	}

	quint16 port = 0;
	if (!parse_port(bind_port, port, QStringLiteral("afs_client_port"))) {
		return;
	}
	client.start(bind_addr, port);
	emit stateChanged();
}

void UiAFSimTest::toggleServer()
{
	if (server.running()) {
		server.stop();
		emit stateChanged();
		return;
	}

	start_server(true);
}

void UiAFSimTest::selectPacket(int index)
{
	if (!packet_store.select(index)) return;
	emit selectedPacketChanged();
}

void UiAFSimTest::setSendFieldValue(int index, const QString& value)
{
	if (!send_model.set_field_value(index, value)) return;
	emit sendModelChanged();
}

void UiAFSimTest::setSendFieldValueByKey(const QString& key, const QString& value)
{
	if (!send_model.set_field_value_by_key(key, value)) return;
	emit sendModelChanged();
}

void UiAFSimTest::buildSendPacket()
{
	QString error;
	const QByteArray data = send_model.build_packet(&error);
	if (!error.isEmpty()) {
		send_model.status = error;
		append_log(QString("[%1] [错误] [数据包] %2").arg(cur_time_str(), error));
		emit sendModelChanged();
		return;
	}

	send_model.hex = bytes_to_hex(data);
	const S_AFSimTestDecodeResult decoded = decode_datagram(data,
		QStringLiteral("BUILD"), QStringLiteral("local"), QDateTime::currentMSecsSinceEpoch());
	send_model.status = send_model.using_default_packet()
		? QString("已使用首个接收包，%1，%2 字节。").arg(decoded.structure_name).arg(data.size())
		: QString("已构建 %1，%2 字节。").arg(decoded.structure_name).arg(data.size());
	emit sendModelChanged();
}

void UiAFSimTest::sendStructuredPacket()
{
	QString error;
	const QByteArray data = send_model.build_packet(&error);
	if (!error.isEmpty()) {
		send_model.status = error;
		append_log(QString("[%1] [错误] [数据包] %2").arg(cur_time_str(), error));
		emit sendModelChanged();
		return;
	}

	send_model.hex = bytes_to_hex(data);
	if (!server.running()) {
		send_model.status = QStringLiteral("发送端未启动，未发送当前结构化数据包。");
		append_log(QString("[%1] [错误] [数据包] 发送端未启动，手动发送不会自动启动发送端。").arg(cur_time_str()));
		emit sendModelChanged();
		emit stateChanged();
		return;
	}
	server.send_datagram(data);
	send_model.status = QStringLiteral("已通过 AFSim 服务端发送当前结构化数据包。");
	emit sendModelChanged();
	emit stateChanged();
}

void UiAFSimTest::decodeSendHex()
{
	QString error;
	const QByteArray data = hex_to_bytes(send_model.hex, &error);
	if (!error.isEmpty()) {
		send_model.status = error;
		append_log(QString("[%1] [错误] [数据包] %2").arg(cur_time_str(), error));
		emit sendModelChanged();
		return;
	}

	const S_AFSimTestDecodeResult decoded = decode_datagram(data,
		QStringLiteral("DECODE"), QStringLiteral("manual"), QDateTime::currentMSecsSinceEpoch());
	send_model.status = decoded.summary;
	emit sendModelChanged();
}

void UiAFSimTest::sendHexPacket()
{
	QString error;
	const QByteArray data = hex_to_bytes(send_model.hex, &error);
	if (!error.isEmpty()) {
		send_model.status = error;
		append_log(QString("[%1] [错误] [数据包] %2").arg(cur_time_str(), error));
		emit sendModelChanged();
		return;
	}

	if (!server.running()) {
		send_model.status = QStringLiteral("发送端未启动，未发送当前十六进制数据包。");
		append_log(QString("[%1] [错误] [数据包] 发送端未启动，手动发送不会自动启动发送端。").arg(cur_time_str()));
		emit sendModelChanged();
		emit stateChanged();
		return;
	}
	server.send_datagram(data);
	send_model.status = QStringLiteral("已通过 AFSim 服务端发送十六进制数据包。");
	emit sendModelChanged();
	emit stateChanged();
}

void UiAFSimTest::clearLogs()
{
	log_store.clear();
	emit logRowsChanged();
}

void UiAFSimTest::clearPackets()
{
	packet_store.clear();
	first_received_packets.clear();
	startup_entity_state_packet.clear();
	emit packetRowsChanged();
	emit selectedPacketChanged();
	emit pduViewsChanged();
	if (send_model.clear_default_packet()) {
		buildSendPacket();
	}
}

void UiAFSimTest::release()
{
	const bool was_client_running = client.running();
	const bool was_server_running = server.running();
	client.stop();
	server.stop();
	if (was_client_running || was_server_running) {
		emit stateChanged();
	}
}

void UiAFSimTest::closeWindow()
{
	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->close();
	}
}

void UiAFSimTest::on_client_log(const QString& info)
{
	append_log(info);
}

void UiAFSimTest::on_server_log(const QString& info)
{
	append_log(info);
}

void UiAFSimTest::on_datagram_received(const QByteArray& data, const QString& peer, quint16 port, qint64 time_ms)
{
	append_packet(data, QStringLiteral("RECV"), QString("%1:%2").arg(peer).arg(port), time_ms);
}

void UiAFSimTest::on_datagram_sent(const QByteArray& data, const QString& target, quint16 port, qint64 time_ms)
{
	append_packet(data, QStringLiteral("SEND"), QString("%1:%2").arg(target).arg(port), time_ms);
}

QObject* UiAFSimTest::root_object() const
{
	return engine.rootObjects().isEmpty() ? nullptr : engine.rootObjects().first();
}

void UiAFSimTest::center_on_primary_screen(QQuickWindow* window)
{
	if (window == nullptr) return;

	QScreen* screen = window->screen();
	const QList<QScreen*> screens = QGuiApplication::screens();
	if (screen == nullptr || !screens.contains(screen)) {
		screen = QGuiApplication::primaryScreen();
	}
	if (screen == nullptr) return;

	const QRect geometry = screen->availableGeometry();
	if (geometry.width() <= 0 || geometry.height() <= 0) return;

	if (window->screen() != screen) {
		window->setScreen(screen);
	}

	QSize target_size(qMax(1, window->width()), qMax(1, window->height()));
	if (target_size.width() > geometry.width() || target_size.height() > geometry.height()) {
		target_size.scale(geometry.size(), Qt::KeepAspectRatio);
		window->resize(qMax(1, target_size.width()), qMax(1, target_size.height()));
	}

	const int x = geometry.x() + qMax(0, (geometry.width() - window->width()) / 2);
	const int y = geometry.y() + qMax(0, (geometry.height() - window->height()) / 2);
	window->setPosition(x, y);
}

void UiAFSimTest::load_icgas_comm_config()
{
	QStringList candidates;
	candidates << cfg_path_candidate(QDir::currentPath());
	candidates << QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../ICGAS/Config/ConfigComm.json"));
	candidates << QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../../ICGAS/Config/ConfigComm.json"));

	for (const QString& path : candidates) {
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		const QJsonObject socket = doc.object().value("SOCKET").toObject();
		const QJsonObject client_obj = socket.value("afsim_client").toObject();
		const QJsonObject server_obj = socket.value("afsim_server").toObject();
		if (!client_obj.isEmpty()) {
			bind_addr = client_obj.value("addr").toString(bind_addr);
			bind_port = client_obj.value("port").toString(bind_port);
		}
		if (!server_obj.isEmpty()) {
			target_addr = server_obj.value("addr").toString(target_addr);
			target_port = server_obj.value("port").toString(target_port);
		}
		append_log(QString("[%1] [配置] [AFSim验证] 已加载 ICGAS 通信配置：%2").arg(cur_time_str(), path));
		emit configChanged();
		return;
	}
}

void UiAFSimTest::append_log(const QString& info)
{
	if (log_store.append(info)) {
		emit logRowsChanged();
	}
}

void UiAFSimTest::append_packet(const QByteArray& data, const QString& direction, const QString& endpoint, qint64 time_ms)
{
	const S_AFSimTestPacketUpdate update = packet_store.append(data, direction, endpoint, time_ms);
	const bool send_default_changed = remember_first_received_packet(data, direction);
	if (update.rows_changed) {
		emit packetRowsChanged();
	}
	if (update.selection_changed) {
		emit selectedPacketChanged();
	}
	if (update.pdu_views_changed) {
		emit pduViewsChanged();
	}
	if (send_default_changed) {
		emit sendModelChanged();
	}
}

bool UiAFSimTest::start_server(bool send_startup_platform)
{
	quint16 port = 0;
	if (!parse_port(target_port, port, QStringLiteral("afs_server_port"))) {
		return false;
	}
	if (!server.start(target_addr, port)) {
		return false;
	}
	if (send_startup_platform) {
		send_startup_platform_state();
	}
	emit stateChanged();
	return true;
}

bool UiAFSimTest::send_startup_platform_state()
{
	if (startup_entity_state_packet.isEmpty()) {
		append_log(QString("[%1] [状态] [发送模板] 尚未收到实体状态 PDU，启动发送端时未自动发送 90101 平台。")
			.arg(cur_time_str()));
		return false;
	}

	server.send_datagram(startup_entity_state_packet);
	append_log(QString("[%1] [发送] [发送模板] 启动发送端，已提交 90101 飞机平台实体状态。")
		.arg(cur_time_str()));
	return true;
}

bool UiAFSimTest::remember_first_received_packet(const QByteArray& data, const QString& direction)
{
	if (direction != QStringLiteral("RECV") || data.size() < DIS_PDU_HEADER_LEN) {
		return false;
	}
	const int pdu_type = UtilDisPdu::read_u8(data, 2);
	if (pdu_type != 1 && pdu_type != 2 && pdu_type != 3 && pdu_type != 23) {
		return false;
	}
	if (pdu_type == 1) {
		if (!startup_entity_state_packet.isEmpty()) {
			return false;
		}

		QString error;
		QByteArray derived_packet;
		if (!build_derived_aircraft_packet(data, derived_packet, error)) {
			append_log(QString("[%1] [错误] [发送模板] %2").arg(cur_time_str(), error));
			return false;
		}

		startup_entity_state_packet = derived_packet;
		append_log(QString("[%1] [状态] [发送模板] 已根据首个实体状态创建 90101 飞机平台：南方 100km，正东 %2 m/s。")
			.arg(cur_time_str()).arg(DERIVED_AIRCRAFT_EAST_SPEED_MPS, 0, 'f', 0));
		if (send_model.current_pdu_type() == pdu_type && send_model.apply_default_packet(startup_entity_state_packet)) {
			send_model.status = QStringLiteral("已生成 90101 飞机平台，发送端启动时自动发送。");
			return true;
		}
		return false;
	}
	if (first_received_packets.find(pdu_type) != first_received_packets.end()) {
		return false;
	}

	first_received_packets[pdu_type] = data;
	append_log(QString("[%1] [状态] [发送模板] 已记录 PDU-%2 的首个接收包。")
		.arg(cur_time_str()).arg(pdu_type, 2, 10, QChar('0')));
	return send_model.current_pdu_type() == pdu_type && send_model.apply_default_packet(data);
}

bool UiAFSimTest::apply_first_received_default()
{
	const int pdu_type = send_model.current_pdu_type();
	if (pdu_type == 1 && !startup_entity_state_packet.isEmpty()) {
		if (!send_model.apply_default_packet(startup_entity_state_packet)) {
			return false;
		}
		send_model.status = QStringLiteral("已生成 90101 飞机平台，发送端启动时自动发送。");
		return true;
	}

	const auto it = first_received_packets.find(pdu_type);
	if (it == first_received_packets.end()) {
		return false;
	}
	return send_model.apply_default_packet(it->second);
}

bool UiAFSimTest::parse_port(const QString& text, quint16& port, const QString& name)
{
	bool ok = false;
	const uint value = text.trimmed().toUInt(&ok);
	if (!ok || value == 0 || value > std::numeric_limits<quint16>::max()) {
		append_log(QString("[%1] [错误] [配置] %2 无效：%3").arg(cur_time_str(), name, text));
		return false;
	}
	port = static_cast<quint16>(value);
	return true;
}

QString UiAFSimTest::cur_time_str() const
{
	return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}
