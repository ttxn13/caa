#include "pch.hpp"
#include "UtilDisPdu.hpp"

// ---------- 基本数据读取，大端序 ----------
quint8 UtilDisPdu::read_u8(const QByteArray& data, int offset) 
{
	return static_cast<quint8>(data.at(offset));
}

quint16 UtilDisPdu::read_u16(const QByteArray& data, int offset) 
{
	return qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(data.constData() + offset));
}

quint32 UtilDisPdu::read_u32(const QByteArray& data, int offset) 
{
	return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(data.constData() + offset));
}

quint64 UtilDisPdu::read_u64(const QByteArray& data, int offset) 
{
	return qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(data.constData() + offset));
}

float UtilDisPdu::read_f32(const QByteArray& data, int offset)
{
	const quint32 raw = read_u32(data, offset);
	float value = 0.0f;
	std::memcpy(&value, &raw, sizeof(value));
	return value;
}

double UtilDisPdu::read_f64(const QByteArray& data, int offset)
{
	const quint64 raw = read_u64(data, offset);
	double value = 0.0;
	std::memcpy(&value, &raw, sizeof(value));
	return value;
}

// ---------- 通用字段转换 ----------
QString UtilDisPdu::entity_identifier_string(const QByteArray& data, int offset)
{
	return QString("%1:%2:%3")
		.arg(read_u16(data, offset))		// site
		.arg(read_u16(data, offset + 2))	// application
		.arg(read_u16(data, offset + 4));	// entity
}

QString UtilDisPdu::entity_type_string(const QByteArray& data, int offset)
{
	return QString("%1:%2:%3:%4:%5:%6:%7")
		.arg(read_u8(data, offset)		)	// kind		
		.arg(read_u8(data, offset + 1)	)	// domain		
		.arg(read_u16(data, offset + 2)	)	// country	
		.arg(read_u8(data, offset + 4)	)	// category	
		.arg(read_u8(data, offset + 5)	)	// subcategory
		.arg(read_u8(data, offset + 6)	)	// specific	
		.arg(read_u8(data, offset + 7)	);	// extra		
}

void UtilDisPdu::recv_pdu_header(const QByteArray& data, S_PDU_HEADER& header)
{
	header.protocol_version = read_u8	(data, 0);
	header.exercise_id		= read_u8	(data, 1);
	header.pdu_type			= read_u8	(data, 2);
	header.protocol_family	= read_u8	(data, 3);
	header.timestamp		= read_u32	(data, 4);
	header.pdu_length		= read_u16	(data, 8);
	header.pdu_status		= read_u8	(data, 10);
	header.padding			= read_u8	(data, 11);
}

bool UtilDisPdu::deal_pdu_entity_state(const QByteArray& data, S_PDU_ENTITY_STATE& pdu)
{
	if (data.size() < 144)	return false;
	recv_pdu_header(data, pdu.header);

	pdu.entity_id.entity_identifier			= get_dis_ori_id(data, PDU_HEADER_LEN);
	pdu.entity_id.entity_id_site			= read_u16	(data, PDU_HEADER_LEN);
	pdu.entity_id.entity_id_application		= read_u16	(data, PDU_HEADER_LEN + 2);
	pdu.entity_id.entity_id_entity			= read_u16	(data, PDU_HEADER_LEN + 4);

	pdu.force_id							= read_u8	(data, PDU_HEADER_LEN + 6);
	pdu.variable_parameter_count			= read_u8	(data, PDU_HEADER_LEN + 7);

	pdu.entity_type.entity_type_complete	= get_dis_ori_type(data, PDU_HEADER_LEN + 8);
	pdu.entity_type.entity_type_kind		= read_u8	(data, PDU_HEADER_LEN + 8);
	pdu.entity_type.entity_type_domain		= read_u8	(data, PDU_HEADER_LEN + 9);
	pdu.entity_type.entity_type_country		= read_u16	(data, PDU_HEADER_LEN + 10);
	pdu.entity_type.entity_type_category	= read_u8	(data, PDU_HEADER_LEN + 12);
	pdu.entity_type.entity_type_subcategory	= read_u8	(data, PDU_HEADER_LEN + 13);
	pdu.entity_type.entity_type_specific	= read_u8	(data, PDU_HEADER_LEN + 14);
	pdu.entity_type.entity_type_extra		= read_u8	(data, PDU_HEADER_LEN + 15);

	pdu.alt_entity_type.entity_type_complete	= get_dis_ori_type(data, PDU_HEADER_LEN + 16);
	pdu.alt_entity_type.entity_type_kind		= read_u8	(data, PDU_HEADER_LEN + 16);
	pdu.alt_entity_type.entity_type_domain		= read_u8	(data, PDU_HEADER_LEN + 17);
	pdu.alt_entity_type.entity_type_country		= read_u16	(data, PDU_HEADER_LEN + 18);
	pdu.alt_entity_type.entity_type_category	= read_u8	(data, PDU_HEADER_LEN + 20);
	pdu.alt_entity_type.entity_type_subcategory = read_u8	(data, PDU_HEADER_LEN + 21);
	pdu.alt_entity_type.entity_type_specific	= read_u8	(data, PDU_HEADER_LEN + 22);
	pdu.alt_entity_type.entity_type_extra		= read_u8	(data, PDU_HEADER_LEN + 23);

	pdu.entity_linear_velocity_x				= read_f32	(data, PDU_HEADER_LEN + 24);
	pdu.entity_linear_velocity_y				= read_f32	(data, PDU_HEADER_LEN + 28);
	pdu.entity_linear_velocity_z				= read_f32	(data, PDU_HEADER_LEN + 32);
	pdu.entity_location_x						= read_f64	(data, PDU_HEADER_LEN + 36);
	pdu.entity_location_y						= read_f64	(data, PDU_HEADER_LEN + 44);
	pdu.entity_location_z						= read_f64	(data, PDU_HEADER_LEN + 52);
	pdu.entity_orientation_psi					= read_f32	(data, PDU_HEADER_LEN + 60);
	pdu.entity_orientation_theta				= read_f32	(data, PDU_HEADER_LEN + 64);
	pdu.entity_orientation_phi					= read_f32	(data, PDU_HEADER_LEN + 68);

	pdu.entity_appearance						= read_u32	(data, PDU_HEADER_LEN + 72);
	pdu.dead_reckoning_algorithm				= read_u8	(data, PDU_HEADER_LEN + 76);
	for (int i = 0; i < 15; ++i)	pdu.dead_reckoning_other_parameters[i] = read_u8(data, PDU_HEADER_LEN + 77 + i);
	pdu.dead_reckoning_linear_acceleration_x	= read_f32(data, PDU_HEADER_LEN + 92);
	pdu.dead_reckoning_linear_acceleration_y	= read_f32(data, PDU_HEADER_LEN + 96);
	pdu.dead_reckoning_linear_acceleration_z	= read_f32(data, PDU_HEADER_LEN + 100);
	pdu.dead_reckoning_angular_velocity_x		= read_f32(data, PDU_HEADER_LEN + 104);
	pdu.dead_reckoning_angular_velocity_y		= read_f32(data, PDU_HEADER_LEN + 108);
	pdu.dead_reckoning_angular_velocity_z		= read_f32(data, PDU_HEADER_LEN + 112);

	pdu.entity_marking_character_set = read_u8(data, PDU_HEADER_LEN + 116);
	for (int i = 0; i < 11; ++i) pdu.entity_marking[i] = read_u8(data, PDU_HEADER_LEN + 117 + i);
	pdu.entity_capabilities = read_u32(data, PDU_HEADER_LEN + 128);

	return true;
}

bool UtilDisPdu::deal_pdu_entity_state(const QByteArray& data, S_PLAT& plat)
{
	if (data.size() < 144)	return false;

	plat.valid = true;
	plat.side		= get_dis_side(data, PDU_HEADER_LEN + 6);
	plat.type		= get_dis_type(data, PDU_HEADER_LEN + 8);
	plat.plt_id	= QString("%1").arg(read_u16(data, PDU_HEADER_LEN + 4), 5, 10, QLatin1Char('0'));
	plat.fmt_id		= plat.plt_id;
	plat.cmd_id		= QStringLiteral("SELF");

	S_MOTION_FRAME motion;
	motion.pos_ecef = S_POS_ECEF(read_f64(data, PDU_HEADER_LEN + 36), read_f64(data, PDU_HEADER_LEN + 44), read_f64(data, PDU_HEADER_LEN + 52));
	motion.vel_ecef = S_VEL_ECEF(read_f32(data, PDU_HEADER_LEN + 24), read_f32(data, PDU_HEADER_LEN + 28), read_f32(data, PDU_HEADER_LEN + 32));
	motion.pos_lla = UtilCoor::pos_ecef2lla(motion.pos_ecef);
	motion.vel_loc = UtilCoor::vel_ecef2loc(motion.pos_lla, motion.vel_ecef);
	motion.update_time_ms = QDateTime::currentMSecsSinceEpoch();

	plat.list_motion.clear();
	plat.list_motion.push_back(motion);

	return true;
}

// ---------- 项目业务映射 ----------
QString UtilDisPdu::get_dis_ori_id(const QByteArray& data, int offset)
{
	if (offset + 6 > data.size()) return "invalid";
	return QString("%1:%2:%3")
		.arg(read_u16(data, offset))
		.arg(read_u16(data, offset + 2))
		.arg(read_u16(data, offset + 4));
}

QString UtilDisPdu::get_dis_ori_type(const QByteArray& data, int offset)
{
	if (offset + 8 > data.size()) return "invalid";
	return QString("%1:%2:%3:%4:%5:%6:%7")
		.arg(static_cast<quint8>(data[offset]))
		.arg(static_cast<quint8>(data[offset + 1]))
		.arg(read_u16(data, offset + 2))
		.arg(static_cast<quint8>(data[offset + 4]))
		.arg(static_cast<quint8>(data[offset + 5]))
		.arg(static_cast<quint8>(data[offset + 6]))
		.arg(static_cast<quint8>(data[offset + 7]));
}

E_SIDE UtilDisPdu::get_dis_side(const QByteArray& data, int offset)
{
	const quint8 force_id = read_u8(data, PDU_HEADER_LEN + 6);
	switch (force_id) {
	default	:return E_SIDE::UKN	; break;	// 0 - Other，其他
	case 1	:return E_SIDE::RED	; break;	// 1 - Friendly，友方
	case 2	:return E_SIDE::BLUE	; break;	// 2 - Opposing，对方
	case 3	:return E_SIDE::WHITE	; break;	// 3 - Neutral，中立
	case 4	:;										// 4 - Friendly 2，友方二
	case 5	:;										// 5 - Opposing 2，对方二
	}
	return E_SIDE::UKN;
}

E_PLAT_TYPE UtilDisPdu::get_dis_type(const QByteArray& data, int offset)
{
	const int kind			= read_u8	(data, offset)		;	// 1 - Kind			实体大类			
	const int domain		= read_u8	(data, offset + 1)	;	// 2 - Domain		作战域或运动域		
	const int country		= read_u16	(data, offset + 2)	;	// 3 - Country		国家或地区代码		
	const int category		= read_u8	(data, offset + 4)	;	// 4 - Category		平台类别			
	const int subcategory	= read_u8	(data, offset + 5)	;	// 5 - Subcategory	子类别				
	const int specific		= read_u8	(data, offset + 6)	;	// 6 - Specific		具体型号			
	const int extra			= read_u8	(data, offset + 7)	;	// 7 - Extra		附加型号或扩展位	

	if (kind == 2) return E_PLAT_TYPE::MIS;
	if (kind != 1) return E_PLAT_TYPE::UKN;

	switch (domain) {
	// 空中平台
	case 2:
		switch (category) {
		case 1:
		case 2:		return E_PLAT_TYPE::FLT;	// 6 - 战斗机
		case 3:		return E_PLAT_TYPE::BOM;	// 7 - 轰炸机
		case 6:		return E_PLAT_TYPE::ELC;	// 5 - 电子战飞机
		case 8:		return E_PLAT_TYPE::WAN;	// 4 - 预警机
		default:	return E_PLAT_TYPE::UKN;
		}
	// 水面平台
	case 3:
		switch (category) {
		case 1:
		case 12:
		case 13:	return E_PLAT_TYPE::CAR;	// 1 - 航空母舰
		case 4:
		case 5:		return E_PLAT_TYPE::DST;	// 2 - 驱逐舰
		case 6:
		case 50:	return E_PLAT_TYPE::FRI;	// 3 - 护卫舰
		default:	return E_PLAT_TYPE::UKN;
		}

	default:
		return E_PLAT_TYPE::UKN;
	}


}
